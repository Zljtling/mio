#ifndef _GAME_SCENE_H_
#define _GAME_SCENE_H_

#include "util.h"
#include "scene.h"
#include "player.h"
#include "platform.h"
#include "status_bar.h"
#include "scene_manager.h"
#include "time_stop_effect.h"
#include "timer.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>

extern Player* player_1;
extern Player* player_2;

extern IMAGE img_city_street;
extern IMAGE img_platform_large;
extern IMAGE img_platform_small;

extern IMAGE img_1P_winner;
extern IMAGE img_2P_winner;
extern IMAGE img_1P_cursor;
extern IMAGE img_2P_cursor;
extern IMAGE img_winner_bar;

extern Camera main_camera;
extern std::vector<Platform> platform_list;

extern IMAGE* img_player_1_avatar;
extern IMAGE* img_player_2_avatar;

extern SceneManager scene_manager;

class GameScene : public Scene
{
public:
	GameScene() = default;
	~GameScene() = default;

	void on_enter()
	{
		is_game_over = false;
		is_slide_out_started = false;
		is_return_to_menu_requested = false;
		winner_result = 0;
		winner_phase_elapsed = 0;
		cursor_elapsed = 0;

		pos_img_winner_bar.x = -img_winner_bar.getwidth();
		pos_img_winner_bar.y = (getheight() - img_winner_bar.getheight()) / 2;
		pos_x_img_winner_bar_dst = (getwidth() - img_winner_bar.getwidth()) / 2;

		pos_img_winner_text.x = -img_1P_winner.getwidth();
		winner_bar_x = (float)pos_img_winner_bar.x;
		winner_text_x = (float)pos_img_winner_text.x;
		pos_img_winner_text.y = (getheight() - img_1P_winner.getheight()) / 2;
		pos_x_img_winner_text_dst = (getwidth() - img_1P_winner.getwidth()) / 2;

		timer_winner_slide_in.restart();
		timer_winner_slide_in.set_wait_time(winner_display_duration);
		timer_winner_slide_in.resume();
		timer_winner_slide_in.set_one_shot(true);
		timer_winner_slide_in.set_callback([&]()
			{
				is_slide_out_started = true;
			});
		timer_winner_slide_out.restart();
		timer_winner_slide_out.set_wait_time(winner_exit_duration);
		timer_winner_slide_out.resume();
		timer_winner_slide_out.set_one_shot(true);
		timer_winner_slide_out.set_callback([&]()
			{
			is_return_to_menu_requested = true;
			});

		status_bar_1P.set_avatar(img_player_1_avatar);
		status_bar_2P.set_avatar(img_player_2_avatar);

		status_bar_1P.set_position(40, GAME_WINDOW_HEIGHT - 110);
		status_bar_2P.set_position(GAME_WINDOW_WIDTH - 421, GAME_WINDOW_HEIGHT - 110);
		status_bar_1P.set_max_hp(player_1->get_max_hp()); status_bar_2P.set_max_hp(player_2->get_max_hp());
		status_bar_1P.set_hp(player_1->get_hp()); status_bar_1P.set_mp(player_1->get_mp());
		status_bar_2P.set_hp(player_2->get_hp()); status_bar_2P.set_mp(player_2->get_mp());	// 第一帧就显示当前血量和能量
		status_bar_1P.set_skill_costs(player_1->get_special_cost(), player_1->get_ultimate_cost());
		status_bar_2P.set_skill_costs(player_2->get_special_cost(), player_2->get_ultimate_cost());
		status_bar_1P.set_ultimate_key(_T("Q")); status_bar_2P.set_ultimate_key(_T("/"));
		status_bar_1P.set_ultimate_ready(player_1->can_release_ultimate());
		status_bar_2P.set_ultimate_ready(player_2->can_release_ultimate());


		debug_is_paused = false;
		debug_step_requested = false;
		reset_time_stop_waves();
		player_1->set_position(1200, GAME_STREET_FLOOR_Y - 126);
		player_2->set_position(1872, GAME_STREET_FLOOR_Y - 126);
		platform_list.clear();
		const auto add_ledge = [&](int x, int y, IMAGE& image)
		{
			Platform ledge;
			ledge.img = &image;
			ledge.render_position = { x, y - 2 };
			ledge.shape.left = (float)x + 8;
			ledge.shape.right = (float)x + image.getwidth() - 8;
			ledge.shape.y = (float)y;
			platform_list.push_back(ledge);
		};
		// 每级相差 180px，三条外墙攀爬路线及中层横向连接台阶。
		for (int x : { 160, 1344, 2528 })
		{
			for (int y : { 1360, 1000, 640 }) add_ledge(x, y, img_platform_large);
			for (int y : { 1180, 820, 460 }) add_ledge(x + 112, y, img_platform_small);
		}
		for (int x : { 724, 1034, 1908, 2218 }) add_ledge(x, 1000, img_platform_small);
		Platform ground;
		ground.shape.left = 0; ground.shape.right = GAME_WORLD_WIDTH;
		ground.shape.y = GAME_STREET_FLOOR_Y;
		platform_list.push_back(ground);
		main_camera.reset();
		main_camera.follow(player_1->get_position(), player_2->get_position(), 0, true);

		mciSendString(_T("play bgm_game repeat from 0"), NULL, 0, NULL);
	}

