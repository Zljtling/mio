#ifndef _JOTARO_PLAYER_H_
#define _JOTARO_PLAYER_H_

#include "player.h"
#include "character_stats.h"
#include "punchwind.h"
#include "orabarrage.h"

extern Atlas atlas_Jotaro_hurt_left;
extern Atlas atlas_Jotaro_hurt_right;
extern Atlas atlas_Jotaro_block_left;
extern Atlas atlas_Jotaro_block_right;
extern Atlas atlas_Jotaro_idle_left;
extern Atlas atlas_Jotaro_idle_right;
extern Atlas atlas_Jotaro_run_left;
extern Atlas atlas_Jotaro_run_right;
extern Atlas atlas_Jotaro_jump_left;
extern Atlas atlas_Jotaro_jump_right;
extern Atlas atlas_Jotaro_defeat_left;
extern Atlas atlas_Jotaro_defeat_right;
extern Atlas atlas_Jotaro_punch_left;
extern Atlas atlas_Jotaro_punch_right;
extern Atlas Atlas_Jotaro_kick_left;
extern Atlas Atlas_Jotaro_kick_right;
extern Atlas atlas_star_platinum_charged_punch_left;
extern Atlas atlas_star_platinum_charged_punch_right;
extern Atlas atlas_star_platinum_ora_barrage_left;
extern Atlas atlas_star_platinum_ora_barrage_right;
extern IMAGE img_star_platinum_summon_aura_left;
extern IMAGE img_star_platinum_summon_aura_right;
extern IMAGE img_star_platinum_charge_vortex_left;
extern IMAGE img_star_platinum_charge_vortex_right;

class JotaroPlayer : public Player
{
public:
	JotaroPlayer()
	{
		animation_idle_left.set_atlas(&atlas_Jotaro_idle_left);
		animation_idle_right.set_atlas(&atlas_Jotaro_idle_right);
		animation_run_left.set_atlas(&atlas_Jotaro_run_left);
		animation_run_right.set_atlas(&atlas_Jotaro_run_right);
		animation_jump_left.set_atlas(&atlas_Jotaro_jump_left);
		animation_jump_right.set_atlas(&atlas_Jotaro_jump_right);
		hp = max_hp = character_stat_int("jotaro", "max_hp", 300);
		mp = character_stat_int("jotaro", "initial_mp", 40);
		run_velocity = (float)character_stat("jotaro", "run_speed", 0.35);
		jump_velocity = -(float)character_stat("jotaro", "jump_speed", 0.85);
		attack_cd = character_stat_int("jotaro", "normal_cooldown_ms", 250);
		timer_attack_cd.set_wait_time(attack_cd);
		normal_protection_threshold = character_stat_int("jotaro", "normal_protection_threshold", 20);
		atlas_hurt_left = &atlas_Jotaro_hurt_left;
		atlas_hurt_right = &atlas_Jotaro_hurt_right;
		hurt_durations = { 20,40,20,20,80,30,30,30,60,40,40,50,30,30,30,120 }; // 已确认的 16 帧受击动画时长
		for (int duration : hurt_durations) hurt_duration += duration;
		atlas_block_left = &atlas_Jotaro_block_left;
		atlas_block_right = &atlas_Jotaro_block_right;
		block_durations = { 40,40,40,30,30,100,160,30,70,40,100,40,40,40,40,120 };
		block_hold_begin = 5; block_hold_end = 6;
		block_impact_begin = 7; block_impact_end = 10;
		block_exit_begin = 11;		// 防御循环及收手段对应现有 16 帧素材
		atlas_defeat_left = &atlas_Jotaro_defeat_left;
		atlas_defeat_right = &atlas_Jotaro_defeat_right;
		defeat_offset_x = -16;			// 192px 倒地画布中的站立姿势右移了 16px
		animation_idle_left.set_interval(50);
		animation_idle_right.set_interval(50);
		animation_run_left.set_interval(30);
		animation_run_right.set_interval(30);
		animation_jump_left.set_interval(75);
		animation_jump_right.set_interval(75);
		animation_jump_left.set_loop(false);
		animation_jump_right.set_loop(false);
		size.x = 72, size.y = 126;		// 缩窄受击与平台判定，水平位置由基类对齐角色画布中心
		special_mp_cost = character_stat_int("jotaro", "special_cost", 30);
		ultimate_mp_cost = character_stat_int("jotaro", "ultimate_cost", 100);
		allow_attack_during_ultimate = true;	// 时停期间允许使用替换后的普通攻击与踢击
	}

