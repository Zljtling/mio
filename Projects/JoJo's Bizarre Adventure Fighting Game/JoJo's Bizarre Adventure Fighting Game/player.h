#ifndef _PLAYER_H_
#define _PLAYER_H_

#include "camera.h"
#include "vector2.h"
#include "platform.h"
#include "particle.h"
#include "animation.h"
#include "player_id.h"
#include "attack.h"
#include "audio.h"
#include "character_stats.h"
#include "timer.h"

#include <algorithm>
#include <memory>
#include <vector>
#include <cstdint>
#include <graphics.h>

extern bool is_debug;

class Player;
extern Player* player_1;
extern Player* player_2;
extern std::vector<Bullet*> bullet_list;
extern std::vector<Platform> platform_list;
extern Atlas atlas_run_effect;
extern Atlas atlas_land_effect;
extern Atlas atlas_naiwa_plain_idle_left, atlas_naiwa_plain_idle_right;
extern Atlas atlas_naiwa_plain_run_left, atlas_naiwa_plain_run_right;
extern Atlas atlas_naiwa_transform_left, atlas_naiwa_transform_right;
extern Atlas atlas_naiwa_revert_left, atlas_naiwa_revert_right;

class Player
{
public:
	Player()
	{
		current_animation = &animation_idle_right;
		timer_attack_cd.set_wait_time(attack_cd);
		timer_attack_cd.set_one_shot(true);
		timer_attack_cd.pause();
		timer_attack_cd.set_callback([this]()
			{
				can_attack = true;
				timer_attack_cd.pause();
			});

		timer_invulnerable.set_wait_time(300);
		timer_invulnerable.set_one_shot(true);
		timer_invulnerable.set_callback([&]()
			{

				is_invulnerable = false;
			});

		timer_invulnerable_blink.set_wait_time(75);
		timer_invulnerable_blink.set_callback([&]()
			{
				is_showing_sketch_frame = !is_showing_sketch_frame;
			});

		timer_run_effect_generation.set_wait_time(75);
		timer_run_effect_generation.pause();
		timer_run_effect_generation.set_callback([this]()
			{
				float offset_x = (is_left_key_down - is_right_key_down) * 28.0f;
				emit_foot_particle(atlas_run_effect, 45, 0, offset_x);
				// 烟尘落在移动方向后方；大招锁定朝向时，后退移动也能保持正确方向
			});

		timer_die_effect_generation.set_wait_time(death_impact_time);
		timer_die_effect_generation.set_one_shot(true);
		timer_die_effect_generation.pause();
		timer_die_effect_generation.set_callback([this]()
			{
				emit_foot_particle(atlas_land_effect, 60, (int)(death_elapsed - death_impact_time));
				timer_die_effect_generation.pause();	// 倒地撞击只产生一次烟尘
			});
	}

	virtual ~Player()
	{
		stop_player_attack_sounds(id);
		callback_lifetime.reset();	// 玩家销毁后，仍在飞行的子弹不能访问此玩家
		clear_attached_attacks();
	}

	Player(const Player&) = delete;				// 定时器和命中回调绑定当前玩家，不允许复制
	Player& operator=(const Player&) = delete;

	virtual void on_update(int delta)
	{
		active_update_delta = 0;
		if (delta <= 0)
			return;
		if (transformed_remaining > 0)
		{
			transformed_intro_elapsed = (std::min)(400, transformed_intro_elapsed + delta);
			transformed_remaining = (std::max)(0, transformed_remaining - delta);
			if (transformed_remaining == 0) on_transform_ended();
		}

		if (hp <= 0)
		{
			if (!is_dead)
				begin_death();
			update_death(delta);
			return;					// 倒地时不再推进派生角色的攻击和技能
		}

		normal_combo_remaining = (std::max)(0, normal_combo_remaining - delta);
		if (normal_combo_remaining == 0) normal_combo_damage = 0; // 实际时间一秒未命中即断连
		timer_invulnerable.on_update(delta);
		timer_invulnerable_blink.on_update(delta);	// 受击保护按实际时间推进，时停不阻止伤害结算

		// 时停期间暂停物理、动画、冷却和技能进度，输入冻结由场景同步。
		int stopped_remaining = get_time_stop_remaining();
		int frozen_delta = delta < stopped_remaining ? delta : stopped_remaining;
		time_stop_remaining = (std::max)(0, time_stop_remaining - frozen_delta);
		time_stop_wave_remaining = (std::max)(0, time_stop_wave_remaining - frozen_delta);
		delta -= frozen_delta;
		update_time_stop_audio(delta <= 0); // 技能声音与冻结的动画一起暂停，解冻后续播
		if (delta <= 0)
		{
			check_bullet_collisions();	// 身体被冻结时，仍可被欧拉拳和其他攻击命中
			if (hp <= 0)
				begin_death();
			return;
		}
		active_update_delta = delta;	// 派生角色也只能推进未被冻结的时间
		update_particles(delta);
		if (hurt_pending) { hurt_pending = false; hurt_active = true; hurt_elapsed = 0; interrupt_normal_attack(); }
		else if (hurt_active)
		{
			hurt_elapsed += delta;
			if (hurt_elapsed >= hurt_duration) hurt_active = false;
		}
		const int movement_delta = update_hit_stun(delta); // 僵直和击退只推进未被时停冻结的时间
		update_block(delta);

		int direction = is_right_key_down - is_left_key_down;
		if (is_attacking_ex || get_block_animation_active() || is_match_finished || movement_delta == 0)
			direction = 0;			// 特殊攻击期间禁止水平移动和转向
		if (direction != 0)
		{
			if (!is_facing_locked)
				is_facing_right = direction > 0;
			const float speed = transformed_remaining > 0
				? (float)character_stat("naiwa", "transformed_run_speed", 0.15) : run_velocity;
			on_run(direction * speed * movement_delta);
		}

		bool was_grounded = is_grounded;
		timer_attack_cd.on_update(delta);
		move_and_collide(delta);

		if (hp <= 0)
		{
			begin_death();
			return;					// 致命命中当帧切换姿势，取消尚未发出的攻击
		}

		if (was_grounded && !is_grounded)
		{
			animation_jump_left.reset();
			animation_jump_right.reset();
		}

		if (!is_grounded)
		{
			current_animation = is_facing_right ? &animation_jump_right : &animation_jump_left;
			animation_jump_left.on_update(delta);
			animation_jump_right.on_update(delta);
		}
		else
		{
			current_animation = direction != 0
				? (is_facing_right ? &animation_run_right : &animation_run_left)
				: (is_facing_right ? &animation_idle_right : &animation_idle_left);
			current_animation->on_update(delta);
		}

		if (is_grounded && direction != 0)
		{
			timer_run_effect_generation.resume();
			timer_run_effect_generation.on_update(delta);	// 在物理更新后，以当前脚底坐标生成烟尘
		}
		else
		{
			timer_run_effect_generation.pause();
			timer_run_effect_generation.restart();		// 停跑和离地后不保留上次发射余量
		}
		update_attached_attacks(delta);
		if (transformed_remaining > 0)
		{
			Animation& animation = direction == 0
				? (is_facing_right ? transformed_idle_right : transformed_idle_left)
				: (is_facing_right ? transformed_run_right : transformed_run_left);
			animation.on_update(delta);
		}
	}