	void on_update(int delta)
	{
		if (!player_1 || !player_2 || delta <= 0) return;
		if (is_debug && debug_is_paused)
		{
			if (!debug_step_requested) return;
			debug_step_requested = false;
			delta = debug_frame_step;
		}
		int remaining_delta = delta;
		while (remaining_delta > 0)
		{
			int step = (std::min)(remaining_delta, debug_frame_step);
			if (is_game_over)
			{
				player_1->on_update(step); player_2->on_update(step);
				main_camera.on_update(step);	// 结算仍播放倒地、粒子及镜头恢复
				update_result_animation(step);
				if (is_return_to_menu_requested)
				{
					scene_manager.switch_to(SceneManager::SceneType::Menu);
					return;			// 切换场景会销毁玩家，此后不能再访问玩家指针
				}
			}
			else
			{
				if (player_1->get_hp() <= 0 || player_2->get_hp() <= 0)
				{
					begin_game_over();
					continue;
				}
				cursor_elapsed = (std::min)(cursor_duration, cursor_elapsed + step);
				sync_time_stop_waves();
				std::array<int, 2> old_remaining = {};
				for (std::size_t i = 0; i < time_stop_waves.size(); i++)
				{
					old_remaining[i] = time_stop_waves[i].effect.get_remaining();
					time_stop_waves[i].effect.on_update(step);
				}
				refresh_time_stop_captures(&old_remaining);
				update_world(step);
				if (player_1->get_position().y >= GAME_WORLD_HEIGHT) player_1->set_hp(0);
				if (player_2->get_position().y >= GAME_WORLD_HEIGHT) player_2->set_hp(0);
				if (player_1->get_hp() <= 0 || player_2->get_hp() <= 0) begin_game_over();
				else
				{
					sync_time_stop_waves();
					refresh_time_stop_captures();
				}
			}
			remaining_delta -= step;
		}
		status_bar_1P.set_max_hp(player_1->get_max_hp()); status_bar_2P.set_max_hp(player_2->get_max_hp());
		status_bar_1P.set_hp(player_1->get_hp()); status_bar_1P.set_mp(player_1->get_mp());
		status_bar_2P.set_hp(player_2->get_hp()); status_bar_2P.set_mp(player_2->get_mp());
		status_bar_1P.set_ultimate_ready(player_1->can_release_ultimate());
		status_bar_2P.set_ultimate_ready(player_2->can_release_ultimate());
		status_bar_1P.on_update(delta); status_bar_2P.on_update(delta);
	}

