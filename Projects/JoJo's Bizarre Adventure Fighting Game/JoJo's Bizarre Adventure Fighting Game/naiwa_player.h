#ifndef _NAIWA_PLAYER_H_
#define _NAIWA_PLAYER_H_

#include "player.h"
#include "character_stats.h"

extern Atlas atlas_naiwa_idle_left, atlas_naiwa_idle_right;
extern Atlas atlas_naiwa_run_left, atlas_naiwa_run_right;
extern Atlas atlas_naiwa_jump_left, atlas_naiwa_jump_right;
extern Atlas atlas_naiwa_hurt_left, atlas_naiwa_hurt_right;
extern Atlas atlas_naiwa_defeat_left, atlas_naiwa_defeat_right;
extern Atlas atlas_naiwa_teleport_left, atlas_naiwa_teleport_right;
extern Atlas atlas_naiwa_cast_left, atlas_naiwa_cast_right;
extern Atlas atlas_naiwa_phantom_summon_left, atlas_naiwa_phantom_summon_right;
extern Atlas atlas_naiwa_phantom_loop_left, atlas_naiwa_phantom_loop_right;
extern Atlas atlas_naiwa_phantom_disappear_left, atlas_naiwa_phantom_disappear_right;
extern Atlas atlas_naiwa_special_cast_left, atlas_naiwa_special_cast_right;
extern Atlas atlas_doubao_appear_left, atlas_doubao_appear_right;
extern Atlas atlas_doubao_guard_left, atlas_doubao_guard_right;
extern Atlas atlas_doubao_disappear_left, atlas_doubao_disappear_right;
extern Atlas atlas_doubao_hit_left, atlas_doubao_hit_right;
extern Atlas atlas_naiwa_laugh_left, atlas_naiwa_laugh_right;
extern Atlas atlas_naiwa_wave_left, atlas_naiwa_wave_right;

class NaiwaPhantomHit : public AttackArea
{
public:
	explicit NaiwaPhantomHit(bool first_swing, int cycle_ms) : first_swing(first_swing), cycle_ms(cycle_ms) {}
	bool check_collision(const Vector2& target_position, const Vector2& target_size) override
	{
		const int phase = (int)(elapsed_time % cycle_ms) * 1000 / cycle_ms;
		if (first_swing ? (phase < 210 || phase >= 340)
			: (phase < 690 || phase >= 820)) return false;
		return AttackArea::check_collision(target_position, target_size);
	}
private:
	bool first_swing;
	int cycle_ms;
};

class NaiwaSoundwaveHit : public AttackArea
{
public:
	bool check_collision(const Vector2& target_position, const Vector2& target_size) override
	{
		const float center_x = position.x + size.x / 2;
		const float center_y = position.y + size.y / 2;
		const float nearest_x = (std::clamp)(center_x, target_position.x, target_position.x + target_size.x);
		const float nearest_y = (std::clamp)(center_y, target_position.y, target_position.y + target_size.y);
		const float dx = nearest_x - center_x;
		const float dy = nearest_y - center_y;
		const float radius = max_radius * (float)(elapsed_time % cycle_ms) / cycle_ms;
		if (dx * dx + dy * dy > radius * radius) return false;
		return AttackArea::check_collision(target_position, target_size);
	}
	float max_radius = 300;
	int cycle_ms = 560;
	void set_cycle_ms(int value) { cycle_ms = (std::max)(1, value); }
};