	virtual void on_draw(const Camera& camera)
	{
		on_draw_particles(camera);
		if (transformed_remaining > 0 && hp > 0)
		{
			if (transformed_intro_elapsed < 400)
			{
				draw_frame(camera, atlas_naiwa_transform_left, atlas_naiwa_transform_right,
					(std::min)(7, transformed_intro_elapsed / 50));
				return;
			}
			if (transformed_remaining <= 400)
			{
				draw_frame(camera, atlas_naiwa_revert_left, atlas_naiwa_revert_right,
					(std::min)(7, (400 - transformed_remaining) / 50));
				return;
			}
			Animation& animation = (is_left_key_down != is_right_key_down)
				? (is_facing_right ? transformed_run_right : transformed_run_left)
				: (is_facing_right ? transformed_idle_right : transformed_idle_left);
			animation.on_draw(camera, (int)position.x, (int)position.y);
			return;
		}

		if (is_dead && atlas_defeat_left && atlas_defeat_right)
		{
			draw_frame(camera, *atlas_defeat_left, *atlas_defeat_right,
				get_frame_index(death_elapsed, defeat_durations), defeat_offset_x);
			return;				// 宽画布对齐站立帧，保持角色脚底位置不变
		}
		if (get_block_animation_active() && atlas_block_left && atlas_block_right)
		{
			draw_frame(camera, *atlas_block_left, *atlas_block_right, block_frame);
			return;				// 防御受击保持盾姿，不使用待机剪影覆盖防御帧
		}
		if (hurt_active && atlas_hurt_left && atlas_hurt_right)
		{
			draw_frame(camera, *atlas_hurt_left, *atlas_hurt_right, get_frame_index(hurt_elapsed, hurt_durations));
			return; // 未格挡命中播放受击；技能判定和替身动画继续按原规则运行
		}
		if (hp > 0 && is_invulnerable && is_showing_sketch_frame)
		{
			sketch_image(current_animation->get_frame(), &img_sketch);
			putimage_alpha(camera, (int)position.x, (int)position.y, &img_sketch);
		}
		else
			current_animation->on_draw(camera, (int)position.x, (int)position.y);
	}
	virtual void on_draw_background_effects(const Camera& camera) const {}

	virtual void on_draw_debug(const Camera& camera) const
	{
		if (!is_debug)
			return;
		DebugDrawState draw_state;
		Vector2 collision_position = get_collision_position();
		COLORREF color = get_invulnerable() ? RGB(180, 110, 255) : RGB(0, 125, 255);
		setlinecolor(color);
		settextcolor(color);
		rectangle(camera, (int)collision_position.x, (int)collision_position.y,
			(int)(collision_position.x + size.x), (int)(collision_position.y + size.y));
		TCHAR text[96];
		_stprintf_s(text, _T("P%d HP:%d MP:%d%s%s%s"), id == PlayerID::P1 ? 1 : 2, hp, mp,
			get_invulnerable() ? _T(" 无敌") : _T(""),
			get_time_stop_remaining() > 0 ? _T(" 时停") : _T(""),
			get_hit_stunned() ? _T(" 僵直") : _T(""));
		const Vector2& camera_position = camera.get_position();
		outtextxy((int)(collision_position.x - camera_position.x),
			(int)(collision_position.y - camera_position.y) - 36, text);
	}