	void on_draw(const Camera& camera)
	{
		IMAGE* window_target = GetWorkingImage();
		const int view_width = camera.get_view_width(), view_height = camera.get_view_height();
		if (world_layer.getwidth() != view_width || world_layer.getheight() != view_height)
			world_layer.Resize(view_width, view_height);
		std::fill_n(GetImageBuffer(&world_layer), (std::size_t)view_width * view_height, 0xFF10121Fu);
		SetWorkingImage(&world_layer);
		bool filter_background = false;
		for (const TimeStopWaveState& wave : time_stop_waves)
			filter_background = filter_background || wave.effect.get_visible();
		IMAGE* previous_target = GetWorkingImage();
		if (filter_background)
		{
			const int width = getwidth(), height = getheight();
			if (background_layer.getwidth() != width || background_layer.getheight() != height)
				background_layer.Resize(width, height);
			std::fill_n(GetImageBuffer(&background_layer), (std::size_t)width * height, 0xFF000000u);
			SetWorkingImage(&background_layer);
		}
		{
			ScopedImageDrawFilter unfiltered;	// 先合成街道和平台，只做一次背景波动过滤
			putimage_alpha(camera, 0, 0, &img_city_street);
			for (const Platform& platform : platform_list)
				platform.on_draw(camera);
		}
		if (filter_background)
		{
			SetWorkingImage(previous_target);
			for (const TimeStopWaveState& wave : time_stop_waves)
				wave.effect.apply_visual_at(&background_layer, camera.get_position(), true);
			putimage_alpha(0, 0, &background_layer);
		}


		draw_filtered(camera, player_1, [&]() { player_1->on_draw_background_effects(camera); });
		draw_filtered(camera, player_2, [&]() { player_2->on_draw_background_effects(camera); });
		draw_filtered(camera, player_1, [&]() { player_1->on_draw(camera); });
		draw_filtered(camera, player_2, [&]() { player_2->on_draw(camera); });
		
		for (const Bullet* bullet : bullet_list)
		{
			const Player* owner = bullet->get_collide_target() == PlayerID::P1 ? player_2 : player_1;
			draw_filtered(camera, owner, [&]() { bullet->on_draw(camera); });
		}
		for (const TimeStopWaveState& wave : time_stop_waves)
			wave.effect.draw_ring(camera);	// 圆环与 UI 在图像过滤作用域外绘制，保留原色
		if (!is_game_over) draw_player_cursors(camera);
		if (is_debug) on_draw_debug_world(camera);
		SetWorkingImage(window_target);
		HDC window_dc = GetImageHDC(window_target);
		SetStretchBltMode(window_dc, COLORONCOLOR);
		StretchBlt(window_dc, 0, 0, GAME_WINDOW_WIDTH, GAME_WINDOW_HEIGHT,
			GetImageHDC(&world_layer), 0, 0, view_width, view_height, SRCCOPY);
		if (is_game_over)
		{
			putimage_alpha(pos_img_winner_bar.x, pos_img_winner_bar.y, &img_winner_bar);
			if (winner_result != 0)
				putimage_alpha(pos_img_winner_text.x, pos_img_winner_text.y,
					winner_result == 1 ? &img_1P_winner : &img_2P_winner);
			else
			{
				DebugDrawState draw_state;
				settextstyle(64, 0, _T("IPix")); settextcolor(RGB(240, 225, 190)); setbkmode(TRANSPARENT);
				outtextxy(pos_img_winner_text.x + (img_1P_winner.getwidth() - textwidth(_T("平局"))) / 2,
					pos_img_winner_text.y + (img_1P_winner.getheight() - textheight(_T("平局"))) / 2, _T("平局"));
			}
		}
		else
		{
			status_bar_1P.on_draw();
			status_bar_2P.on_draw();
		}
		if (is_debug)
			on_draw_debug(camera);				// 最后绘制，避免被角色、替身和技能特效遮住
	}

	void on_input(const ExMessage& msg)
	{
		if (!player_1 || !player_2) return;
		if (!is_game_over) { sync_time_stop_waves(); refresh_time_stop_captures(); }	// 输入前先检查扩散接触，冻结时不积存新的按键
		if (msg.message == WM_KEYUP || msg.message == WM_SYSKEYUP)
		{
			if (msg.vkcode == VK_OEM_3)
			{
				is_debug = !is_debug;
				debug_is_paused = false;			// 关闭调试时恢复运行
				debug_step_requested = false;
				return;
			}
			if (is_debug)
			{
				switch (msg.vkcode)
				{
				case VK_F8:
					if (is_game_over) return;
					player_1->set_mp(100);
					player_2->set_mp(100);		// 仅调试时使用，方便检查特殊技能和大招
					return;
				case VK_F9:
					debug_is_paused = !debug_is_paused;
					debug_step_requested = false;
					return;
				case VK_F10:
					if (debug_is_paused)
						debug_step_requested = true;
					return;
				}
			}
		}

		if (is_game_over) return;			// 结算期间只允许调试开关，禁止继续战斗
		// 暂停时不启动新动作，但仍接收松键，防止恢复后角色一直移动。
		if (is_debug && debug_is_paused && msg.message != WM_KEYUP && msg.message != WM_SYSKEYUP)
			return;
		player_1->on_input(msg);
		player_2->on_input(msg);
		sync_time_stop_waves();				// 只在大招成功、发动序号变化后创建一次扩散圆
		refresh_time_stop_captures();
	}

