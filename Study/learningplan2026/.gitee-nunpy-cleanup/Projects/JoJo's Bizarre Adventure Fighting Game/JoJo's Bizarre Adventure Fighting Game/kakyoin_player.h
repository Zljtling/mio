#ifndef _KAKYOIN_PLAYER_H_
#define _KAKYOIN_PLAYER_H_

#include "player.h"
#include "character_stats.h"
#include "quill_bullet.h"
#include "emerald.h"
#include "hierophant.h"

extern Atlas atlas_Kakyoin_hurt_left;
extern Atlas atlas_Kakyoin_hurt_right;
extern Atlas atlas_Kakyoin_block_left;
extern Atlas atlas_Kakyoin_block_right;
extern Atlas atlas_Kakyoin_idle_left;
extern Atlas atlas_Kakyoin_idle_right;
extern Atlas atlas_Kakyoin_run_left;
extern Atlas atlas_Kakyoin_run_right;
extern Atlas atlas_Kakyoin_jump_left;
extern Atlas atlas_Kakyoin_jump_right;
extern Atlas atlas_Kakyoin_defeat_left;
extern Atlas atlas_Kakyoin_defeat_right;
extern Atlas atlas_Kakyoin_quill_shot_left;
extern Atlas atlas_Kakyoin_quill_shot_right;
extern Atlas atlas_hierophant_green_emerald_burst_left;
extern Atlas atlas_hierophant_green_emerald_burst_right;
extern Atlas atlas_hierophant_green_emerald_cannon_left;
extern Atlas atlas_hierophant_green_emerald_cannon_right;

class KakyoinPlayer : public Player
{
public:
	KakyoinPlayer()
	{
		animation_idle_left.set_atlas(&atlas_Kakyoin_idle_left);
		animation_idle_right.set_atlas(&atlas_Kakyoin_idle_right);
		animation_run_left.set_atlas(&atlas_Kakyoin_run_left);
		animation_run_right.set_atlas(&atlas_Kakyoin_run_right);
		animation_jump_left.set_atlas(&atlas_Kakyoin_jump_left);
		animation_jump_right.set_atlas(&atlas_Kakyoin_jump_right);
		hp = max_hp = character_stat_int("kakyoin", "max_hp", 270);
		mp = character_stat_int("kakyoin", "initial_mp", 30);
		run_velocity = (float)character_stat("kakyoin", "run_speed", 0.40);
		jump_velocity = -(float)character_stat("kakyoin", "jump_speed", 0.85);
		normal_protection_threshold = character_stat_int("kakyoin", "normal_protection_threshold", 30);
		atlas_hurt_left = &atlas_Kakyoin_hurt_left;
		atlas_hurt_right = &atlas_Kakyoin_hurt_right;
		hurt_durations = { 20,20,30,10,20,30,50,20,20,40,20,20,40,10,20,30 }; // 已确认的 16 帧受击动画时长
		for (int duration : hurt_durations) hurt_duration += duration;
		atlas_block_left = &atlas_Kakyoin_block_left;
		atlas_block_right = &atlas_Kakyoin_block_right;
		block_durations = { 10,10,30,20,20,30,30,40,30,40,30,10,20,20,10,10 };
		block_hold_begin = 3; block_hold_end = 4;
		block_impact_begin = 5; block_impact_end = 9;
		block_exit_begin = 10;		// 防御循环及收手段对应现有 16 帧素材
		atlas_defeat_left = &atlas_Kakyoin_defeat_left;
		atlas_defeat_right = &atlas_Kakyoin_defeat_right;
		defeat_offset_x = -20;			// 192px 倒地画布中的站立姿势右移了 20px
		animation_idle_left.set_interval(50);
		animation_idle_right.set_interval(50);
		animation_run_left.set_interval(30);
		animation_run_right.set_interval(30);
		animation_jump_left.set_interval(75);
		animation_jump_right.set_interval(75);
		animation_jump_left.set_loop(false);
		animation_jump_right.set_loop(false);
		size.x = 72, size.y = 126;		// 缩窄受击与平台判定，水平位置由基类对齐角色画布中心
		special_mp_cost = character_stat_int("kakyoin", "special_cost", 20);
		ultimate_mp_cost = character_stat_int("kakyoin", "ultimate_cost", 100);
		attack_cd = character_stat_int("kakyoin", "normal_cooldown_ms", 250);
		timer_attack_cd.set_wait_time(attack_cd);	// 普攻发射频率提高至两倍
	}