	virtual void on_input(const ExMessage& msg)
	{
		if (hp <= 0 || is_match_finished)
			return;				// 结算或倒地后不再接受移动、跳跃和攻击输入
		bool is_p1 = id == PlayerID::P1;
		BYTE key_left = is_p1 ? 'A' : VK_LEFT;
		BYTE key_right = is_p1 ? 'D' : VK_RIGHT;
		BYTE key_jump = is_p1 ? 'W' : VK_UP;
		BYTE key_block = is_p1 ? 'S' : VK_DOWN;	// 长按防御，松开后播放收手
		BYTE key_attack = is_p1 ? 'F' : VK_OEM_COMMA;		// 普通攻击
		BYTE key_special = is_p1 ? 'E' : VK_OEM_PERIOD;		// 特殊攻击
		BYTE key_ultimate = is_p1 ? 'Q' : VK_OEM_2;		// 大招

		if ((get_time_stop_remaining() > 0 || get_hit_stunned())
			&& (msg.message == WM_KEYDOWN || msg.message == WM_SYSKEYDOWN))
			return;				// 时停或僵直时不接受新操作，松键消息仍正常处理

		if (msg.message == WM_KEYDOWN || msg.message == WM_SYSKEYDOWN)
		{
			if (transformed_remaining > 0 && msg.vkcode != key_left
				&& msg.vkcode != key_right && msg.vkcode != key_jump) return;
			if (msg.vkcode == key_block)
			{
				is_block_key_down = true;
				on_block_key_down();
				return;
			}
			if (get_block_animation_active())
				return;			// 举手、保持、受击及收手期间都不接受其他动作
			if (msg.vkcode == key_left)
				is_left_key_down = true;
			else if (msg.vkcode == key_right)
				is_right_key_down = true;
			else if (msg.vkcode == key_jump)
				on_jump();
			else if (msg.vkcode == key_attack)
				on_attack();
			else if (msg.vkcode == key_special)
			{
				if (!is_special_key_down)
					on_attack_ex();
				is_special_key_down = true;		// 忽略长按产生的重复技能消息
			}
			else if (msg.vkcode == key_ultimate)
			{
				if (!is_ultimate_key_down)
					on_attack_exxxx();
				is_ultimate_key_down = true;
			}
		}
		else if (msg.message == WM_KEYUP || msg.message == WM_SYSKEYUP)
		{
			if (msg.vkcode == key_left)
				is_left_key_down = false;
			else if (msg.vkcode == key_right)
				is_right_key_down = false;
			else if (msg.vkcode == key_block)
			{
				is_block_key_down = false;
				if (get_time_stop_remaining() == 0) on_block_key_up();
				else on_block_key_up_while_stopped();
			}
			else if (msg.vkcode == key_special)
				is_special_key_down = false;
			else if (msg.vkcode == key_ultimate)
				is_ultimate_key_down = false;
		}
	}

	void set_id(PlayerID id) { this->id = id; }
	PlayerID get_id() const { return id; }
	void set_position(float x, float y) { position.x = x, position.y = y; }
	const Vector2& get_position() const { return position; }
	const Vector2& get_size() const { return size; }
	Vector2 get_collision_position() const
	{
		return Vector2(position.x + (sprite_width - size.x) / 2, position.y);
	}

	bool can_be_hit_by(const Bullet& bullet) const
	{
		return hp > 0 && !is_match_finished && bullet.get_valid() && !bullet.check_can_remove()
			&& !get_ultimate_invulnerable() && !bullet.get_time_stopped()
			&& (!is_invulnerable || bullet.get_attack_kind() != Bullet::AttackKind::Normal);
		// 时停大招无敌阻止所有攻击；普通 300ms 保护仍只阻止普攻
	}

	void set_hp(int val) 
	{
		hp = val;
	}

	int get_hp() const
	{
		return hp;
	}

	int get_max_hp() const { return max_hp; }
	int get_special_cost() const { return special_mp_cost; }
	int get_ultimate_cost() const { return ultimate_mp_cost; }
	bool can_release_ultimate() const
	{
		return hp > 0 && !is_match_finished && transformed_remaining == 0 && !get_hit_stunned()
			&& !get_block_animation_active() && get_time_stop_remaining() == 0
			&& !is_attacking && !is_attacking_ex && !is_attacking_exxxx && mp >= ultimate_mp_cost;
	}
	bool get_transformed() const { return transformed_remaining > 0; }
	void apply_naiwa_transform(int duration)
	{
		if (hp <= 0 || duration <= 0) return;
		const bool first_hit = transformed_remaining == 0;
		if (first_hit)
		{
			transformed_idle_left.set_atlas(&atlas_naiwa_plain_idle_left);
			transformed_idle_right.set_atlas(&atlas_naiwa_plain_idle_right);
			transformed_run_left.set_atlas(&atlas_naiwa_plain_run_left);
			transformed_run_right.set_atlas(&atlas_naiwa_plain_run_right);
			transformed_idle_left.set_interval(100); transformed_idle_right.set_interval(100);
			transformed_run_left.set_interval(80); transformed_run_right.set_interval(80);
		}
		transformed_remaining = duration;
		if (!first_hit) return;
		transformed_intro_elapsed = 0;
		is_attacking = is_attacking_ex = is_attacking_exxxx = false;
		is_facing_locked = false;
		is_special_key_down = is_ultimate_key_down = is_block_key_down = false;
		block_state = BlockState::None;
		clear_attached_attacks();
		transformed_idle_left.reset(); transformed_idle_right.reset();
		transformed_run_left.reset(); transformed_run_right.reset();
		on_transformed();
	}
	bool get_hurt_active() const { return hurt_active; }
	bool get_hit_stunned() const { return hit_stun_remaining > 0; }
	int get_hit_stun_remaining() const { return hit_stun_remaining; }