	void on_exit()
	{
		delete player_1; player_1 = nullptr;
		delete player_2; player_2 = nullptr;
		debug_is_paused = false;
		debug_step_requested = false;
		reset_time_stop_waves();
		clear_bullets();
		timer_winner_slide_in.pause(); timer_winner_slide_out.pause();
		mciSendString(_T("stop bgm_game"), NULL, 0, NULL);
		mciSendString(_T("stop jotaro_time_stop"), NULL, 0, NULL);
		mciSendString(_T("stop jotaro_time_resume"), NULL, 0, NULL);
		main_camera.reset();
	}


	void on_draw_debug_world(const Camera& camera) const
	{
		if (!is_debug)
			return;
		DebugDrawState draw_state;
		for (const Platform& platform : platform_list)
			platform.on_draw_debug(camera);
		player_1->on_draw_debug(camera);
		player_2->on_draw_debug(camera);

		for (const Bullet* bullet : bullet_list)
		{
			if (!bullet || bullet->check_can_remove())
				continue;
			const Player* target = bullet->get_collide_target() == PlayerID::P1 ? player_1 : player_2;
			bool overlaps = target && bullet->check_collision_shape(target->get_collision_position(), target->get_size());
			bool target_can_be_hit = target && target->can_be_hit_by(*bullet);
			bullet->on_draw_debug(camera, overlaps, target_can_be_hit);
		}

	}

	void on_draw_debug(const Camera& camera) const
	{
		DebugDrawState draw_state;
		int total = 0, ready = 0;
		for (const Bullet* bullet : bullet_list)
			if (bullet && bullet->get_valid()) { ++total; if (bullet->check_debug_can_hit()) ++ready; }
		// 面板使用屏幕坐标，碰撞框使用世界坐标，两者都不会随镜头错位。
		setlinecolor(RGB(65, 70, 80));
		setfillcolor(RGB(20, 24, 30));
		int panel_right = getwidth() - 8 < 1130 ? getwidth() - 8 : 1130;
		solidrectangle(8, 8, panel_right, 138);
		settextcolor(RGB(235, 235, 235));
		TCHAR text[256];
		_stprintf_s(text, _T("调试模式 [%s]  ~ 关闭  |  F8 双方能量补满  |  F9 暂停/继续  |  F10 逐帧"),
			debug_is_paused ? _T("已暂停") : _T("运行中"));
		outtextxy(20, 16, text);
		const Vector2& camera_position = camera.get_position();
		_stprintf_s(text, _T("攻击/子弹:%d  可判定:%d  暂不可判定:%d  相机:(%.1f, %.1f)  单步:%dms"),
			total, ready, total - ready, camera_position.x, camera_position.y, debug_frame_step);
		outtextxy(20, 40, text);
		_stprintf_s(text, _T("P1 HP:%d MP:%d 时停:%dms  |  P2 HP:%d MP:%d 时停:%dms"),
			player_1->get_hp(), player_1->get_mp(), player_1->get_time_stop_remaining(),
			player_2->get_hp(), player_2->get_mp(), player_2->get_time_stop_remaining());
		outtextxy(20, 64, text);
		settextcolor(RGB(0, 125, 255)); outtextxy(20, 88, _T("蓝:角色"));
		settextcolor(RGB(180, 110, 255)); outtextxy(145, 88, _T("紫:无敌"));
		settextcolor(RGB(255, 255, 255)); outtextxy(270, 88, _T("白:子弹框+中心点"));
		settextcolor(RGB(255, 180, 40)); outtextxy(480, 88, _T("橙:矩形攻击范围"));
		settextcolor(RGB(128, 128, 128)); outtextxy(690, 88, _T("灰:暂不判定"));
		settextcolor(RGB(255, 70, 70)); outtextxy(870, 88, _T("红:可命中重叠"));
		settextcolor(RGB(235, 235, 235));
		outtextxy(20, 112, _T("命中次数只记录实际回调；延迟、非伤害帧、命中间隔及失效状态会显示在框旁。"));
	}

private:
	void clear_bullets()
	{
		for (Bullet* bullet : bullet_list) delete bullet;
		bullet_list.clear();				// 结算和离场都释放弹丸，不能只清空指针列表
	}