	~JotaroPlayer() override = default;

	void on_update(int delta) override
	{
		Player::on_update(delta);
		if (delta <= 0)
			return;

		if (is_attacking_exxxx && ultimate_startup_remaining == 0)
		{
			ultimate_remaining -= delta;	// 时停大招按实际时间结束，双方同时释放也不会互相延长
			if (ultimate_remaining <= 0)
			{
				ultimate_remaining = 0;
				is_attacking_exxxx = false;
				stop_player_sound(id, _T("jotaro_ora_barrage"));
				if (attack_type == AttackType::Ora)
				{
					clear_attached_attacks();
					finish_normal_attack();		// 五秒结束后不再保留欧拉伤害
				}
			}
		}

		delta = active_update_delta;
		if (delta <= 0)
			return;

		if (is_attacking_exxxx && ultimate_startup_remaining > 0)
		{
			ultimate_startup_remaining = (std::max)(0, ultimate_startup_remaining - delta);
			if (ultimate_startup_remaining == 0) activate_time_stop();
		}

		if (is_attacking)
			update_normal_attack(delta);

		if (is_attacking_ex && !is_attacking_exxxx)
		{
			special_elapsed += delta;
			if (!special_released && special_elapsed >= charged_release_time)
			{
				special_released = true;
				int age = (int)(special_elapsed - charged_release_time);
				if (age < charged_wind_duration)
				{
					AttackArea* area = new PunchWindBullet();
					charged_wind = area;
					area->on_update(age);		// 跨过释放点时保留动画余量
					area->set_damage(character_stat_int("jotaro", "special_damage", 30));
					register_bullet(area, character_stat_int("jotaro", "special_mp_gain", 20));
					attach_attack(area, 62, -5, charged_wind_duration - age, false);
				}
			}
			if (special_elapsed < charged_release_time + charged_wind_duration)
			{
				float x = special_elapsed < charged_release_time + 20 ? 35.0f : (special_elapsed < charged_release_time + 40 ? 28.0f : 62.0f);
				float y = special_elapsed >= charged_release_time + 20 && special_elapsed < charged_release_time + 40 ? -8.0f : -5.0f;
				set_attached_offset(charged_wind, x, y);	// 拳风前两帧贴近拳头，随后延伸到远处
			}
			else
				charged_wind = nullptr;
			if (special_elapsed >= charged_duration)
				is_attacking_ex = false;		// 含召唤、蓄力、出拳、收臂和淡出
		}
	}