	int get_mp() const
	{
		return mp;
	}

	bool get_blocking() const { return block_state != BlockState::None && block_state != BlockState::Releasing; }
	bool get_block_animation_active() const { return block_state != BlockState::None; }
	int get_block_frame() const { return block_frame; }

	// 结算时停止攻击和操作，保留重力、待机、倒地及已有粒子动画。
	void finish_match()
	{
		stop_player_attack_sounds(id);
		time_stop_audio_mask = 0;
		is_attack_audio_paused = false;
		is_match_finished = true;
		is_attacking = is_attacking_ex = is_attacking_exxxx = false;
		is_facing_locked = false;
		is_left_key_down = is_right_key_down = false;
		is_special_key_down = is_ultimate_key_down = is_block_key_down = false;
		block_state = BlockState::None;
		hurt_active = hurt_pending = false;
		hit_stun_remaining = knockback_remaining = 0;
		knockback_velocity = 0;
		velocity.x = 0;
		clear_attached_attacks();
		on_match_finished();
		if (hp <= 0 && !is_dead) begin_death();
	}

	virtual bool get_ultimate_invulnerable() const { return false; } // 派生角色单独提供大招无敌，技能不能绕过
	bool get_invulnerable() const { return is_invulnerable || get_ultimate_invulnerable(); }
	bool get_facing_right() const { return is_facing_right; }
	bool get_attacking() const { return is_attacking; }
	bool get_attacking_ex() const { return is_attacking_ex; }
	bool get_attacking_exxxx() const { return is_attacking_exxxx; }
	int get_time_stop_remaining() const
	{
		return (std::max)(time_stop_remaining, time_stop_wave_remaining);
	}
	virtual std::uint64_t get_time_stop_cast_id() const { return 0; }
	virtual int get_time_stop_cast_remaining() const { return 0; }

	void set_mp(int value)
	{
		mp = value < 0 ? 0 : (value > 100 ? 100 : value);
	}

	void add_mp(int value)
	{
		std::int64_t result = (std::int64_t)mp + value;
		mp = result < 0 ? 0 : (result > 100 ? 100 : (int)result);
	}

	void set_time_stopped(int duration)
	{
		if (duration > time_stop_remaining)
			time_stop_remaining = duration;		// 重复设置不缩短已有时停
	}

	void set_time_stop_wave_remaining(int duration)
	{
		time_stop_wave_remaining = (std::max)(0, duration);
		if (time_stop_wave_remaining > 0)
		{
			is_left_key_down = is_right_key_down = false;
			is_special_key_down = is_ultimate_key_down = false;
		}						// 被扩散圆捕获后清除按键，避免解冻时沿用旧操作
	}

	virtual void on_run(float distance)
	{
		if (hp <= 0 || is_match_finished || get_hit_stunned() || get_block_animation_active() || is_attacking_ex || get_time_stop_remaining() > 0)
			return;
		
		position.x += distance;
	}

	virtual void on_jump()
	{
		if (hp <= 0 || is_match_finished || get_hit_stunned() || get_block_animation_active() || !is_grounded || is_attacking_ex || get_time_stop_remaining() > 0)
			return;
		velocity.y = jump_velocity;
		is_grounded = false;
		animation_jump_left.reset();
		animation_jump_right.reset();
	}

	virtual void on_attack() {}				// 普通攻击，由角色决定发射物或近战区域
	virtual void on_kick() {}				// 踢击，仅承太郎实现
	virtual void on_attack_ex() {}			// 特殊攻击
	virtual void on_attack_exxxx() {}		// 大招
	virtual void on_block_key_down() { begin_block(); }
	virtual void on_block_key_up() { end_block(); }
	virtual void on_block_key_up_while_stopped() {}
	virtual void on_match_finished() {}
	virtual int modify_incoming_damage(int damage) { return damage; }
	virtual void on_damage_taken(int damage) {}
	virtual void on_transformed() {}
	virtual void on_transform_ended() {}

	void make_invulnerable()
	{
		is_invulnerable = true;
		timer_invulnerable.restart();
	}


protected:
	void on_draw_particles(const Camera& camera) const
	{
		for (const Particle& particle : particle_list)
			particle.on_draw(camera);
	}

	bool begin_normal_attack()
	{
		if (hp <= 0 || is_match_finished || transformed_remaining > 0 || get_hit_stunned() || get_block_animation_active() || get_time_stop_remaining() > 0 || !can_attack || is_attacking || is_attacking_ex
			|| (is_attacking_exxxx && !allow_attack_during_ultimate))
			return false;
		hurt_active = hurt_pending = false;	// 主动发动新攻击后切回攻击姿势
		is_attacking = true;
		can_attack = false;
		timer_attack_cd.restart();
		timer_attack_cd.resume();			// 仅成功发动攻击时开始冷却
		return true;
	}