	void begin_game_over()
	{
		if (is_game_over) return;
		is_game_over = true;
		winner_result = player_1->get_hp() > 0 ? 1 : (player_2->get_hp() > 0 ? 2 : 0);
		player_1->finish_match(); player_2->finish_match();
		clear_bullets(); reset_time_stop_waves();
		cursor_elapsed = cursor_duration;
		const IMAGE& text = winner_result == 2 ? img_2P_winner : img_1P_winner;
		winner_text_x = (float)-text.getwidth();
		pos_img_winner_text.x = (int)winner_text_x;
		pos_img_winner_text.y = (getheight() - text.getheight()) / 2;
		pos_x_img_winner_text_dst = (getwidth() - text.getwidth()) / 2;
		timer_winner_slide_in.restart(); timer_winner_slide_out.restart();
		winner_phase_elapsed = 0;
		mciSendString(_T("stop bgm_game"), NULL, 0, NULL);
		mciSendString(_T("stop jotaro_time_stop"), NULL, 0, NULL);
		mciSendString(_T("stop jotaro_time_resume"), NULL, 0, NULL);
		mciSendString(_T("play ui_win from 0"), NULL, 0, NULL);
	}

	void update_result_animation(int delta)
	{
		while (delta > 0 && !is_return_to_menu_requested)
		{
			const bool sliding_out = is_slide_out_started;
			const int duration = sliding_out ? winner_exit_duration : winner_display_duration;
			const int step = (std::min)(delta, duration - winner_phase_elapsed);
			winner_bar_x += speed_winner_bar * step;
			winner_text_x += speed_winner_text * step;
			if (!sliding_out)
			{
				winner_bar_x = (std::min)(winner_bar_x, (float)pos_x_img_winner_bar_dst);
				winner_text_x = (std::min)(winner_text_x, (float)pos_x_img_winner_text_dst);
			}
			pos_img_winner_bar.x = (int)std::lround(winner_bar_x);
			pos_img_winner_text.x = (int)std::lround(winner_text_x);
			winner_phase_elapsed += step;
			if (sliding_out) timer_winner_slide_out.on_update(step);
			else timer_winner_slide_in.on_update(step);
			if (winner_phase_elapsed >= duration) winner_phase_elapsed = 0;
			delta -= step;				// 保留跨越滑入/滑出边界的时间，低帧率也按时结束
		}
	}

	void draw_player_cursors(const Camera& camera) const
	{
		if (cursor_elapsed >= cursor_duration || (cursor_elapsed / cursor_blink_interval) % 2 != 0) return;
		const std::array<const Player*, 2> players = { player_1, player_2 };
		const std::array<const IMAGE*, 2> cursors = { &img_1P_cursor, &img_2P_cursor };
		for (std::size_t i = 0; i < players.size(); ++i)
		{
			if (!players[i] || players[i]->get_hp() <= 0 || cursors[i]->getwidth() <= 0 || cursors[i]->getheight() <= 0) continue;
			const Vector2 position = players[i]->get_collision_position();
			const Vector2& offset = camera.get_position();
			const int x = (int)(position.x + players[i]->get_size().x / 2 - offset.x - cursor_size / 2);
			const int y = (std::max)(4, (int)(position.y + 10 - offset.y - cursor_size));
			AlphaBlend(GetImageHDC(GetWorkingImage()), x, y, cursor_size, cursor_size,
				GetImageHDC(cursors[i]), 0, 0, cursors[i]->getwidth(), cursors[i]->getheight(), { AC_SRC_OVER,0,255,AC_SRC_ALPHA });
		}
	}