	void on_draw(const Camera& camera) override
	{
		if (get_transformed()) { Player::on_draw(camera); return; }
		if (is_attacking_ex && !is_attacking_exxxx)
		{
			int index = get_frame_index(animation_time(special_elapsed, charged_duration, 1733), charged_durations);
			if (index < 7)
				draw_image(camera, is_facing_right ? &img_star_platinum_summon_aura_right
					: &img_star_platinum_summon_aura_left, -64, -64, summon_opacity[index]);
			else if (index >= 10 && index < 17)
				draw_image(camera, is_facing_right ? &img_star_platinum_charge_vortex_right
					: &img_star_platinum_charge_vortex_left, -80, index == 16 ? -71.0f : -70.0f,
					charge_opacity[index - 10]);
			draw_frame(camera, atlas_star_platinum_charged_punch_left,
				atlas_star_platinum_charged_punch_right, index, -64, (float)charged_stand_y[index] - 88);
			Player::on_draw(camera);			// 白金之星位于承太郎后上方
		}
		else if (is_attacking && !get_hurt_active() && attack_type == AttackType::Punch)
		{
			on_draw_particles(camera);	// 攻击姿势覆盖基础绘制时仍显示已有烟尘
			draw_frame(camera, atlas_Jotaro_punch_left, atlas_Jotaro_punch_right,
				get_frame_index(animation_time(attack_elapsed, punch_duration, 640), punch_durations));
		}
		else if (is_attacking && !get_hurt_active() && attack_type == AttackType::Kick)
		{
			on_draw_particles(camera);
			draw_frame(camera, Atlas_Jotaro_kick_left, Atlas_Jotaro_kick_right,
				get_frame_index(animation_time(attack_elapsed, kick_duration, 860), kick_durations));
		}
		else
		{
			Player::on_draw(camera);			// 欧拉时玩家继续显示跑动、跳跃或站立
			if (is_attacking && attack_type == AttackType::Ora)
				draw_frame(camera, atlas_star_platinum_ora_barrage_left,
					atlas_star_platinum_ora_barrage_right, (int)(animation_time(attack_elapsed, ora_duration, 480) / 20), 76, -25);
		}
	}

	void on_attack() override
	{
		start_normal_attack();
	}

	void on_kick() override
	{
		on_attack();	// 保留接口兼容，输入统一走普通攻击键
	}

	void on_attack_ex() override
	{
		if (!begin_special_attack())
			return;
		play_player_sound(id, _T("jotaro_charged_punch"), 0, charged_duration);	// 只有成功消耗能量发动技能才播放
		special_elapsed = 0;
		special_released = false;
		charged_wind = nullptr;
	}

	void on_attack_exxxx() override
	{
		if (!begin_ultimate_attack())
			return;
		ora_audio_segment = 0;
		ultimate_startup_remaining = ultimate_startup_duration;
		ultimate_remaining = 0;
		if (ultimate_startup_remaining == 0) activate_time_stop();
		else is_attacking_ex = true; // 前摇期间锁定移动与普攻，生效后才开始时停。
	}

	bool get_ultimate_invulnerable() const override
	{
		return get_time_stop_cast_remaining() > 0; // 成功发动至五秒结束，普通、特殊和大招均不扣血
	}

	int get_ultimate_remaining() const { return get_time_stop_cast_remaining(); }
	std::uint64_t get_time_stop_cast_id() const override { return time_stop_cast_id; }
	int get_time_stop_cast_remaining() const override
	{
		return hp > 0 && is_attacking_exxxx ? ultimate_remaining : 0;
	}

private:
	void activate_time_stop()
	{
		is_attacking_ex = false;
		ultimate_remaining = time_stop_duration;
		time_stop_cast_id++;
	}

	enum class AttackType { None, Punch, Kick, Ora };

	void start_normal_attack()
	{
		if (!begin_normal_attack())
			return;
		attack_type = is_attacking_exxxx ? AttackType::Ora
			: (rand() % 2 == 0 ? AttackType::Punch : AttackType::Kick); // 成功普攻后随机选择拳或踢
		attack_elapsed = 0;
		attack_released = false;
		if (attack_type == AttackType::Ora)
		{
			AttackArea* area = new OraBarrageBullet();
			area->set_damage(character_stat_int("jotaro", "ultimate_damage_per_hit", 10));
			int duration = ultimate_remaining < ora_duration ? ultimate_remaining : ora_duration;
			area->set_duration(duration);
			play_player_sound(id, _T("jotaro_ora_barrage"), ora_audio_segment * 500, duration);
			ora_audio_segment = (ora_audio_segment + 1) % 3; // 将 1.5 秒原音分成三段，按每次 480ms 连续拳轮流播放
			register_bullet(area, character_stat_int("jotaro", "ultimate_mp_gain_per_hit", 3));
			attach_attack(area, 115, -21, duration);	// 连续拳跟随移动和转向
			attack_released = true;
		}
	}