class NaiwaPlayer : public Player
{
private:
	struct Phantom
	{
		Vector2 origin;
		bool facing_right;
		int age;
		bool hit_spawned;
		NaiwaPhantomHit* hit_left;
		NaiwaPhantomHit* hit_right;
	};
public:
	NaiwaPlayer()
	{
		animation_idle_left.set_atlas(&atlas_naiwa_idle_left);
		animation_idle_right.set_atlas(&atlas_naiwa_idle_right);
		animation_run_left.set_atlas(&atlas_naiwa_run_left);
		animation_run_right.set_atlas(&atlas_naiwa_run_right);
		animation_jump_left.set_atlas(&atlas_naiwa_jump_left);
		animation_jump_right.set_atlas(&atlas_naiwa_jump_right);
		animation_idle_left.set_interval(100); animation_idle_right.set_interval(100);
		animation_run_left.set_interval(30); animation_run_right.set_interval(30);
		animation_jump_left.set_interval(75); animation_jump_right.set_interval(75);
		animation_jump_left.set_loop(false); animation_jump_right.set_loop(false);
		atlas_hurt_left = &atlas_naiwa_hurt_left;
		atlas_hurt_right = &atlas_naiwa_hurt_right;
		hurt_durations = { 70, 70, 70, 70, 70, 70, 70, 70 };
		for (int duration : hurt_durations) hurt_duration += duration;
		atlas_defeat_left = &atlas_naiwa_defeat_left;
		atlas_defeat_right = &atlas_naiwa_defeat_right;
		defeat_durations = { 100, 100, 110, 160, 150, 150, 150, 280 };
		defeat_offset_x = -32;
		size.x = 72; size.y = 126;
		hp = max_hp = character_stat_int("naiwa", "max_hp", 330);
		mp = character_stat_int("naiwa", "initial_mp", 30);
		run_velocity = (float)character_stat("naiwa", "run_speed", 0.25);
		jump_velocity = -(float)character_stat("naiwa", "jump_speed", 0.85);
		normal_protection_threshold = character_stat_int("naiwa", "normal_protection_threshold", 20);
		special_mp_cost = character_stat_int("naiwa", "special_cost", 30);
		ultimate_mp_cost = character_stat_int("naiwa", "ultimate_cost", 100);
		attack_cd = character_stat_int("naiwa", "normal_cooldown_ms", 350);
		timer_attack_cd.set_wait_time(attack_cd);
	}

	void on_update(int delta) override
	{
		Player::on_update(delta);
		delta = active_update_delta;
		if (hp <= 0) { clear_phantoms(); cancel_wave(); guard_active = false; return; }
		if (delta <= 0) return;

		if (teleport_elapsed >= 0)
		{
			teleport_elapsed += delta;
			if (!teleport_moved && teleport_elapsed >= teleport_cast_ms / 2)
			{
				const float distance = (float)character_stat("naiwa", "teleport_distance_px", 256);
				position.x = (std::max)(0.0f, (std::min)(position.x + (is_facing_right ? distance : -distance),
					(float)GAME_WORLD_WIDTH - sprite_width));
				teleport_moved = true;
			}
			if (teleport_elapsed >= teleport_cast_ms) { teleport_elapsed = -1; is_attacking_ex = false; }
		}

		if (is_attacking)
		{
			cast_elapsed += delta;
			if (cast_elapsed >= normal_cast_ms)
			{
				is_attacking = false;
				phantoms.push_back({ position, is_facing_right, 0, false, nullptr, nullptr });
			}
		}
		const int active_ms = character_stat_int("naiwa", "phantom_duration_ms", 5000);
		for (Phantom& phantom : phantoms)
		{
			phantom.age += delta;
			if (!phantom.hit_spawned && phantom.age >= phantom_summon_ms)
			{
				phantom.hit_spawned = true;
				phantom.hit_left = create_phantom_hit(phantom, false, active_ms);
				phantom.hit_right = create_phantom_hit(phantom, true, active_ms);
			}
		}
		phantoms.erase(std::remove_if(phantoms.begin(), phantoms.end(),
			[this, active_ms](const Phantom& phantom) { return phantom.age >= phantom_summon_ms + active_ms + 480; }), phantoms.end());

		if (guard_cast_elapsed >= 0)
		{
			guard_cast_elapsed += delta;
			if (guard_cast_elapsed >= guard_activation_ms && !guard_started)
			{
				guard_started = true;
				guard_active = true;
				guard_damage_remainder = 0;
				guard_remaining = character_stat_int("naiwa", "guard_duration_ms", 8000);
			}
			if (guard_cast_elapsed >= special_cast_ms) { guard_cast_elapsed = -1; guard_elapsed = 0; is_attacking_ex = false; }
		}
		if (guard_active)
		{
			guard_elapsed += delta;
			guard_remaining = (std::max)(0, guard_remaining - delta);
			if (guard_remaining == 0)
			{
				guard_active = false;
				guard_disappear_elapsed = 0;
			}
		}
		if (guard_disappear_elapsed >= 0)
		{
			guard_disappear_elapsed += delta;
			if (guard_disappear_elapsed >= 640) guard_disappear_elapsed = -1;
		}
		if (guard_hit_elapsed >= 0)
		{
			guard_hit_elapsed += delta;
			if (guard_hit_elapsed >= 400) guard_hit_elapsed = -1;
		}

		if (is_attacking_exxxx)
		{
			ultimate_elapsed += delta;
			if (!wave_spawned && ultimate_elapsed >= ultimate_startup_ms)
			{
				wave_spawned = true;
				const int active_ms = character_stat_int("naiwa", "ultimate_wave_duration_ms", 2240);
				auto* hit = new NaiwaSoundwaveHit();
				hit->set_cycle_ms(ultimate_wave_cycle_ms);
				hit->max_radius = (float)character_stat("naiwa", "ultimate_radius_px", 500);
				hit->set_attack_kind(Bullet::AttackKind::Ultimate);
				hit->set_hit_shake(10.0f, 180);
				hit->set_size(hit->max_radius * 2, hit->max_radius * 2);
				hit->set_damage(character_stat_int("naiwa", "ultimate_damage_per_hit", 9));
				hit->set_transform_duration(character_stat_int("naiwa", "transform_duration_ms", 7000));
				hit->set_duration(active_ms);
				hit->set_hit_interval(ultimate_wave_cycle_ms);
				register_bullet(hit);
				wave_hit = hit;
				wave_center = Vector2(position.x + sprite_width / 2, position.y + 64);
				hit->set_position(wave_center.x - hit->max_radius, wave_center.y - hit->max_radius);
			}
			if (ultimate_elapsed >= ultimate_startup_ms + character_stat_int("naiwa", "ultimate_wave_duration_ms", 2240) + ultimate_recovery_ms)
			{ is_attacking_exxxx = false; is_attacking_ex = false; }
		}
	}