	struct TimeStopWaveState
	{
		TimeStopEffect effect;
		Player* caster = nullptr;
		std::uint64_t observed_cast_id = 0;
		bool recovery_played = true;			// 每次发动只播放一次恢复尾音
		bool captured = false;				// 圆周接触后锁存，直到本次时停共同结束
	};

	void reset_time_stop_waves()
	{
		for (TimeStopWaveState& wave : time_stop_waves)
			wave = TimeStopWaveState();
		if (player_1) player_1->set_time_stop_wave_remaining(0);
		if (player_2) player_2->set_time_stop_wave_remaining(0);
	}

	bool has_active_time_stop_wave() const
	{
		for (const TimeStopWaveState& wave : time_stop_waves)
			if (wave.effect.get_active())
				return true;
		return false;
	}

	float get_time_stop_max_radius(const Vector2& center) const
	{
		float radius = 0;
		for (int corner = 0; corner < 4; corner++)
		{
			float x = (float)(corner & 1 ? GAME_WORLD_WIDTH : 0);
			float y = (float)(corner & 2 ? GAME_WORLD_HEIGHT : 0);
			float dx = x - center.x, dy = y - center.y;
			radius = (std::max)(radius, std::sqrt(dx * dx + dy * dy));
		}
		return radius + 64;					// 让扩散圆覆盖四角后自然移出画面
	}

	void sync_time_stop_waves()
	{
		const std::array<Player*, 2> players = { player_1, player_2 };
		for (std::size_t i = 0; i < time_stop_waves.size(); i++)
		{
			TimeStopWaveState& wave = time_stop_waves[i];
			Player* caster = players[i];
			if (wave.caster != caster)
			{
				time_stop_waves[1 - i].captured = false;	// 新玩家不继承旧玩家被另一波捕获的状态
				wave = TimeStopWaveState();	// 更换角色时清除旧槽，不访问已销毁的玩家指针
				wave.caster = caster;
			}
			if (!caster)
				continue;

			std::uint64_t cast_id = caster->get_time_stop_cast_id();
			int cast_remaining = caster->get_time_stop_cast_remaining();
			if (wave.observed_cast_id != cast_id)
			{
				wave.observed_cast_id = cast_id;
				wave.effect.reset();
				wave.captured = false;
				wave.recovery_played = true;
				if (caster->get_hp() > 0 && cast_remaining > 0)
				{
					Vector2 center = caster->get_collision_position() + caster->get_size() * 0.5f;
					wave.effect.start(center, cast_remaining, get_time_stop_max_radius(center));
					wave.recovery_played = false;
					main_camera.shake(3.0f, 120);	// 发动瞬间的轻微震动
					mciSendString(_T("play jotaro_time_stop from 0"), NULL, 0, NULL);
				}
			}
			if (caster->get_hp() <= 0 || cast_remaining <= 0)
			{
				wave.effect.finish();			// 立即解冻，画面继续短暂恢复颜色
				wave.captured = false;
				if (!wave.recovery_played)
				{
					wave.recovery_played = true;
					mciSendString(_T("play jotaro_time_resume from 0"), NULL, 0, NULL);
				}
			}
		}
	}

	void refresh_time_stop_captures(const std::array<int, 2>* remaining_override = nullptr)
	{
		const std::array<Player*, 2> players = { player_1, player_2 };
		std::array<int, 2> frozen_remaining = {};
		for (std::size_t i = 0; i < time_stop_waves.size(); i++)
		{
			TimeStopWaveState& wave = time_stop_waves[i];
			Player* target = players[1 - i];
			if (!target || target->get_hp() <= 0)
				continue;
			if (!wave.captured && wave.effect.get_active()
				&& wave.effect.check_collision(target->get_collision_position(), target->get_size()))
				wave.captured = true;
			if (wave.captured)
			{
				int remaining = remaining_override ? (*remaining_override)[i] : wave.effect.get_remaining();
				frozen_remaining[1 - i] = (std::max)(frozen_remaining[1 - i], remaining);
			}
		}
		for (std::size_t i = 0; i < players.size(); i++)
			if (players[i]) players[i]->set_time_stop_wave_remaining(frozen_remaining[i]);
	}