	~KakyoinPlayer() override = default;

	void on_update(int delta) override
	{
		Player::on_update(delta);
		delta = active_update_delta;
		if (delta <= 0)
			return;

		if (is_attacking)
		{
			attack_elapsed += delta;
			if (!quill_released && attack_elapsed >= quill_release_time)
			{
				quill_released = true;
				spawn_quill_bullet(speed_quill);
			}
			if (attack_elapsed >= quill_duration)
				is_attacking = false;
		}

		if (is_attacking_ex)
		{
			special_elapsed += delta;
			while (burst_released_count < (int)burst_centers.size())
			{
				int release_time = burst_release_time + burst_released_count * burst_release_interval;
				if (special_elapsed < release_time)
					break;
				int age = (int)(special_elapsed - release_time);
				if (age < burst_effect_duration)
					spawn_emerald_burst(burst_released_count, age);
				burst_released_count++;		// 卡顿跨过多个落点时逐一补发，已消散的爆炸直接跳过
			}
			if (special_elapsed >= burst_duration)
				is_attacking_ex = false;		// 法皇退场结束后允许移动
		}

		if (is_attacking_exxxx)
		{
			ultimate_elapsed += delta;
			if (!cannon_released && ultimate_elapsed >= cannon_prepare_duration)
			{
				cannon_released = true;
				int age = (int)(ultimate_elapsed - cannon_prepare_duration);
				if (age < cannon_attack_duration)
				{
					AttackArea* area = new HierophantBullet();
					area->set_duration(cannon_attack_duration);
					area->set_damage(character_stat_int("kakyoin", "ultimate_damage_per_hit", 5));
					area->on_update(age);
					register_bullet(area, character_stat_int("kakyoin", "ultimate_mp_gain_per_hit", 2));
					play_player_sound(id, _T("kakyoin_emerald_cannon_loop"), age, cannon_attack_duration - age);
					attach_attack(area, 204, 13, cannon_attack_duration - age, false);
				}
			}
			if (!cannon_audio_stopped && ultimate_elapsed >= cannon_prepare_duration + cannon_attack_duration)
			{
				stop_player_sound(id, _T("kakyoin_emerald_cannon_loop"));
				cannon_audio_stopped = true; // 炮击结束即停止持续声音，退场阶段不再发射
			}
			if (ultimate_elapsed >= cannon_total_duration)
			{
				is_attacking_exxxx = false;
				is_facing_locked = false;	// 三秒炮击后仍播放退场，最后解除朝向锁定
			}
		}
	}

	void on_draw(const Camera& camera) override
	{
		if (get_transformed()) { Player::on_draw(camera); return; }
		if (is_attacking_ex)
		{
			draw_frame(camera, atlas_hierophant_green_emerald_burst_left,
				atlas_hierophant_green_emerald_burst_right, (int)(animation_time(special_elapsed, burst_duration, 900) / 25), -55, -37);
			Player::on_draw(camera);
		}
		else if (is_attacking && !get_hurt_active())
		{
			on_draw_particles(camera);	// 掏笔动作期间仍显示地面上的跑动烟尘
			draw_frame(camera, atlas_Kakyoin_quill_shot_left, atlas_Kakyoin_quill_shot_right,
				(int)(animation_time(attack_elapsed, quill_duration, 160) / 10));
		}
		else
		{
			Player::on_draw(camera);
			if (is_attacking_exxxx)
				draw_frame(camera, atlas_hierophant_green_emerald_cannon_left,
					atlas_hierophant_green_emerald_cannon_right,
					get_frame_index(cannon_animation_time(), cannon_durations), 56, -28);
		}
	}

	void on_attack() override
	{
		if (!begin_normal_attack())
			return;
		attack_elapsed = 0;
		quill_released = false;
	}

	void on_attack_ex() override
	{
		if (!begin_special_attack())
			return;
		play_player_sound(id, _T("kakyoin_emerald_cast"));
		special_elapsed = 0;
		burst_released_count = 0;
	}