	void on_draw(const Camera& camera) override
	{
		if (get_transformed() || hp <= 0) { Player::on_draw(camera); return; }
		if (guard_active || guard_cast_elapsed >= guard_activation_ms || guard_disappear_elapsed >= 0)
		{
			Atlas* left = &atlas_doubao_guard_left;
			Atlas* right = &atlas_doubao_guard_right;
			int frame = (int)(guard_elapsed / 100) % 8;
			if (guard_cast_elapsed >= guard_activation_ms && guard_cast_elapsed < special_cast_ms)
			{ left = &atlas_doubao_appear_left; right = &atlas_doubao_appear_right; frame = (std::min)(7, (guard_cast_elapsed - guard_activation_ms) * 8 / (std::max)(1, special_cast_ms - guard_activation_ms)); }
			else if (guard_disappear_elapsed >= 0)
			{ left = &atlas_doubao_disappear_left; right = &atlas_doubao_disappear_right; frame = (std::min)(7, guard_disappear_elapsed / 80); }
			draw_frame(camera, *left, *right, frame, is_facing_right ? -88.0f : 24.0f, -65);
		}
		for (const Phantom& phantom : phantoms)
		{
			Atlas* left = &atlas_naiwa_phantom_loop_left;
			Atlas* right = &atlas_naiwa_phantom_loop_right;
			int frame = 0;
			const int active_ms = character_stat_int("naiwa", "phantom_duration_ms", 5000);
			if (phantom.age < phantom_summon_ms)
			{ left = &atlas_naiwa_phantom_summon_left; right = &atlas_naiwa_phantom_summon_right; frame = (std::min)(7, phantom.age * 8 / phantom_summon_ms); }
			else if (phantom.age < phantom_summon_ms + active_ms)
			{ frame = (int)(((phantom.age - phantom_summon_ms) % phantom_attack_cycle_ms) * 8 / phantom_attack_cycle_ms); }
			else
			{ left = &atlas_naiwa_phantom_disappear_left; right = &atlas_naiwa_phantom_disappear_right; frame = (std::min)(7, (phantom.age - phantom_summon_ms - active_ms) / 60); }
			Atlas* atlas = phantom.facing_right ? right : left;
			putimage_alpha(camera, (int)(phantom.origin.x - 16), (int)(phantom.origin.y - 33), atlas->get_image(frame));
		}
		if (teleport_elapsed >= 0)
			draw_frame(camera, atlas_naiwa_teleport_left, atlas_naiwa_teleport_right,
				(std::min)(7, teleport_elapsed * 8 / teleport_cast_ms));
		else if (is_attacking && !get_hurt_active())
			draw_frame(camera, atlas_naiwa_cast_left, atlas_naiwa_cast_right,
				(std::min)(7, cast_elapsed * 8 / normal_cast_ms));
		else if (guard_cast_elapsed >= 0 && !get_hurt_active())
			draw_frame(camera, atlas_naiwa_special_cast_left, atlas_naiwa_special_cast_right,
				(std::min)(7, guard_cast_elapsed * 8 / special_cast_ms));
		else if (is_attacking_exxxx && !get_hurt_active())
			draw_frame(camera, atlas_naiwa_laugh_left, atlas_naiwa_laugh_right,
				ultimate_elapsed < ultimate_startup_ms ? (std::min)(2, ultimate_elapsed * 3 / (std::max)(1, ultimate_startup_ms)) : 3 + ((ultimate_elapsed - ultimate_startup_ms) / 110) % 3);
		else Player::on_draw(camera);
		if (guard_hit_elapsed >= 0)
			draw_frame(camera, atlas_doubao_hit_left, atlas_doubao_hit_right,
				(std::min)(7, guard_hit_elapsed / 50), -16, -16);
	}
	void on_draw_background_effects(const Camera& camera) const override
	{
		if (!is_attacking_exxxx || !wave_spawned || hp <= 0) return;
		const int elapsed = ultimate_elapsed - ultimate_startup_ms;
		if (elapsed < 0 || elapsed >= character_stat_int("naiwa", "ultimate_wave_duration_ms", 2240)) return;
		const int frame = (elapsed % ultimate_wave_cycle_ms) * 8 / ultimate_wave_cycle_ms;
		Atlas& atlas = is_facing_right ? atlas_naiwa_wave_right : atlas_naiwa_wave_left;
		IMAGE* image = atlas.get_image(frame);
		if (image)
			putimage_alpha(camera, (int)(wave_center.x - image->getwidth() / 2),
				(int)(wave_center.y - image->getheight() / 2), image);
	}