	bool begin_special_attack()
	{
		if (hp <= 0 || is_match_finished || transformed_remaining > 0 || get_hit_stunned() || get_block_animation_active() || get_time_stop_remaining() > 0 || is_attacking || is_attacking_ex || is_attacking_exxxx
			|| mp < special_mp_cost)
			return false;
		mp -= special_mp_cost;				// 消耗按角色设置，不依赖 1P / 2P
		hurt_active = hurt_pending = false;
		is_attacking_ex = true;
		return true;
	}

	bool begin_ultimate_attack()
	{
		if (!can_release_ultimate())
			return false;
		add_mp(-ultimate_mp_cost);
		hurt_active = hurt_pending = false;
		is_attacking_exxxx = true;
		return true;
	}

	Player* get_opponent() const
	{
		return id == PlayerID::P1 ? player_2 : player_1;
	}

	void register_bullet(Bullet* bullet, int mp_gain = 0)
	{
		bullet->set_source_id(id);
		bullet->set_knockback_direction(is_facing_right ? 1 : -1); // 发射时锁定击退方向，不随攻击者之后转身改变
		if (bullet->get_attack_kind() == Bullet::AttackKind::Normal)
			bullet->set_normal_protection_threshold(normal_protection_threshold);
		bullet->set_collide_target(id == PlayerID::P1 ? PlayerID::P2 : PlayerID::P1);
		std::weak_ptr<int> lifetime = callback_lifetime;
		bullet->set_callback([this, lifetime, mp_gain]()
			{
				if (!lifetime.expired() && mp_gain > 0)
					add_mp(mp_gain);			// 命中回能，最多 100 点
			});
		bullet_list.push_back(bullet);		// 子弹更新、碰撞和删除仍由场景阶段接入
	}

	// 近战和持续技能跟随角色；offset 是朝右时相对 128px 角色画布的位置。
	void attach_attack(AttackArea* area, float x, float y, int remaining, bool follow_facing = true)
	{
		prune_attached_attacks();
		attached_attacks.push_back({ area, Vector2(x, y), remaining, follow_facing, is_facing_right });
		position_attached_attack(attached_attacks.back());
	}

	void set_attached_offset(AttackArea* area, float x, float y)
	{
		prune_attached_attacks();
		for (AttachedAttack& attack : attached_attacks)
		{
			if (attack.area == area)
			{
				attack.offset = Vector2(x, y);
				position_attached_attack(attack);
				break;
			}
		}
	}

	void clear_attached_attacks()
	{
		for (const AttachedAttack& attack : attached_attacks)
		{
			if (has_bullet(attack.area))
				attack.area->remove();		// 不在这里 delete，避免与场景回收重复释放
		}
		attached_attacks.clear();
	}

	int get_frame_index(std::int64_t elapsed, const std::vector<int>& durations) const
	{
		for (int i = 0; i < (int)durations.size(); i++)
		{
			if (elapsed < durations[i])
				return i;
			elapsed -= durations[i];
		}
		return durations.empty() ? 0 : (int)durations.size() - 1;
	}

	void draw_frame(const Camera& camera, Atlas& left, Atlas& right, int index,
		float x = 0, float y = 0) const
	{
		Atlas& atlas = is_facing_right ? right : left;
		if (atlas.get_size() <= 0)
			return;
		index = index < atlas.get_size() ? index : atlas.get_size() - 1;
		IMAGE* image = atlas.get_image(index);
		if (image)
			draw_image(camera, image, x, y);
	}

	void draw_image(const Camera& camera, IMAGE* image, float x, float y, BYTE opacity = 255) const
	{
		if (!image || opacity == 0)
			return;
		float draw_x = is_facing_right ? position.x + x
			: position.x + sprite_width - x - image->getwidth();
		const Vector2& camera_position = camera.get_position();
		int w = image->getwidth(), h = image->getheight();
		blend_image_alpha((int)(draw_x - camera_position.x),
			(int)(position.y + y - camera_position.y), w, h, image, 0, 0, opacity);
	}