	void update_normal_attack(int delta)
	{
		attack_elapsed += delta;
		int duration = attack_type == AttackType::Punch ? punch_duration
			: (attack_type == AttackType::Kick ? kick_duration : ora_duration);
		int release = attack_type == AttackType::Punch ? punch_release_time : kick_release_time;
		int hit_duration = attack_type == AttackType::Punch ? 80 : 100;
		if (!attack_released && attack_elapsed >= release)
		{
			attack_released = true;
			int age = (int)(attack_elapsed - release);
			if (age < hit_duration)
			{
				AttackArea* area = new AttackArea();
				bool is_kick = attack_type == AttackType::Kick;
				area->set_size(is_kick ? 66.0f : 44.0f, is_kick ? 36.0f : 32.0f);
				area->set_damage(character_stat_int("jotaro", "normal_damage", 10));
				area->set_duration(hit_duration);
				area->on_update(age);
				register_bullet(area, character_stat_int("jotaro", "normal_mp_gain", 20)); // 拳与踢每次有效命中回能
				attach_attack(area, is_kick ? 116.0f : 92.0f,
					is_kick ? 80.0f : 40.0f, hit_duration - age);
			}
		}
		if (attack_elapsed >= duration)
			finish_normal_attack();			// 最后一帧显示完整后恢复默认动作
	}

	void finish_normal_attack()
	{
		is_attacking = false;
		attack_type = AttackType::None;
	}

private:
	const int punch_duration = character_duration_ms("jotaro", "normal_cast_ms", 640);
	const int kick_duration = character_duration_ms("jotaro", "kick_cast_ms", 860);
	const int ora_duration = character_duration_ms("jotaro", "ora_cast_ms", 480);
	const int punch_release_time = punch_duration * 210 / 640;
	const int kick_release_time = kick_duration * 380 / 860;
	const int charged_duration = character_duration_ms("jotaro", "special_cast_ms", 1733);
	const int charged_release_time = charged_duration * 453 / 1733;
	const int charged_wind_duration = 360;
	const int ultimate_startup_duration = character_duration_ms("jotaro", "ultimate_startup_ms", 0, true);
	const int time_stop_duration = character_duration_ms("jotaro", "ultimate_effect_duration_ms", 5000);
	AttackType attack_type = AttackType::None;
	std::int64_t attack_elapsed = 0;
	std::int64_t special_elapsed = 0;
	int ultimate_remaining = 0;
	int ultimate_startup_remaining = 0;
	int ora_audio_segment = 0;			// 不变速、不改音调；三段欧拉语音随连续普攻衔接
	std::uint64_t time_stop_cast_id = 0;	// 只在成功释放大招后增加，避免重复创建圆形效果
	bool attack_released = false;
	bool special_released = false;
	AttackArea* charged_wind = nullptr;		// 仅作挂点查找，访问前由基类检查是否仍在子弹列表中
	const std::vector<BYTE> summon_opacity = { 0,77,153,217,255,255,89 };
	const std::vector<BYTE> charge_opacity = { 77,153,255,255,255,255,102 };
	const std::vector<int> punch_durations = { 80,40,30,20,20,20,50,30,30,30,30,30,30,40,40,120 };
	const std::vector<int> kick_durations = { 70,40,40,40,40,40,50,20,20,20,60,20,20,30,40,40,40,40,40,150 };
	const std::vector<int> charged_durations = {
		38,25,25,25,31,44,19,19,19,19,19,25,25,25,31,38,13,13,20,20,30,80,30,30,40,50,60,80,40,40,40,50,60,70,40,50,40,50,40,50,40,50,40,50,40,50 };
	const std::vector<int> charged_stand_y = {
		24,28,27,26,25,24,24,24,24,24,24,24,24,24,24,24,23,21,20,18,18,18,18,18,
		18,18,18,18,20,21,22,23,24,24,24,24,24,24,24,24,24,24,24,24,24,24 };
};

#endif // !_JOTARO_PLAYER_H_
