#ifndef _BULLET_H_
#define _BULLET_H_

#include <cstdlib>

#include "vector2.h"
#include "camera.h"
#include "player_id.h"
#include "util.h"

#include <graphics.h>
#include <functional>

extern bool is_debug;
extern Camera main_camera;

class Bullet
{
public:
	enum class AttackKind { Normal, Special, Ultimate }; // 区分普攻累计保护与技能命中
	void set_attack_kind(AttackKind kind) { attack_kind = kind; }
	AttackKind get_attack_kind() const { return attack_kind; }
	void set_normal_protection_threshold(int value) { normal_protection_threshold = value; }
	int get_normal_protection_threshold() const { return normal_protection_threshold; }
	void set_source_id(PlayerID id) { source_id = id; }
	PlayerID get_source_id() const { return source_id; }
	void set_transform_duration(int ms) { transform_duration = ms > 0 ? ms : 0; }
	int get_transform_duration() const { return transform_duration; }
	void set_hit_shake(float strength, int duration) { hit_shake_strength = strength; hit_shake_duration = duration; }
	void set_persistent_on_owner_hurt(bool value) { persistent_on_owner_hurt = value; }
	bool get_persistent_on_owner_hurt() const { return persistent_on_owner_hurt; }

	void set_time_stopped(bool flag) { is_time_stopped = flag; } // 保留有效状态和命中间隔，解冻后继续
	bool get_time_stopped() const { return is_time_stopped; }

	void set_knockback_direction(int value) { knockback_direction = value < 0 ? -1 : 1; }
	int get_knockback_direction() const { return knockback_direction; }

	Bullet() = default;
	virtual ~Bullet() = default;	// 通过 Bullet 指针移除子类时，正确调用子类析构函数

	void set_damage(int val)
	{
		damage = val;
	}

	int get_damage() const
	{
		return damage;
	}

	void set_position(float x, float y)
	{
		position.x = x, position.y = y;
	}

	const Vector2& get_position() const
	{
		return position;
	}

	const Vector2& get_size() const
	{
		return size;
	}

	void set_size(float width, float height)
	{
		size.x = width > 0 ? width : 0;
		size.y = height > 0 ? height : 0;
	}

	void set_velocity(float x, float y)
	{
		velocity.x = x, velocity.y = y;
	}

	void set_collide_target(PlayerID target)
	{
		target_id = target;
	}

	PlayerID get_collide_target() const
	{
		return target_id;
	}

	void set_callback(std::function<void()> callback)
	{
		this->callback = callback;
	}

	void set_valid(bool flag)
	{
		valid = flag;
	}

	bool get_valid() const
	{
		return valid;
	}

	bool check_can_remove() const
	{
		return can_remove;
	}

	void remove()
	{
		valid = false;
		can_remove = true;
	}

	// 场景先核对 target_id、检查碰撞，再调用此函数；子类决定是否结束攻击。
	virtual void on_collide()
	{
		debug_hit_count++;		// 仅记录已经触发的命中，调试绘制不调用碰撞回调
		if (hit_shake_strength > 0) main_camera.shake(hit_shake_strength, hit_shake_duration);
		if (callback)
			callback();
	}

	virtual bool check_collision(const Vector2& position, const Vector2& size)
	{
		if (is_time_stopped || !valid || can_remove || this->size.x <= 0 || this->size.y <= 0
			|| size.x <= 0 || size.y <= 0)
			return false;

		return check_collision_shape(position, size);
	}

	// 只读取空间关系，不改变命中状态；实际判定和调试显示共用此形状。
	virtual bool check_collision_shape(const Vector2& position, const Vector2& size) const
	{
		if (this->size.x <= 0 || this->size.y <= 0 || size.x <= 0 || size.y <= 0)
			return false;
		return this->position.x + this->size.x / 2 >= position.x
			&& this->position.x + this->size.x / 2 <= position.x + size.x
			&& this->position.y + this->size.y / 2 >= position.y
			&& this->position.y + this->size.y / 2 <= position.y + size.y;
	}

	virtual void on_update(int delta) {}
	virtual void on_draw(const Camera& camera) const {}

	virtual bool uses_point_collision() const { return true; }
	virtual bool check_debug_can_hit() const
	{
		return !is_time_stopped && valid && !can_remove && size.x > 0 && size.y > 0;
	}

	virtual LPCTSTR get_debug_state() const
	{
		if (can_remove) return _T("待回收");
		if (!valid) return _T("失效");
		if (is_time_stopped) return _T("时停冻结");
		return check_debug_can_hit() ? _T("可命中") : _T("尺寸无效");
	}

	int get_debug_hit_count() const { return debug_hit_count; }

	// 由场景最后统一调用，子类重写素材绘制也不会漏掉调试框。
	virtual void on_draw_debug(const Camera& camera, bool overlaps_target = false,
		bool target_can_be_hit = true) const
	{
		if (!is_debug || can_remove || size.x <= 0 || size.y <= 0)
			return;
		DebugDrawState draw_state;
		bool can_hit = check_debug_can_hit();
		COLORREF color = !can_hit ? RGB(128, 128, 128)
			: (overlaps_target && target_can_be_hit ? RGB(255, 70, 70)
				: (uses_point_collision() ? RGB(255, 255, 255) : RGB(255, 180, 40)));
		setlinecolor(color);
		setfillcolor(color);
		settextcolor(color);
		setlinestyle(can_hit ? PS_SOLID : PS_DOT, 1);
		rectangle(camera, (int)position.x, (int)position.y,
			(int)(position.x + size.x), (int)(position.y + size.y));
		if (uses_point_collision())
			solidcircle(camera, (int)(position.x + size.x / 2), (int)(position.y + size.y / 2), 3);

		LPCTSTR state = can_hit && overlaps_target && !target_can_be_hit
			? _T("目标不可受击") : get_debug_state();
		TCHAR text[160];
		_stprintf_s(text, _T("%s ->P%d 伤害:%d 命中:%d [%s]"),
			uses_point_collision() ? _T("点判定") : _T("矩形判定"),
			target_id == PlayerID::P1 ? 1 : 2, damage, debug_hit_count, state);
		const Vector2& camera_position = camera.get_position();
		outtextxy((int)(position.x - camera_position.x),
			(int)(position.y - camera_position.y) - 18, text);
	}

protected:	
	Vector2 size;						//子弹尺寸
	Vector2 position;					//子弹位置
	Vector2 velocity;					//子弹速度
	int damage = 10;					//子弹伤害
	bool valid = true;					//子弹是否有效
	bool can_remove = false;			//子弹是否可以被移除
	std::function<void()> callback;		//子弹碰撞回调函数
	PlayerID target_id = PlayerID::P1;	//子弹碰撞目标玩家ID 

private:
	int knockback_direction = 1; // 特殊技能和大招的发力方向
	bool is_time_stopped = false;
	AttackKind attack_kind = AttackKind::Normal;
	int normal_protection_threshold = 0; // 注册到玩家时，按角色设置 20 / 30 点阈值
	PlayerID source_id = PlayerID::P1;
	int debug_hit_count = 0;				// 此对象实际触发 on_collide 的次数
	int transform_duration = 0;			// 奶娃大招命中后目标的变身时间
	float hit_shake_strength = 0;
	int hit_shake_duration = 0;
	bool persistent_on_owner_hurt = false;

protected:
	bool check_if_exceeds_screen()
	{
		return (position.x + size.x <= 0 || position.x >= GAME_WORLD_WIDTH
			|| position.y + size.y <= 0 || position.y >= GAME_WORLD_HEIGHT);
	}
};

#endif // !_BULLET_H_