	void move_and_collide(int delta)
	{
		is_grounded = false;
		const float last_tick_foot_pos_y = position.y + size.y;	// 移动前直接记录脚底，避免浮点反推让静止角色穿过平台
		velocity.y += gravity * delta;
		position += velocity * (float)delta;
		position.x = (std::clamp)(position.x, 0.0f, GAME_WORLD_WIDTH - sprite_width);
		Vector2 collision_position = get_collision_position();
		if (velocity.y > 0)
		{
			for (const Platform& platform : platform_list)
			{
				const Platform::CollisionShape& shape = platform.shape;
				bool is_collide_x = (max(collision_position.x + size.x, shape.right) - min(collision_position.x, shape.left)
					<= size.x + (shape.right - shape.left));
				bool is_collide_y = shape.y >= position.y && shape.y <= position.y + size.y;
				if (is_collide_x && is_collide_y)
				{
					if (last_tick_foot_pos_y <= shape.y)
					{
						position.y = shape.y - size.y;
						velocity.y = 0;
						is_grounded = true;
						break;
					}
				}
			}
		}

		check_bullet_collisions();
	}

protected:
	const float gravity = 1.6e-3f;			// 玩家重力
	float run_velocity = 0.35f;		// 跑动速度
	float jump_velocity = -0.85f;	// 跳跃速度
	const float sprite_width = 128;		// 角色画布宽度，镜像挂点不使用碰撞宽度
	int mp = 0;							// 角色能量，0～100
	int max_hp = 100;					// 血条按角色生命上限计算比例
	int normal_protection_threshold = 20;	// 此角色的普攻累计实际伤害阈值
	Atlas* atlas_hurt_left = nullptr;
	Atlas* atlas_hurt_right = nullptr;
	std::vector<int> hurt_durations;
	int hurt_duration = 0;
	int hp = 100;						// 角色生命值，归零后播放倒地动画
	Vector2 size;						// 角色碰撞尺寸
	Vector2 position;					// 玩家位置
	Vector2 velocity;					// 角色速度
	Animation animation_idle_left;
	Animation animation_idle_right;
	Animation animation_run_left;
	Animation animation_run_right;
	Animation animation_jump_left;
	Animation animation_jump_right;
	Atlas* atlas_block_left = nullptr;
	Atlas* atlas_block_right = nullptr;
	std::vector<int> block_durations;		// 按已确认素材的每帧时长推进，帧号在代码中从 0 开始
	int block_hold_begin = 0, block_hold_end = 0;
	int block_impact_begin = 0, block_impact_end = 0;
	int block_exit_begin = 0;
	Atlas* atlas_defeat_left = nullptr;	// 派生角色绑定已加载的 24 帧倒地素材
	Atlas* atlas_defeat_right = nullptr;
	float defeat_offset_x = 0;			// 倒地画布比常规画布宽，各角色单独校正水平挂点
	bool is_grounded = false;
	Animation* current_animation = nullptr;
	PlayerID id = PlayerID::P1;
	bool is_left_key_down = false;
	bool is_right_key_down = false;
	bool is_facing_right = true;
	bool is_facing_locked = false;		// 花京院大招期间锁定朝向，移动不改此值
	int attack_cd = 500;					// 普通攻击共用冷却
	bool can_attack = true;
	Timer timer_attack_cd;
	bool is_attacking = false;			// 普通攻击动作尚未结束
	bool is_attacking_ex = false;
	bool is_attacking_exxxx = false;
	bool allow_attack_during_ultimate = false;
	int special_mp_cost = 30;
	int ultimate_mp_cost = 100;
	int active_update_delta = 0;			// 本次更新中未被时停冻结的毫秒数
	int transformed_remaining = 0;
	int transformed_intro_elapsed = 0;
	Animation transformed_idle_left, transformed_idle_right, transformed_run_left, transformed_run_right;

private:
	void check_bullet_collisions()
	{
		Vector2 collision_position = get_collision_position();
		for (Bullet* bullet : bullet_list)
		{
			if (!bullet || !bullet->get_valid() || bullet->get_collide_target() != id
				|| !can_be_hit_by(*bullet))
				continue;
			if (!bullet->check_collision(collision_position, size))
				continue;

			AttackArea* area = dynamic_cast<AttackArea*>(bullet);
			int damage = (std::max)(0, bullet->get_damage());
			const bool blocked = get_blocking() && damage > 0;
			if (blocked)
			{
				// 有能量减伤 90% 并扣一点；最后一点仍享受 90%，耗尽后减伤 50%。
				const int multiplier = mp > 0 ? 1 : 5;
				if (mp > 0) add_mp(-1);
				const std::int64_t chip = (std::int64_t)damage * multiplier + block_damage_remainder;
				damage = (int)(chip / 10);
				block_damage_remainder = (int)(chip % 10);
				if (get_time_stop_remaining() == 0 && block_state != BlockState::Impact)
				{
					block_state = BlockState::Impact;
					block_frame = block_impact_begin;
					block_frame_time = 0;
				}
			}
			damage = (std::max)(0, modify_incoming_damage(damage));
			bullet->on_collide();
			if (bullet->get_transform_duration() > 0)
				apply_naiwa_transform(bullet->get_transform_duration());
			if (!area)
				bullet->set_valid(false);	// 攻击区域自行记录命中，不在首次命中后强制销毁
			const int actual_damage = (std::min)(hp, damage);
			hp -= actual_damage;
			if (actual_damage > 0)
			{
				on_damage_taken(actual_damage);
				if (!blocked)
				{
					apply_hit_stun(*bullet); // 格挡分支不会执行；时停期间保存僵直和击退，解冻后继续
					if (get_time_stop_remaining() > 0) hurt_pending = true; // 冻结期间不改变原有姿势
					else { hurt_active = true; hurt_elapsed = 0; interrupt_normal_attack(); }
				}
				if (bullet->get_attack_kind() == Bullet::AttackKind::Normal)
				{
					if (normal_combo_remaining == 0 || normal_combo_source != bullet->get_source_id()) normal_combo_damage = 0;
					normal_combo_source = bullet->get_source_id();
					normal_combo_remaining = 1000;
					normal_combo_damage += actual_damage; // 只累计同一攻击者普攻实际扣除的生命值
					const int threshold = bullet->get_normal_protection_threshold();
					if (hp > 0 && threshold > 0 && normal_combo_damage >= threshold)
					{
						normal_combo_damage = normal_combo_remaining = 0;
						make_invulnerable();
					}
				}
			}
			if (hp <= 0)
			{
				hp = 0;
				break;					// 致命命中后不继续扣血
			}
		}
	}