	void on_attack() override
	{
		if ((int)phantoms.size() >= character_stat_int("naiwa", "phantom_max_count", 5)) return;
		if (!begin_normal_attack()) return;
		cast_elapsed = 0;
	}

	void on_attack_ex() override
	{
		if (guard_active) return;
		if (!begin_special_attack()) return;
		add_mp(character_stat_int("naiwa", "special_mp_return", 10));
		guard_cast_elapsed = 0;
		guard_started = false;
		guard_elapsed = 0;
		guard_disappear_elapsed = -1;
	}

	void on_attack_exxxx() override
	{
		if (!begin_ultimate_attack()) return;
		add_mp(character_stat_int("naiwa", "ultimate_mp_return", 10));
		ultimate_elapsed = 0;
		wave_spawned = false;
		is_attacking_ex = true;
	}

	void on_block_key_down() override
	{
		if (teleport_key_down) return;
		teleport_key_down = true;
		if (hp <= 0 || get_hit_stunned() || get_time_stop_remaining() > 0
			|| is_attacking || is_attacking_ex || is_attacking_exxxx) return;
		const int cost = character_stat_int("naiwa", "teleport_cost", 5);
		if (mp < cost) return;
		add_mp(-cost);
		teleport_elapsed = 0;
		teleport_moved = false;
		is_attacking_ex = true;
	}
	void on_block_key_up() override { teleport_key_down = false; }
	void on_block_key_up_while_stopped() override { teleport_key_down = false; }
	int modify_incoming_damage(int damage) override
	{
		if (!guard_active || damage <= 0) return damage;
		guard_hit_elapsed = 0;
		main_camera.shake(4.0f, 110);
		const int reduction = (std::clamp)(character_stat_int("naiwa", "guard_reduction_percent", 70), 0, 100);
		const int numerator = damage * (100 - reduction) + guard_damage_remainder;
		guard_damage_remainder = numerator % 100;
		return numerator / 100;
	}
	void on_damage_taken(int damage) override
	{
		add_mp(character_stat_int("naiwa", "passive_mp_per_hit", 2));
		if (guard_active) guard_hit_elapsed = 0;
	}
	void on_transformed() override
	{
		guard_active = false;
		guard_started = false;
		guard_cast_elapsed = guard_disappear_elapsed = guard_hit_elapsed = -1;
		guard_remaining = 0;
		clear_phantoms();
		cancel_wave();
		teleport_elapsed = -1;
		wave_spawned = false;
	}
	void on_match_finished() override
	{
		guard_active = false;
		guard_cast_elapsed = guard_disappear_elapsed = guard_hit_elapsed = -1;
		clear_phantoms();
		cancel_wave();
	}

private:
	NaiwaPhantomHit* create_phantom_hit(const Phantom& phantom, bool right, int duration)
	{
		auto* hit = new NaiwaPhantomHit(right == phantom.facing_right, phantom_attack_cycle_ms);
		hit->set_attack_kind(Bullet::AttackKind::Normal);
		hit->set_size(130, 100);
		hit->set_damage(character_stat_int("naiwa", "normal_damage", 10));
		hit->set_duration(duration);
		hit->set_hit_interval((std::max)(1, phantom_attack_cycle_ms * 950 / 1000));
		const float center = phantom.origin.x + sprite_width / 2;
		hit->set_position(right ? center : center - 130, phantom.origin.y + 20);
		register_bullet(hit, character_stat_int("naiwa", "normal_mp_gain", 5));
		return hit;
	}
	void clear_phantoms()
	{
		for (const Phantom& phantom : phantoms)
			for (NaiwaPhantomHit* hit : { phantom.hit_left, phantom.hit_right })
				if (hit && std::find(bullet_list.begin(), bullet_list.end(), hit) != bullet_list.end()) hit->remove();
		phantoms.clear();
	}
	void cancel_wave()
	{
		if (wave_hit && std::find(bullet_list.begin(), bullet_list.end(), wave_hit) != bullet_list.end()) wave_hit->remove();
		wave_hit = nullptr;
		wave_spawned = false;
	}
	int teleport_elapsed = -1;
	bool teleport_moved = false, teleport_key_down = false;
	int cast_elapsed = 0;
	std::vector<Phantom> phantoms;
	int guard_cast_elapsed = -1, guard_elapsed = 0, guard_disappear_elapsed = -1, guard_hit_elapsed = -1;
	int guard_remaining = 0;
	int guard_damage_remainder = 0;
	bool guard_active = false, guard_started = false;
	int ultimate_elapsed = 0;
	bool wave_spawned = false;
	NaiwaSoundwaveHit* wave_hit = nullptr;
	Vector2 wave_center;
	const int normal_cast_ms = character_duration_ms("naiwa", "normal_cast_ms", 480);
	const int special_cast_ms = character_duration_ms("naiwa", "special_cast_ms", 720);
	const int guard_activation_ms = (std::max)(1, special_cast_ms * 300 / 720);
	const int teleport_cast_ms = character_duration_ms("naiwa", "teleport_cast_ms", 360);
	const int ultimate_startup_ms = character_duration_ms("naiwa", "ultimate_startup_ms", 270, true);
	const int ultimate_recovery_ms = character_duration_ms("naiwa", "ultimate_recovery_ms", 180, true);
	const int phantom_summon_ms = character_duration_ms("naiwa", "phantom_summon_ms", 500);
	const int phantom_attack_cycle_ms = character_duration_ms("naiwa", "phantom_attack_cycle_ms", 1000);
	const int ultimate_wave_cycle_ms = character_duration_ms("naiwa", "ultimate_wave_cycle_ms", 560);
};

#endif
