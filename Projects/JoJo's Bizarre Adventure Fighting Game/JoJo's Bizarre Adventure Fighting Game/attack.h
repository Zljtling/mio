#ifndef _ATTACK_H_
#define _ATTACK_H_

#include "bullet.h"
#include "animation.h"

#include <cstdint>
#include <initializer_list>
#include <vector>

// 攻击区域沿用 Bullet 的伤害、目标、回调和回收流程，可放入同一子弹列表。
// 不设置图集时，只负责拳击、踢击等近战伤害判定，动作由角色动画显示。
class AttackArea : public Bullet
{
public:
	AttackArea() = default;
	~AttackArea() = default;

	void set_duration(int ms)
	{
		duration = ms > 0 ? ms : 1;	// 显示与存在时间，不包含开始前的延迟
	}

	void set_delay(int ms)
	{
		delay = ms > 0 ? ms : 0;		// 延迟期间不显示特效，也不造成伤害
	}

	void set_hit_interval(int ms)
	{
		hit_interval = ms > 0 ? ms : 0;	// 0 表示一次命中，正数表示重复命中的间隔
	}

	int get_hit_interval() const { return hit_interval; }	// 连续攻击由自身间隔控制伤害频率

	void set_hit_window(int begin_ms, int end_ms)
	{
		hit_begin = begin_ms > 0 ? begin_ms : 0;
		hit_end = end_ms >= hit_begin ? end_ms : hit_begin;	// 结束时间不包含在有效窗口中
	}

	void set_facing_right(bool flag)
	{
		is_facing_right = flag;		// 两个方向的动画同步更新，转向不会从头播放
	}

	void set_draw_offset(float x, float y)
	{
		draw_offset.x = x, draw_offset.y = y;	// 图片位置与碰撞区域分别配置
	}

	void set_atlas(Atlas* left, Atlas* right, int interval, bool loop)
	{
		atlas_left = left, atlas_right = right;
		frame_interval = interval > 0 ? interval : 1;
		is_loop = loop;
		animation_left.set_atlas(left);
		animation_right.set_atlas(right);
		animation_left.set_interval(frame_interval);
		animation_right.set_interval(frame_interval);
		animation_left.set_loop(loop);
		animation_right.set_loop(loop);
		frame_durations.clear();
		timer_animation = 0;
		animation_finished = false;
	}

	void set_frame_durations(std::initializer_list<int> durations)
	{
		frame_durations.clear();
		for (int ms : durations)
			frame_durations.push_back(ms > 0 ? ms : 1);
	}

	// 每次更新调用一次，使攻击区域跟随角色；offset_x 表示距角色前侧的间距。
	void follow(const Vector2& owner_position, const Vector2& owner_size,
		bool facing_right, float offset_x = 0, float offset_y = 0)
	{
		set_facing_right(facing_right);
		float x = facing_right ? owner_position.x + owner_size.x + offset_x
			: owner_position.x - size.x - offset_x;
		set_position(x, owner_position.y + offset_y);
	}

	bool check_collision(const Vector2& position, const Vector2& size) override
	{
		if (!check_can_hit() || this->size.x <= 0 || this->size.y <= 0
			|| size.x <= 0 || size.y <= 0)
			return false;

		return check_collision_shape(position, size);
	}

	bool check_collision_shape(const Vector2& position, const Vector2& size) const override
	{
		if (this->size.x <= 0 || this->size.y <= 0 || size.x <= 0 || size.y <= 0)
			return false;
		// 调试与实际攻击检查同一矩形，不沿用飞行子弹的中心点判定。
		return this->position.x + this->size.x >= position.x
			&& this->position.x <= position.x + size.x
			&& this->position.y + this->size.y >= position.y
			&& this->position.y <= position.y + size.y;
	}

	bool uses_point_collision() const override { return false; }
	bool check_debug_can_hit() const override
	{
		return size.x > 0 && size.y > 0 && check_can_hit();
	}

	LPCTSTR get_debug_state() const override
	{
		if (can_remove) return _T("待回收");
		if (!valid) return _T("失效");
		if (get_time_stopped()) return _T("时停冻结");
		if (elapsed_time < delay) return _T("延迟");
		if (!check_active()) return _T("结束");
		std::int64_t active_time = elapsed_time - delay;
		if (active_time < hit_begin || (hit_end >= 0 && active_time >= hit_end))
			return _T("非伤害帧");
		if (has_hit && hit_interval == 0) return _T("已命中");
		if (has_hit && elapsed_time < next_hit_time) return _T("命中间隔");
		return check_debug_can_hit() ? _T("可命中") : _T("尺寸无效");
	}