	void interrupt_normal_attack()
	{
		if (block_state == BlockState::Releasing) block_state = BlockState::None; // 收手期间已没有格挡，受击立即切换为受击姿势
		if (!is_attacking) return;
		is_attacking = false; // 未格挡受击取消普攻，已经飞出的羽毛笔保留；替身技能不取消
		for (const AttachedAttack& attack : attached_attacks)
			if (has_bullet(attack.area) && attack.area->get_attack_kind() == Bullet::AttackKind::Normal
				&& !attack.area->get_persistent_on_owner_hurt())
				attack.area->remove();
		prune_attached_attacks();
	}

	void apply_hit_stun(const Bullet& bullet)
	{
		const Bullet::AttackKind kind = bullet.get_attack_kind();
		const int duration = kind == Bullet::AttackKind::Normal ? 120
			: (kind == Bullet::AttackKind::Special ? 220 : 100);
		hit_stun_remaining = (std::max)(hit_stun_remaining, duration); // 多次命中刷新，但不叠加为超长僵直
		if (kind != Bullet::AttackKind::Normal)
		{
			knockback_remaining = duration;
			knockback_velocity = bullet.get_knockback_direction()
				* (kind == Bullet::AttackKind::Special ? 0.45f : 0.22f); // 特殊技能更重，大招每次只作小幅推退
		}
	}

	int update_hit_stun(int delta)
	{
		const int locked_delta = (std::min)(delta, hit_stun_remaining);
		hit_stun_remaining -= locked_delta;
		velocity.x = 0;
		if (knockback_remaining > 0)
		{
			const int step = (std::min)(delta, knockback_remaining);
			const float next_velocity = knockback_velocity * (1.0f - (float)step / knockback_remaining);
			velocity.x = (knockback_velocity + next_velocity) * 0.5f * step / delta;
			knockback_velocity = next_velocity;
			knockback_remaining -= step; // 匀减速积分，低帧率或单帧跨过结束点时也不会多推退
		}
		return delta - locked_delta;
	}

	void begin_block()
	{
		if (hp <= 0 || is_match_finished || get_hit_stunned() || get_time_stop_remaining() > 0 || !atlas_block_left || !atlas_block_right
			|| block_durations.empty() || is_attacking || is_attacking_ex
			|| (is_attacking_exxxx && !allow_attack_during_ultimate)) return;
		if (block_state != BlockState::None && block_state != BlockState::Releasing) return;
		hurt_active = hurt_pending = false;
		block_frame = block_state == BlockState::Releasing ? block_hold_begin : 0;
		block_state = block_state == BlockState::Releasing ? BlockState::Holding : BlockState::Raising;
		block_frame_time = 0;
		is_left_key_down = is_right_key_down = false;
		is_special_key_down = is_ultimate_key_down = false;
		velocity.x = 0;				// 防御不后退；空中仍由重力自然落地
	}

	void end_block()
	{
		if (!get_blocking()) return;
		block_state = BlockState::Releasing;
		block_frame = block_exit_begin;
		block_frame_time = 0;			// 松键即解除减伤，但必须播放完收手才能执行其他动作
	}

	void update_block(int delta)
	{
		if (is_block_key_down) begin_block();
		else end_block();
		if (!get_block_animation_active()) return;
		block_frame_time += delta;
		while (get_block_animation_active())
		{
			if (block_state == BlockState::Holding)
			{
				int loop_duration = 0;
				for (int i = block_hold_begin; i <= block_hold_end; ++i) loop_duration += block_durations[i];
				if (loop_duration > 0) block_frame_time %= loop_duration;	// 长时间按住或低帧率不积累多余循环
			}
			const int frame_duration = (std::max)(1, block_durations[block_frame]);
			if (block_frame_time < frame_duration) break;
			block_frame_time -= frame_duration;
			++block_frame;
			if ((block_state == BlockState::Raising && block_frame > block_hold_begin)
				|| (block_state == BlockState::Holding && block_frame > block_hold_end)
				|| (block_state == BlockState::Impact && block_frame > block_impact_end))
			{
				block_state = BlockState::Holding;
				block_frame = block_hold_begin;
			}
			else if (block_state == BlockState::Releasing && block_frame >= (int)block_durations.size())
			{
				block_state = BlockState::None;
				block_frame_time = 0;
				animation_idle_left.reset(); animation_idle_right.reset();
			}
		}
	}

	void emit_foot_particle(Atlas& atlas, int interval, int age = 0, float offset_x = 0)
	{
		IMAGE* frame = atlas.get_image(0);
		if (!frame)
			return;					// 图集未加载时不创建无效粒子
		Vector2 particle_position(position.x + (sprite_width - frame->getwidth()) / 2 + offset_x,
			position.y + 118 - frame->getheight());	// 现有角色画布脚底位于 y=118
		Particle particle(particle_position, &atlas, interval);
		particle.on_update(age);
		if (particle.check_valid())
			particle_list.push_back(particle);	// 粒子留在生成位置，不跟随角色移动
	}

	void update_particles(int delta)
	{
		for (Particle& particle : particle_list)
			particle.on_update(delta);
		particle_list.erase(std::remove_if(particle_list.begin(), particle_list.end(),
			[](const Particle& particle) { return !particle.check_valid(); }), particle_list.end());
	}