	void on_attack_exxxx() override
	{
		if (!begin_ultimate_attack())
			return;
		play_player_sound(id, _T("kakyoin_emerald_cannon_cast"));
		cannon_audio_stopped = false;
		ultimate_elapsed = 0;
		cannon_released = false;
		is_facing_locked = true;				// 允许左右移动和跳跃，整次技能不转身
	}

private:
	std::int64_t cannon_animation_time() const
	{
		if (ultimate_elapsed < cannon_prepare_duration)
			return animation_time(ultimate_elapsed, cannon_prepare_duration, 560);
		if (ultimate_elapsed < cannon_prepare_duration + cannon_attack_duration)
			return 560 + animation_time(ultimate_elapsed - cannon_prepare_duration, cannon_attack_duration, 3000);
		return 3560 + animation_time(ultimate_elapsed - cannon_prepare_duration - cannon_attack_duration, cannon_exit_duration, 440);
	}

	void spawn_quill_bullet(float speed)
	{
		Bullet* bullet = new QuillBullet();
		bullet->set_damage(character_stat_int("kakyoin", "normal_damage", 7));
		const Vector2& bullet_size = bullet->get_size();
		float x = is_facing_right ? position.x + 104
			: position.x + sprite_width - 104 - bullet_size.x;
		bullet->set_position(x, position.y + 50 - bullet_size.y / 2);
		bullet->set_velocity(is_facing_right ? speed : -speed, 0);
		register_bullet(bullet, character_stat_int("kakyoin", "normal_mp_gain", 8));	// 羽毛笔从胸前手部飞出

		play_player_sound(id, _T("kakyoin_quill_shoot"));	// 40ms 发射点同步播放短音效

	}

	void spawn_emerald_burst(int index, int age)
	{
		const Vector2& offset = burst_centers[index];
		EmeraldBullet* bullet = new EmeraldBullet();
		bullet->set_damage(character_stat_int("kakyoin", "special_damage", 10));
		float x = is_facing_right ? position.x + offset.x
			: position.x + sprite_width - offset.x;
		bullet->set_center(x, position.y + offset.y);
		bullet->on_update(age);
		register_bullet(bullet, character_stat_int("kakyoin", "special_mp_gain", 2));	// 每颗绿宝石有效命中回复两点能量
		play_player_sound(id, _T("kakyoin_emerald_burst"), 0, -1, false);	// 八个爆炸不反复截断同一段尾音
	}

private:
	const float speed_quill = 1.5f;			// 羽毛笔速度，单位 px/ms
	const int quill_duration = character_duration_ms("kakyoin", "normal_cast_ms", 160);
	const int quill_release_time = quill_duration * 40 / 160;
	const int burst_duration = character_duration_ms("kakyoin", "special_cast_ms", 900);
	const int burst_release_time = burst_duration * 400 / 900;
	const int burst_release_interval = burst_duration * 35 / 900;
	const int burst_effect_duration = 200;
	const int cannon_prepare_duration = character_duration_ms("kakyoin", "ultimate_startup_ms", 560, true);
	const int cannon_attack_duration = character_duration_ms("kakyoin", "ultimate_effect_duration_ms", 3000);
	const int cannon_exit_duration = character_duration_ms("kakyoin", "ultimate_recovery_ms", 440, true);
	const int cannon_total_duration = cannon_prepare_duration + cannon_attack_duration + cannon_exit_duration;
	std::int64_t attack_elapsed = 0;
	std::int64_t special_elapsed = 0;
	std::int64_t ultimate_elapsed = 0;
	bool quill_released = false;
	int burst_released_count = 0;			// 已处理的落点数量，时停时随 special_elapsed 一起暂停
	bool cannon_released = false;
	bool cannon_audio_stopped = false;
	const std::vector<Vector2> burst_centers = {
		{ 148,80 }, { 180,60 }, { 196,32 }, { 224,60 },
		{ 244,80 }, { 212,76 }, { 164,48 }, { 196,56 } }; // 保持原射程与高度，在内部补足可重叠的爆炸中心
	const std::vector<int> cannon_durations = {
		40,40,40,40,40,40,40,40,40,40,40,40,40,40,3000,40,40,40,40,40,40,40,40,40,40,40 };
};

#endif // !_KAKYOIN_PLAYER_H_
