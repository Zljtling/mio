#ifndef _HIEROPHANT_H_
#define _HIEROPHANT_H_

#include "attack.h"
#include "audio.h"

extern Atlas atlas_hierophant_green_magic_circle_left;
extern Atlas atlas_hierophant_green_magic_circle_right;
extern Atlas atlas_hierophant_green_emerald_spiral_left;
extern Atlas atlas_hierophant_green_emerald_spiral_right;
extern Atlas atlas_hierophant_green_emerald_cannon_hit_left;
extern Atlas atlas_hierophant_green_emerald_cannon_hit_right;

// 法皇螺旋炮：魔法阵、螺旋风和命中溅射组成持续攻击区域。
// 法皇本体的召唤、释放和退场动画由角色技能控制，此类只负责炮的三秒攻击阶段。
class HierophantBullet : public AttackArea
{
public:
	HierophantBullet()
	{
		size.x = 240, size.y = 40;	// 螺旋风的有效部分，排除上下透明区域
		set_attack_kind(AttackKind::Ultimate);	// 技能不触发普攻累计保护
		set_hit_shake(5.0f, 110);
		damage = 5;				// 持续炮击每次命中造成五点伤害

		set_duration(3000);
		set_hit_interval(100);		// 三秒内每 100ms 最多结算一次伤害
		set_atlas(&atlas_hierophant_green_emerald_spiral_left,
			&atlas_hierophant_green_emerald_spiral_right, 50, true);
		set_draw_offset(-8, -20);

		animation_circle_left.set_atlas(&atlas_hierophant_green_magic_circle_left);
		animation_circle_right.set_atlas(&atlas_hierophant_green_magic_circle_right);
		animation_circle_left.set_interval(50);
		animation_circle_right.set_interval(50);
		animation_circle_left.set_loop(true);
		animation_circle_right.set_loop(true);

		animation_hit_left.set_atlas(&atlas_hierophant_green_emerald_cannon_hit_left);
		animation_hit_right.set_atlas(&atlas_hierophant_green_emerald_cannon_hit_right);
		animation_hit_left.set_interval(50);
		animation_hit_right.set_interval(50);
		animation_hit_left.set_loop(false);
		animation_hit_right.set_loop(false);
	}

	~HierophantBullet() = default;

	bool check_collision(const Vector2& target_position, const Vector2& target_size) override
	{
		if (!AttackArea::check_collision(target_position, target_size))
			return false;

		// 在敌人与攻击区域相交的位置显示溅射，不固定在炮的末端。
		float top = target_position.y > position.y ? target_position.y : position.y;
		float bottom = target_position.y + target_size.y < position.y + size.y
			? target_position.y + target_size.y : position.y + size.y;
		float x = is_facing_right ? target_position.x : target_position.x + target_size.x;
		x = x < position.x ? position.x : x;
		x = x > position.x + size.x ? position.x + size.x : x;
		collision_position.x = x, collision_position.y = (top + bottom) / 2;
		has_collision_position = true;
		return true;
	}

	void on_collide() override
	{
		if (!check_can_hit())
			return;

		hit_position = has_collision_position ? collision_position
			: Vector2(is_facing_right ? position.x + size.x : position.x, position.y + size.y / 2);
		if (!is_hit_playing)
		{
			animation_hit_left.reset();
			animation_hit_right.reset();
			timer_hit = 0;
			hit_elapsed = 0;
			is_hit_playing = true;
		}
		play_player_sound(get_source_id(), _T("kakyoin_emerald_cannon_hit"), 0, -1, false);
		AttackArea::on_collide();	// 正在播放时保留进度，避免高频命中让溅射一直停在首帧
	}

	void on_update(int delta) override
	{
		if (get_time_stopped() || delta <= 0) return; // 魔法阵、螺旋风和溅射都保留当前帧
		std::int64_t old_time = elapsed_time;
		AttackArea::on_update(delta);
		if (delta <= 0 || !check_active())
			return;

		int animation_delta = get_active_delta(old_time);
		has_collision_position = false;
		timer_circle += animation_delta;
		while (timer_circle >= 50)
		{
			timer_circle -= 50;
			if (atlas_hierophant_green_magic_circle_left.get_size() > 0)
				animation_circle_left.on_update(50);
			if (atlas_hierophant_green_magic_circle_right.get_size() > 0)
				animation_circle_right.on_update(50);
		}

		if (is_hit_playing)
		{
			hit_elapsed += animation_delta;
			if (hit_elapsed >= 300)
			{
				is_hit_playing = false;
				return;
			}
			timer_hit += animation_delta;
			while (timer_hit >= 50)
			{
				timer_hit -= 50;
				if (atlas_hierophant_green_emerald_cannon_hit_left.get_size() > 0)
					animation_hit_left.on_update(50);
				if (atlas_hierophant_green_emerald_cannon_hit_right.get_size() > 0)
					animation_hit_right.on_update(50);
			}
		}
	}

	void on_draw(const Camera& camera) const override
	{
		if (!check_active())
			return;

		Atlas* circle_atlas = is_facing_right ? &atlas_hierophant_green_magic_circle_right
			: &atlas_hierophant_green_magic_circle_left;
		if (circle_atlas->get_size() > 0)
		{
			IMAGE* frame = circle_atlas->get_image(0);
			float offset_x = draw_offset.x - 28;	// 与原素材的魔法阵、螺旋风相对位置一致
			float x = is_facing_right ? position.x + offset_x
				: position.x + size.x - offset_x - frame->getwidth();
			const Animation& animation = is_facing_right ? animation_circle_right : animation_circle_left;
			animation.on_draw(camera, (int)x, (int)(position.y + draw_offset.y));
		}

		AttackArea::on_draw(camera);

		Atlas* hit_atlas = is_facing_right ? &atlas_hierophant_green_emerald_cannon_hit_right
			: &atlas_hierophant_green_emerald_cannon_hit_left;
		if (is_hit_playing && hit_atlas->get_size() > 0)
		{
			IMAGE* frame = hit_atlas->get_image(0);
			const Animation& animation = is_facing_right ? animation_hit_right : animation_hit_left;
			animation.on_draw(camera, (int)(hit_position.x - frame->getwidth() / 2.0f),
				(int)(hit_position.y - frame->getheight() / 2.0f));
		}
	}

private:
	Animation animation_circle_left;		// 朝左魔法阵动画
	Animation animation_circle_right;	// 朝右魔法阵动画
	Animation animation_hit_left;		// 朝左命中溅射动画
	Animation animation_hit_right;		// 朝右命中溅射动画
	std::int64_t timer_circle = 0;		// 魔法阵动画累计时间
	std::int64_t timer_hit = 0;			// 溅射动画累计时间
	std::int64_t hit_elapsed = 0;			// 当前溅射已播放的时间
	Vector2 collision_position;			// 本次碰撞的接触位置
	Vector2 hit_position;				// 溅射绘制中心
	bool has_collision_position = false;// 是否有本次碰撞位置
	bool is_hit_playing = false;			// 是否正在显示命中溅射
};

#endif // !_HIEROPHANT_H_