	void on_collide() override
	{
		if (!check_can_hit())
			return;

		has_hit = true;
		next_hit_time = elapsed_time + hit_interval;
		Bullet::on_collide();	// 先记录命中，再回调，防止同一更新中重复触发
	}

	void on_update(int delta) override
	{
		if (get_time_stopped() || can_remove || delta <= 0)
			return;

		std::int64_t old_time = elapsed_time;
		std::int64_t end_time = (std::int64_t)delay + duration;
		elapsed_time += delta;
		if (elapsed_time >= end_time)
		{
			remove();
			return;
		}

		// 只把进入显示阶段后的时间传给动画，避免延迟结束时跳过前几帧。
		int animation_delta = get_active_delta(old_time);
		if (animation_delta > 0)
			update_animation(animation_delta);
	}

	void on_draw(const Camera& camera) const override
	{
		if (!check_active())
			return;

		Atlas* atlas = is_facing_right ? atlas_right : atlas_left;
		if (!atlas || atlas->get_size() <= 0)
			return;

		IMAGE* frame = atlas->get_image(0);
		float x = is_facing_right ? position.x + draw_offset.x
			: position.x + size.x - draw_offset.x - frame->getwidth();
		const Animation& animation = is_facing_right ? animation_right : animation_left;
		animation.on_draw(camera, (int)x, (int)(position.y + draw_offset.y));
	}

protected:
	bool check_active() const
	{
		return !can_remove && elapsed_time >= delay
			&& elapsed_time < (std::int64_t)delay + duration;
	}

	int get_active_delta(std::int64_t old_time) const
	{
		std::int64_t begin = old_time > delay ? old_time : delay;
		return elapsed_time > begin ? (int)(elapsed_time - begin) : 0;
	}

	bool check_can_hit() const
	{
		if (get_time_stopped() || !valid || !check_active())
			return false;

		std::int64_t active_time = elapsed_time - delay;
		if (active_time >= duration || active_time < hit_begin
			|| (hit_end >= 0 && active_time >= hit_end))
			return false;

		return !has_hit || (hit_interval > 0 && elapsed_time >= next_hit_time);
	}

	void update_animation(int delta)
	{
		Atlas* reference = atlas_right && atlas_right->get_size() > 0 ? atlas_right : atlas_left;
		if (!reference || reference->get_size() <= 0 || animation_finished)
			return;

		timer_animation += delta;
		while (!animation_finished)
		{
			int index = atlas_right && atlas_right->get_size() > 0
				? animation_right.get_idx_frame() : animation_left.get_idx_frame();
			int interval = index < (int)frame_durations.size() ? frame_durations[index] : frame_interval;
			if (timer_animation < interval)
				break;
			timer_animation -= interval;

			// 最后一帧也显示完整的时长，非循环动画随后保持最后一帧。
			if (!is_loop && index >= reference->get_size() - 1)
			{
				animation_finished = true;
				break;
			}
			if (atlas_left && atlas_left->get_size() > 0)
			{
				animation_left.set_interval(interval);
				animation_left.on_update(interval);
			}
			if (atlas_right && atlas_right->get_size() > 0)
			{
				animation_right.set_interval(interval);
				animation_right.on_update(interval);
			}
		}
	}

	bool is_facing_right = true;		// 特效朝向
	Vector2 draw_offset;				// 图片相对碰撞区域的偏移
	Animation animation_left;		// 朝左特效动画
	Animation animation_right;		// 朝右特效动画
	Atlas* atlas_left = nullptr;		// 图集由 main.cpp 管理，这里不释放
	Atlas* atlas_right = nullptr;
	std::int64_t elapsed_time = 0;	// 攻击区域存在时间

private:
	int duration = 200;				// 攻击区域默认存在 200ms，创建后可调整
	int delay = 0;					// 开始延迟
	int hit_interval = 0;			// 重复伤害间隔
	int hit_begin = 0;				// 伤害窗口开始，按延迟结束后的时间计算
	int hit_end = -1;				// -1 表示整个存在阶段都可命中
	bool has_hit = false;			// 是否已经命中过指定目标
	std::int64_t next_hit_time = 0;	// 下一次允许命中的时间
	int frame_interval = 40;			// 未指定单帧时长时的统一间隔
	std::int64_t timer_animation = 0;// 动画累计时间，保留更新后的余量
	bool is_loop = false;			// 特效是否循环
	bool animation_finished = false;// 非循环特效是否播放完毕
	std::vector<int> frame_durations;// 每帧时长，可与已确认素材的 animation.json 对齐
};

#endif // !_ATTACK_H_