	void begin_death()
	{
		stop_player_attack_sounds(id);
		time_stop_audio_mask = 0;
		is_attack_audio_paused = false;
		is_dead = true;
		hit_stun_remaining = knockback_remaining = 0;
		knockback_velocity = 0;
		block_state = BlockState::None;
		hurt_active = hurt_pending = false;
		is_block_key_down = false;
		hp = 0;
		death_elapsed = 0;
		active_update_delta = 0;
		is_attacking = is_attacking_ex = is_attacking_exxxx = false;
		is_facing_locked = false;
		is_left_key_down = is_right_key_down = false;
		velocity.x = 0;
		is_invulnerable = is_showing_sketch_frame = false;
		time_stop_remaining = 0;
		time_stop_wave_remaining = 0;
		timer_attack_cd.pause();
		timer_run_effect_generation.pause();
		timer_die_effect_generation.restart();
		timer_die_effect_generation.resume();
		clear_attached_attacks();		// 倒地时移除仍跟随玩家的近战和持续攻击区域
	}

	void update_death(int delta)
	{
		update_particles(delta);
		move_and_collide(delta);			// 空中被击败时仍受重力影响，落地后才播放倒地
		if (!is_grounded)
			return;
		death_elapsed = (std::min)(death_elapsed + delta, (std::int64_t)1200);
		timer_die_effect_generation.on_update(delta);
	}

	struct AttachedAttack
	{
		AttackArea* area;
		Vector2 offset;
		int remaining;
		bool follow_facing;
		bool facing_right;
	};

	bool has_bullet(Bullet* bullet) const
	{
		return std::find(bullet_list.begin(), bullet_list.end(), bullet) != bullet_list.end()
			&& bullet->get_collide_target() == (id == PlayerID::P1 ? PlayerID::P2 : PlayerID::P1);
	}

	void position_attached_attack(const AttachedAttack& attack)
	{
		bool facing = attack.follow_facing ? is_facing_right : attack.facing_right;
		const Vector2& area_size = attack.area->get_size();
		float x = facing ? position.x + attack.offset.x
			: position.x + sprite_width - attack.offset.x - area_size.x;
		attack.area->set_position(x, position.y + attack.offset.y);
		attack.area->set_facing_right(facing);
	}

	void prune_attached_attacks()
	{
		attached_attacks.erase(std::remove_if(attached_attacks.begin(), attached_attacks.end(),
			[this](const AttachedAttack& attack)
			{
				return !has_bullet(attack.area) || attack.area->check_can_remove();
			}), attached_attacks.end());	// 先确认场景仍持有对象，再访问对象
	}

	void update_attached_attacks(int delta)
	{
		prune_attached_attacks();
		for (AttachedAttack& attack : attached_attacks)
		{
			attack.remaining -= delta;
			if (attack.remaining <= 0)
				attack.area->remove();
			else
				position_attached_attack(attack);
		}
		prune_attached_attacks();
	}

	enum class BlockState { None, Raising, Holding, Impact, Releasing };
	BlockState block_state = BlockState::None;
	int block_frame = 0;
	std::int64_t block_frame_time = 0;
	int block_damage_remainder = 0;		// 防御的小额伤害余数，跨次防御保留，防止反复收手抵消伤害
	bool is_block_key_down = false;
	bool is_match_finished = false;
	std::vector<AttachedAttack> attached_attacks;
	std::shared_ptr<int> callback_lifetime = std::make_shared<int>(0);
	void update_time_stop_audio(bool paused)
	{
		if (is_attack_audio_paused == paused) return;
		if (paused) time_stop_audio_mask = pause_player_attack_sounds(id);
		else resume_player_attack_sounds(id, time_stop_audio_mask);
		is_attack_audio_paused = paused;
	}
	unsigned int time_stop_audio_mask = 0; // 只恢复本次时停暂停的声音，不重启已结束的音效
	bool is_attack_audio_paused = false;
	int time_stop_remaining = 0;			// 直接冻结剩余时间，保留供其他效果使用
	int time_stop_wave_remaining = 0;		// 扩散圆捕获产生的冻结，由场景控制共同结束时间
	bool is_special_key_down = false;
	bool is_ultimate_key_down = false;

	int normal_combo_damage = 0, normal_combo_remaining = 0;
	PlayerID normal_combo_source = PlayerID::P1;
	int hit_stun_remaining = 0, knockback_remaining = 0; // 未格挡受击锁定和击退剩余时间（ms）
	float knockback_velocity = 0; // 单独记录受击速度，玩家输入不能覆盖击退
	bool hurt_active = false, hurt_pending = false;
	std::int64_t hurt_elapsed = 0;
	IMAGE img_sketch;						//动画帧剪影图片
	bool is_invulnerable = false;			//角色是否处于无敌状态
	bool is_showing_sketch_frame = false;	//当前帧是否应该显示剪影
	Timer timer_invulnerable;				//无敌状态定时器
	Timer timer_invulnerable_blink;			//无敌状态闪烁定时器
	
	std::vector<Particle> particle_list;	// 粒子对象列表
	bool is_dead = false;				// 倒地流程只初始化一次
	std::int64_t death_elapsed = 0;		// 倒地动画累计时间，结束后停在最后一帧
	const int death_impact_time = 880;	// 第 16 帧向前倒下时产生撞击烟尘
protected:
	std::vector<int> defeat_durations = {
		60,60,60,60,60,60,60,60,100,50,50,50,50,50,50,40,30,30,30,30,30,30,30,70 };

	Timer timer_run_effect_generation;		//跑动特效粒子发射定时器
	Timer timer_die_effect_generation;		//死亡撞击特效单次发射定时器
};

#endif // !_PLAYER_H_