	void update_world(int delta)
	{
		// 在更新任何玩家之前确定两方攻击的时间，避免 1P / 2P 更新顺序影响冻结和伤害。
		const int attack_delta[2] = {
			(std::max)(0, delta - player_1->get_time_stop_remaining()),
			(std::max)(0, delta - player_2->get_time_stop_remaining()) };
		for (Bullet* bullet : bullet_list)
			bullet->set_time_stopped(attack_delta[bullet->get_source_id() == PlayerID::P1 ? 0 : 1] == 0);
		player_1->on_update(delta);
		player_2->on_update(delta);
		main_camera.follow(player_1->get_position(), player_2->get_position(), delta);
		main_camera.on_update(delta);
		bullet_list.erase(std::remove_if(bullet_list.begin(), bullet_list.end(),
			[](const Bullet* bullet)
			{
				bool deletable = bullet->check_can_remove();
				if (deletable) delete bullet;
				return deletable;
			}), bullet_list.end());
		for (Bullet* bullet : bullet_list)
		{
			int active_delta = attack_delta[bullet->get_source_id() == PlayerID::P1 ? 0 : 1];
			bullet->set_time_stopped(active_delta == 0);
			bullet->on_update(active_delta); // 只推进未被冻结的时间，包含解冻所在帧的剩余毫秒
		}
	}

	void draw_filtered(const Camera& camera, const Player* owner, const std::function<void()>& draw) const
	{
		bool needs_filter = false;
		for (const TimeStopWaveState& wave : time_stop_waves)
			if (wave.effect.get_visible() && wave.caster != owner)
				needs_filter = true;
		if (!needs_filter)
		{
			draw();
			return;
		}
		ScopedImageDrawFilter image_filter([this, &camera, owner](IMAGE& image, int screen_x, int screen_y)
		{
			const Vector2& camera_position = camera.get_position();
			Vector2 world_origin(screen_x + camera_position.x, screen_y + camera_position.y);
			for (const TimeStopWaveState& wave : time_stop_waves)
				if (wave.effect.get_visible() && wave.caster != owner)
					wave.effect.apply_visual_at(&image, world_origin, owner == nullptr);
		});
		draw();							// 源图复制后过滤，不改变图集与原来的遮挡次序
	}

	IMAGE world_layer;
	IMAGE background_layer;				// 复用背景缓冲区，避免逐层重复做全屏像素处理
	std::array<TimeStopWaveState, 2> time_stop_waves;

	StatusBar status_bar_1P;				//玩家 1 的状态条
	StatusBar status_bar_2P;				// 玩家 2 的状态条
	bool debug_is_paused = false;		// 调试暂停，关闭调试后自动恢复
	bool debug_step_requested = false;	// 等待一次逐帧推进
	const int debug_frame_step = 16;		// 单步时长，接近 60FPS 的一帧

	int cursor_elapsed = 0;
	const int cursor_duration = 2000;		// 开局提示只持续两秒，不受时停影响
	const int cursor_blink_interval = 200;
	const int cursor_size = 64;			// 缩小 200px 的原图，让提示停在角色头顶
	int winner_result = 0;				// 1:1P胜，2:2P胜，0:平局，结算后不再变化
	int winner_phase_elapsed = 0;
	float winner_bar_x = 0, winner_text_x = 0;
	bool is_return_to_menu_requested = false;
	const int winner_display_duration = 2500;
	const int winner_exit_duration = 1000;
	bool is_game_over = false;			//游戏是否结束

	POINT pos_img_winner_bar = { 0 };		// 结算动效背景位置
	POINT pos_img_winner_text = { 0 };		//结算动效文本位置
	int pos_x_img_winner_bar_dst = 0;		// 结算动效背景移动的目标位置
	int pos_x_img_winner_text_dst = 0;		//结算动效文本移动的目标位置
	Timer timer_winner_slide_in;			// 结算动效滑入定时器
	Timer timer_winner_slide_out;			// 结算动效滑出定时器
	bool is_slide_out_started = false;		//结算动效是否开始滑出

private:
	const float speed_winner_bar = 3.0f;	//结算动效背景移动速度
	const float speed_winner_text = 1.5f;	//结算动效文本移动速度
};

#endif // !_GAME_SCENE_H_
