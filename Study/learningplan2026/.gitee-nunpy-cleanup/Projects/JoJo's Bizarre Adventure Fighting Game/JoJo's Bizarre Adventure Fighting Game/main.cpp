#include <Windows.h>
#include <imm.h>
#pragma comment(lib, "Imm32.lib")
#include "util.h"
#include "audio.h"
#include "atlas.h"
#include "scene.h"
#include "bullet.h"
#include "player.h"
#include "platform.h"
#include "menu_scene.h"
#include "game_scene.h"
#include "selector_scene.h"
#include "scene_manager.h"
#include "character_stats.h"

#include <graphics.h>

Atlas atlas_naiwa_idle_left, atlas_naiwa_idle_right;
Atlas atlas_naiwa_run_left, atlas_naiwa_run_right;
Atlas atlas_naiwa_jump_left, atlas_naiwa_jump_right;
Atlas atlas_naiwa_hurt_left, atlas_naiwa_hurt_right;
Atlas atlas_naiwa_defeat_left, atlas_naiwa_defeat_right;
Atlas atlas_naiwa_teleport_left, atlas_naiwa_teleport_right;
Atlas atlas_naiwa_cast_left, atlas_naiwa_cast_right;
Atlas atlas_naiwa_phantom_summon_left, atlas_naiwa_phantom_summon_right;
Atlas atlas_naiwa_phantom_loop_left, atlas_naiwa_phantom_loop_right;
Atlas atlas_naiwa_phantom_disappear_left, atlas_naiwa_phantom_disappear_right;
Atlas atlas_naiwa_special_cast_left, atlas_naiwa_special_cast_right;
Atlas atlas_doubao_appear_left, atlas_doubao_appear_right;
Atlas atlas_doubao_guard_left, atlas_doubao_guard_right;
Atlas atlas_doubao_disappear_left, atlas_doubao_disappear_right;
Atlas atlas_doubao_hit_left, atlas_doubao_hit_right;
Atlas atlas_naiwa_laugh_left, atlas_naiwa_laugh_right;
Atlas atlas_naiwa_wave_left, atlas_naiwa_wave_right;
Atlas atlas_naiwa_plain_idle_left, atlas_naiwa_plain_idle_right;
Atlas atlas_naiwa_plain_run_left, atlas_naiwa_plain_run_right;
Atlas atlas_naiwa_transform_left, atlas_naiwa_transform_right;
Atlas atlas_naiwa_revert_left, atlas_naiwa_revert_right;
IMAGE img_avatar_Naiwa;
IMAGE img_1P_naiwa_desc, img_2P_naiwa_desc;
IMAGE img_Naiwa_selector_background_left, img_Naiwa_selector_background_right;

#pragma comment(lib,"Winmm.lib")

bool is_debug = false;							//是否开启调试模式

IMAGE img_menu_background;						//主菜单背景图片

IMAGE img_VS;									//VS 艺术字图片
IMAGE img_1P;									//1P 文本图片
IMAGE img_2P;									//2P 文本图片
IMAGE img_1P_desc;								//1P 键位描述图片
IMAGE img_2P_desc;								//2P 键位描述图片
IMAGE img_gravestone_left;						//朝向左的墓碑图片
IMAGE img_gravestone_right;						//朝向右的墓碑图片
IMAGE img_selector_tip;							//选角界面提示信息文本图片
IMAGE img_selector_background;					//选角界面背景图
IMAGE img_1P_selector_btn_idle_left;			//1P 向左选择按钮默认状态图片
IMAGE img_1P_selector_btn_idle_right;			//1P 向右选择按钮默认状态图片
IMAGE img_1P_selector_btn_down_left;			//1P 向左选择按钮按下状态图片
IMAGE img_1P_selector_btn_down_right;			//1P 向右选择按钮按下状态图片
IMAGE img_2P_selector_btn_idle_left;			//2P 向左选择按钮默认状态图片
IMAGE img_2P_selector_btn_idle_right;			//2P 向右选择按钮默认状态图片
IMAGE img_2P_selector_btn_down_left;			//2P 向左选择按钮按下状态图片
IMAGE img_2P_selector_btn_down_right;			//2P 向右选择按钮按下状态图片
IMAGE img_Jotaro_selector_background_left;		//选角界面朝向左的空条承太郎背景图片
IMAGE img_Jotaro_selector_background_right;		//选角界面朝向右的空条承太郎背景图片
IMAGE img_Kakyoin_selector_background_left;		//选角界面朝向左的花京院背景图片
IMAGE img_Kakyoin_selector_background_right;	//选角界面朝向右的花京院背景图片

IMAGE img_city_street;
IMAGE img_platform_large;						//大型平台图片
IMAGE img_platform_small;						//小型平台图片
 
IMAGE img_1P_cursor;							//1P 指示光标图片
IMAGE img_2P_cursor;							//2P 指示光标图片

Atlas atlas_Jotaro_block_left;					//空条承太郎朝向左的防御动画图集
Atlas atlas_Jotaro_block_right;					//空条承太郎朝向右的防御动画图集
Atlas atlas_Jotaro_defeat_left;					//空条承太郎朝向左的死亡动画图集
Atlas atlas_Jotaro_defeat_right;				//空条承太郎朝向右的死亡动画图集
Atlas atlas_Jotaro_hurt_left;					//空条承太郎朝向左的受击动画图集
Atlas atlas_Jotaro_hurt_right;					//空条承太郎朝向右的受击动画图集
Atlas atlas_Jotaro_idle_left;					//空条承太郎朝向左的站立动画图集
Atlas atlas_Jotaro_idle_right;					//空条承太郎朝向右的站立动画图集	
Atlas atlas_Jotaro_jump_left;					//空条承太郎朝向左的跳跃动画图集
Atlas atlas_Jotaro_jump_right;					//空条承太郎朝向右的跳跃动画图集
Atlas Atlas_Jotaro_kick_left;					//空条承太郎朝向左的踢腿动画图集
Atlas Atlas_Jotaro_kick_right;					//空条承太郎朝向右的踢腿动画图集
Atlas atlas_Jotaro_punch_left;					//空条承太郎朝向左的拳击动画图集
Atlas atlas_Jotaro_punch_right;					//空条承太郎朝向右的拳击动画图集
Atlas atlas_Jotaro_run_left;					//空条承太郎朝向左的奔跑动画图集
Atlas atlas_Jotaro_run_right;					//空条承太郎朝向右的奔跑动画图集

Atlas atlas_Kakyoin_block_left;					//花京院朝向左的防御动画图集
Atlas atlas_Kakyoin_block_right;				//花京院朝向右的防御动画图集
Atlas atlas_Kakyoin_defeat_left;				//花京院朝向左的死亡动画图集
Atlas atlas_Kakyoin_defeat_right;				//花京院朝向右的死亡动画图集
Atlas atlas_Kakyoin_hurt_left;					//花京院朝向左的受击动画图集
Atlas atlas_Kakyoin_hurt_right;					//花京院朝向右的受击动画图集
Atlas atlas_Kakyoin_idle_left;					//花京院朝向左的站立动画图集
Atlas atlas_Kakyoin_idle_right;					//花京院朝向右的站立动画图集
Atlas atlas_Kakyoin_jump_left;					//花京院朝向左的跳跃动画图集
Atlas atlas_Kakyoin_jump_right;					//花京院朝向右的跳跃动画图集
Atlas atlas_Kakyoin_quill_shot_left;			//花京院朝向左的攻击动画图集
Atlas atlas_Kakyoin_quill_shot_right;			//花京院朝向右的攻击动画图集
Atlas atlas_Kakyoin_run_left;					//花京院朝向左的奔跑动画图集
Atlas atlas_Kakyoin_run_right;					//花京院朝向右的奔跑动画图集

Atlas atlas_star_platinum_charged_punch_left;			// 白金之星蓄力重拳（朝左）
Atlas atlas_star_platinum_charged_punch_right;			// 白金之星蓄力重拳（朝右）
Atlas atlas_star_platinum_punch_wind_left;				// 重拳拳风（朝左）
Atlas atlas_star_platinum_punch_wind_right;				// 重拳拳风（朝右）
Atlas atlas_star_platinum_ora_barrage_left;				// 白金之星连续拳（朝左）
Atlas atlas_star_platinum_ora_barrage_right;			// 白金之星连续拳（朝右）
Atlas atlas_star_platinum_ora_barrage_afterimages_left;	// 连续拳虚影（朝左）
Atlas atlas_star_platinum_ora_barrage_afterimages_right;// 连续拳虚影（朝右）
Atlas atlas_hierophant_green_emerald_burst_left;		// 法皇范围绿宝石爆发（朝左）
Atlas atlas_hierophant_green_emerald_burst_right;		// 法皇范围绿宝石爆发（朝右）
Atlas atlas_hierophant_green_emerald_cannon_left;		// 法皇绿宝石螺旋炮（朝左）
Atlas atlas_hierophant_green_emerald_cannon_right;		// 法皇绿宝石螺旋炮（朝右）
Atlas atlas_hierophant_green_magic_circle_left;			// 螺旋炮魔法阵（朝左）
Atlas atlas_hierophant_green_magic_circle_right;		// 螺旋炮魔法阵（朝右）
Atlas atlas_hierophant_green_emerald_spiral_left;		// 绿宝石螺旋风（朝左）
Atlas atlas_hierophant_green_emerald_spiral_right;		// 绿宝石螺旋风（朝右）
Atlas atlas_hierophant_green_emerald_cannon_hit_left;	// 螺旋炮命中溅射（朝左）
Atlas atlas_hierophant_green_emerald_cannon_hit_right;	// 螺旋炮命中溅射（朝右）
IMAGE img_star_platinum_summon_aura_left;				// 白金之星召唤光环（朝左）
IMAGE img_star_platinum_summon_aura_right;				// 白金之星召唤光环（朝右）
IMAGE img_star_platinum_charge_vortex_left;				// 白金之星蓄力旋涡（朝左）
IMAGE img_star_platinum_charge_vortex_right;			// 白金之星蓄力旋涡（朝右）
IMAGE img_quill_left;									// 朝左的羽毛笔弹丸
IMAGE img_quill_right;									//羽毛笔图片
Atlas atlas_quill_break;								//羽毛笔爆炸动画图集
Atlas atlas_emerald_burst;								// 绿宝石爆炸、碎片散开和消散动画图集


Atlas atlas_run_effect;							//奔跑特效动画图集
Atlas atlas_land_effect;						//落地特效动画图集

IMAGE img_1P_winner;							//1P 获胜图片
IMAGE img_2P_winner;							//2P 获胜图片
IMAGE img_winner_bar;							//获胜玩家文本背景图片

IMAGE img_avatar_Jotaro;						//空条承太郎头像图片
IMAGE img_avatar_Kakyoin;						//花京院头像图片

Scene* menu_scene = nullptr;
Scene* game_scene = nullptr;
Scene* selector_scene = nullptr;

Camera main_camera;
SceneManager scene_manager;

std::vector<Bullet*> bullet_list;
std::vector<Platform> platform_list;

Player* player_1 = nullptr;
Player* player_2 = nullptr;

IMAGE* img_player_1_avatar = nullptr;
IMAGE* img_player_2_avatar = nullptr;

void flip_atlas(Atlas& src, Atlas& dst)
{
	dst.clear();
	for (int i = 0; i < src.get_size(); i++)
	{
		IMAGE img_flpipped;
		flip_image(src.get_image(i), &img_flpipped);
		dst.add_image(img_flpipped);
	}
}

// 声波素材按原始 300px 半径制作，启动时根据数值表等比放大，保持像素边缘清晰。
void scale_wave_atlas(Atlas& atlas, int radius)
{
	if (radius == 300) return;
	Atlas scaled;
	for (int i = 0; i < atlas.get_size(); ++i)
	{
		IMAGE* source = atlas.get_image(i);
		const int source_width = source->getwidth();
		const int source_height = source->getheight();
		if (source_width <= 0 || source_height <= 0) return;
		const int width = (std::max)(1, (int)std::lround(source_width * radius / 300.0));
		const int height = (std::max)(1, (int)std::lround(source_height * radius / 300.0));
		IMAGE resized(width, height);
		DWORD* source_pixels = GetImageBuffer(source);
		DWORD* target_pixels = GetImageBuffer(&resized);
		if (!source_pixels || !target_pixels) return;
		for (int y = 0; y < height; ++y)
			for (int x = 0; x < width; ++x)
				target_pixels[y * width + x] = source_pixels[(y * source_height / height) * source_width
					+ x * source_width / width];
		scaled.add_image(resized);
	}
	atlas = std::move(scaled);
}

// 裁去生成素材的透明留白，使用最近邻采样保持像素边缘清晰。
void load_city_ledge(IMAGE& target, LPCTSTR path, int target_width)
{
	IMAGE source;
	loadimage(&source, path);
	const int width = source.getwidth(), height = source.getheight();
	DWORD* pixels = GetImageBuffer(&source);
	int left = width, right = -1, top = height, bottom = -1;
	for (int y = 0; y < height; ++y)
		for (int x = 0; x < width; ++x)
			if ((pixels[y * width + x] >> 24) >= 64)
			{
				left = (std::min)(left, x); right = (std::max)(right, x);
				top = (std::min)(top, y); bottom = (std::max)(bottom, y);
			}
	if (right < left || bottom < top) { target = source; return; }
	const int crop_width = right - left + 1, crop_height = bottom - top + 1;
	const int target_height = (std::max)(1, target_width * crop_height / crop_width);
	target.Resize(target_width, target_height);
	DWORD* output = GetImageBuffer(&target);
	for (int y = 0; y < target_height; ++y)
		for (int x = 0; x < target_width; ++x)
		{
			DWORD pixel = pixels[(top + y * crop_height / target_height) * width + left + x * crop_width / target_width];
			output[y * target_width + x] = (pixel >> 24) >= 64 ? pixel : 0;
		}
}

void load_game_resources()
{
	auto load_naiwa = [](Atlas& right, Atlas& left, LPCTSTR path, int count)
	{
		right.load_from_file(path, count);
		flip_atlas(right, left);
	};
	load_naiwa(atlas_naiwa_idle_right, atlas_naiwa_idle_left, _T("resources/characters/naiwa/idle_right/frames/naiwa_idle_%d.png"), 16);
	load_naiwa(atlas_naiwa_run_right, atlas_naiwa_run_left, _T("resources/characters/naiwa/run_right/frames/naiwa_run_%d.png"), 24);
	load_naiwa(atlas_naiwa_jump_right, atlas_naiwa_jump_left, _T("resources/characters/naiwa/jump_right/frames/naiwa_jump_%d.png"), 8);
	load_naiwa(atlas_naiwa_hurt_right, atlas_naiwa_hurt_left, _T("resources/characters/naiwa/hurt_right/frames/naiwa_hurt_%d.png"), 8);
	load_naiwa(atlas_naiwa_defeat_right, atlas_naiwa_defeat_left, _T("resources/characters/naiwa/defeat_right/frames/naiwa_defeat_%d.png"), 8);
	load_naiwa(atlas_naiwa_teleport_right, atlas_naiwa_teleport_left, _T("resources/characters/naiwa/teleport_right/frames/naiwa_teleport_%d.png"), 8);
	load_naiwa(atlas_naiwa_cast_right, atlas_naiwa_cast_left, _T("resources/characters/naiwa/normal_attack/cast_right/frames/naiwa_cast_%d.png"), 8);
	load_naiwa(atlas_naiwa_phantom_summon_right, atlas_naiwa_phantom_summon_left, _T("resources/characters/naiwa/normal_attack/phantom_summon/frames/naiwa_phantom_summon_%d.png"), 8);
	load_naiwa(atlas_naiwa_phantom_loop_right, atlas_naiwa_phantom_loop_left, _T("resources/characters/naiwa/normal_attack/phantom_attack_loop/frames/naiwa_phantom_attack_%d.png"), 8);
	load_naiwa(atlas_naiwa_phantom_disappear_right, atlas_naiwa_phantom_disappear_left, _T("resources/characters/naiwa/normal_attack/phantom_disappear/frames/naiwa_phantom_disappear_%d.png"), 8);
	load_naiwa(atlas_naiwa_special_cast_right, atlas_naiwa_special_cast_left, _T("resources/characters/naiwa/special_skill/cast_right/frames/naiwa_special_cast_%d.png"), 8);
	load_naiwa(atlas_doubao_appear_right, atlas_doubao_appear_left, _T("resources/characters/naiwa/special_skill/doubao_appear_open/frames/doubao_appear_%d.png"), 8);
	load_naiwa(atlas_doubao_guard_right, atlas_doubao_guard_left, _T("resources/characters/naiwa/special_skill/doubao_guard_loop/frames/doubao_guard_%d.png"), 8);
	load_naiwa(atlas_doubao_disappear_right, atlas_doubao_disappear_left, _T("resources/characters/naiwa/special_skill/doubao_disappear/frames/doubao_disappear_%d.png"), 8);
	load_naiwa(atlas_doubao_hit_right, atlas_doubao_hit_left, _T("resources/characters/naiwa/special_skill/guard_hit_effect/frames/doubao_guard_hit_%d.png"), 8);
	load_naiwa(atlas_naiwa_laugh_right, atlas_naiwa_laugh_left, _T("resources/characters/naiwa/ultimate/belly_laugh_right/frames/naiwa_belly_laugh_%d.png"), 8);
	atlas_naiwa_wave_right.load_from_file(_T("resources/characters/naiwa/ultimate/soundwave_expand/frames/naiwa_soundwave_%d.png"), 8);
	scale_wave_atlas(atlas_naiwa_wave_right, character_stat_int("naiwa", "ultimate_radius_px", 500));
	flip_atlas(atlas_naiwa_wave_right, atlas_naiwa_wave_left);
	load_naiwa(atlas_naiwa_plain_idle_right, atlas_naiwa_plain_idle_left, _T("resources/characters/naiwa/ultimate/transformed_plain_idle_right/frames/naiwa_plain_idle_%d.png"), 8);
	load_naiwa(atlas_naiwa_plain_run_right, atlas_naiwa_plain_run_left, _T("resources/characters/naiwa/ultimate/transformed_plain_run_right/frames/naiwa_plain_run_%d.png"), 8);
	load_naiwa(atlas_naiwa_transform_right, atlas_naiwa_transform_left, _T("resources/characters/naiwa/ultimate/target_transform/frames/naiwa_target_transform_%d.png"), 8);
	load_naiwa(atlas_naiwa_revert_right, atlas_naiwa_revert_left, _T("resources/characters/naiwa/ultimate/target_revert/frames/naiwa_target_revert_%d.png"), 8);
	loadimage(&img_avatar_Naiwa, _T("resources/selector/avatar_naiwa.png"));
	AddFontResourceEx(_T("resources/fonts/IPix.ttf"), FR_PRIVATE, NULL);	// 注册 IPix 像素字体

	loadimage(&img_menu_background, _T("resources/menu/menu_background.png"), GAME_WINDOW_WIDTH, GAME_WINDOW_HEIGHT);	// 主菜单背景

	loadimage(&img_VS, _T("resources/selector/VS.png"));	// 选角界面 VS 标识
	loadimage(&img_1P, _T("resources/selector/1P.png"));	// 选角界面 1P 标识
	loadimage(&img_2P, _T("resources/selector/2P.png"));	// 选角界面 2P 标识
	loadimage(&img_1P_desc, _T("resources/selector/1P_desc.png"), 440, 120);	// 1P 操作按键说明
	loadimage(&img_2P_desc, _T("resources/selector/2P_desc.png"), 440, 120);	// 2P 操作按键说明
	loadimage(&img_1P_naiwa_desc, _T("resources/selector/1P_naiwa_desc.png"), 440, 120);
	loadimage(&img_2P_naiwa_desc, _T("resources/selector/2P_naiwa_desc.png"), 440, 120);
	loadimage(&img_gravestone_right, _T("resources/selector/gravestone.png"));	// 角色选择框架（朝右）
	flip_image(&img_gravestone_right, &img_gravestone_left);	// 镜像生成角色选择框架（朝左）
	loadimage(&img_selector_tip, _T("resources/selector/selector_tip.png"));	// 选角界面开始提示
	loadimage(&img_selector_background, _T("resources/selector/selector_background.png"), GAME_WINDOW_WIDTH, GAME_WINDOW_HEIGHT);	// 选角主背景
	loadimage(&img_1P_selector_btn_idle_right, _T("resources/selector/1P_selector_btn_idle.png"));	// 1P 向右选择按钮默认状态图片
	flip_image(&img_1P_selector_btn_idle_right, &img_1P_selector_btn_idle_left);	// 镜像生成1P 向左选择按钮默认状态图片（朝左）
	loadimage(&img_1P_selector_btn_down_right, _T("resources/selector/1P_selector_btn_down.png"));	// 1P 向右选择按钮按下状态图片
	flip_image(&img_1P_selector_btn_down_right, &img_1P_selector_btn_down_left);	// 镜像生成1P 向左选择按钮按下状态图片（朝左）
	loadimage(&img_2P_selector_btn_idle_right, _T("resources/selector/2P_selector_btn_idle.png"));	// 2P 向右选择按钮默认状态图片
	flip_image(&img_2P_selector_btn_idle_right, &img_2P_selector_btn_idle_left);	// 镜像生成2P 向左选择按钮默认状态图片（朝左）
	loadimage(&img_2P_selector_btn_down_right, _T("resources/selector/2P_selector_btn_down.png"));	// 2P 向右选择按钮按下状态图片
	flip_image(&img_2P_selector_btn_down_right, &img_2P_selector_btn_down_left);	// 镜像生成2P 向左选择按钮按下状态图片（朝左）
	loadimage(&img_Jotaro_selector_background_right, _T("resources/selector/jotaro_selector_background.png"), 560, GAME_WINDOW_HEIGHT);	// 承太郎选角滚动底纹（朝右）
	flip_image(&img_Jotaro_selector_background_right, &img_Jotaro_selector_background_left);	// 镜像生成选角界面空条承太郎背景图片（朝左）
	loadimage(&img_Kakyoin_selector_background_right, _T("resources/selector/kakyoin_selector_background.png"), 560, GAME_WINDOW_HEIGHT);	// 花京院选角滚动底纹（朝右）
	flip_image(&img_Kakyoin_selector_background_right, &img_Kakyoin_selector_background_left);	// 镜像生成选角界面花京院背景图片（朝左）
	loadimage(&img_Naiwa_selector_background_right, _T("resources/selector/naiwa_selector_background.png"), 560, GAME_WINDOW_HEIGHT);
	flip_image(&img_Naiwa_selector_background_right, &img_Naiwa_selector_background_left);

	loadimage(&img_city_street, _T("resources/game_scene/backgrounds/city_street.png"), GAME_WORLD_WIDTH, GAME_WORLD_HEIGHT);	// 夜间城市街道
	load_city_ledge(img_platform_large, _T("resources/game_scene/platforms/city_ledge_large.png"), 512);	// 大型平台图片
	load_city_ledge(img_platform_small, _T("resources/game_scene/platforms/city_ledge_small.png"), 288);	// 小型平台图片

	loadimage(&img_1P_cursor, _T("resources/game_scene/ui/1P_cursor.png"));	// 1P 指示光标图片
	loadimage(&img_2P_cursor, _T("resources/game_scene/ui/2P_cursor.png"));	// 2P 指示光标图片

	atlas_Jotaro_block_right.load_from_file(_T("resources/characters/jojo/block_right/frames/jojo_block_%d.png"), 16);	// 空条承太郎朝向右的防御动画图集
	flip_atlas(atlas_Jotaro_block_right, atlas_Jotaro_block_left);	// 镜像生成空条承太郎防御动画图集（朝左）
	atlas_Jotaro_defeat_right.load_from_file(_T("resources/characters/jojo/defeat_right/frames/jojo_defeat_%d.png"), 24);	// 空条承太郎朝向右的死亡动画图集
	flip_atlas(atlas_Jotaro_defeat_right, atlas_Jotaro_defeat_left);	// 镜像生成空条承太郎死亡动画图集（朝左）
	atlas_Jotaro_hurt_right.load_from_file(_T("resources/characters/jojo/hurt_right/frames/jojo_hurt_%d.png"), 16);	// 空条承太郎朝向右的受击动画图集
	flip_atlas(atlas_Jotaro_hurt_right, atlas_Jotaro_hurt_left);	// 镜像生成空条承太郎受击动画图集（朝左）
	atlas_Jotaro_idle_right.load_from_file(_T("resources/characters/jojo/idle_right/frames/jojo_idle_%d.png"),16);	// 空条承太郎朝向右的站立动画图集
	flip_atlas(atlas_Jotaro_idle_right, atlas_Jotaro_idle_left);	// 镜像生成空条承太郎站立动画图集（朝左）
	atlas_Jotaro_jump_right.load_from_file(_T("resources/characters/jojo/jump_right/frames/jojo_jump_%d.png"), 24);	// 空条承太郎朝向右的跳跃动画图集
	flip_atlas(atlas_Jotaro_jump_right, atlas_Jotaro_jump_left);	// 镜像生成空条承太郎跳跃动画图集（朝左）
	Atlas_Jotaro_kick_right.load_from_file(_T("resources/characters/jojo/kick_right/frames/jojo_kick_%d.png"), 20);	// 空条承太郎朝向右的踢腿动画图集
	flip_atlas(Atlas_Jotaro_kick_right, Atlas_Jotaro_kick_left);	// 镜像生成空条承太郎踢腿动画图集（朝左）
	atlas_Jotaro_punch_right.load_from_file(_T("resources/characters/jojo/punch_right/frames/jojo_punch_%d.png"), 16);	// 空条承太郎朝向右的拳击动画图集
	flip_atlas(atlas_Jotaro_punch_right, atlas_Jotaro_punch_left);	// 镜像生成空条承太郎拳击动画图集（朝左）
	atlas_Jotaro_run_right.load_from_file(_T("resources/characters/jojo/run_right/frames/jojo_run_%d.png"), 24);	// 空条承太郎朝向右的奔跑动画图集
	flip_atlas(atlas_Jotaro_run_right, atlas_Jotaro_run_left);	// 镜像生成空条承太郎奔跑动画图集（朝左）

	atlas_Kakyoin_block_right.load_from_file(_T("resources/characters/Kakyoin/block_right/frames/Kakyoin_block_%d.png"), 16);	// 花京院朝向右的防御动画图集
	flip_atlas(atlas_Kakyoin_block_right, atlas_Kakyoin_block_left);	// 镜像生成花京院防御动画图集（朝左）
	atlas_Kakyoin_defeat_right.load_from_file(_T("resources/characters/Kakyoin/defeat_right/frames/Kakyoin_defeat_%d.png"), 24);	// 花京院朝向右的死亡动画图集
	flip_atlas(atlas_Kakyoin_defeat_right, atlas_Kakyoin_defeat_left);	// 镜像生成花京院死亡动画图集（朝左）
	atlas_Kakyoin_hurt_right.load_from_file(_T("resources/characters/Kakyoin/hurt_right/frames/Kakyoin_hurt_%d.png"), 16);	// 花京院朝向右的受击动画图集
	flip_atlas(atlas_Kakyoin_hurt_right, atlas_Kakyoin_hurt_left);	// 镜像生成花京院受击动画图集（朝左）
	atlas_Kakyoin_idle_right.load_from_file(_T("resources/characters/Kakyoin/idle_right/frames/Kakyoin_idle_%d.png"), 16);	// 花京院朝向右的站立动画图集
	flip_atlas(atlas_Kakyoin_idle_right, atlas_Kakyoin_idle_left);	// 镜像生成花京院站立动画图集（朝左）
	atlas_Kakyoin_jump_right.load_from_file(_T("resources/characters/Kakyoin/jump_right/frames/Kakyoin_jump_%d.png"), 24);	// 花京院朝向右的跳跃动画图集
	flip_atlas(atlas_Kakyoin_jump_right, atlas_Kakyoin_jump_left);	// 镜像生成花京院跳跃动画图集（朝左）
	atlas_Kakyoin_quill_shot_right.load_from_file(_T("resources/characters/kakyoin/quill_shot_right/frames/kakyoin_quill_shot_%d.png"), 16);	// 花京院朝向右的攻击动画图集
	flip_atlas(atlas_Kakyoin_quill_shot_right, atlas_Kakyoin_quill_shot_left);	// 镜像生成花京院攻击动画图集（朝左）
	atlas_Kakyoin_run_right.load_from_file(_T("resources/characters/Kakyoin/run_right/frames/Kakyoin_run_%d.png"), 24);	// 花京院朝向右的奔跑动画图集
	flip_atlas(atlas_Kakyoin_run_right, atlas_Kakyoin_run_left);	// 镜像生成花京院奔跑动画图集（朝左）

	atlas_star_platinum_charged_punch_right.load_from_file(_T("resources/characters/jojo/skills/charged_punch_right/stand/frames/star_platinum_charged_punch_%d.png"), 46);	// 白金之星蓄力重拳（朝右）
	flip_atlas(atlas_star_platinum_charged_punch_right, atlas_star_platinum_charged_punch_left);	// 镜像生成白金之星蓄力重拳（朝左）
	atlas_star_platinum_punch_wind_right.load_from_file(_T("resources/characters/jojo/skills/charged_punch_right/effects/punch/frames/star_platinum_punch_wind_%d.png"), 8);	// 重拳拳风（朝右）
	flip_atlas(atlas_star_platinum_punch_wind_right, atlas_star_platinum_punch_wind_left);	// 镜像生成重拳拳风（朝左）
	atlas_star_platinum_ora_barrage_right.load_from_file(_T("resources/characters/jojo/skills/ora_barrage_right/stand/frames/star_platinum_ora_barrage_%d.png"), 24);	// 白金之星连续拳（朝右）
	flip_atlas(atlas_star_platinum_ora_barrage_right, atlas_star_platinum_ora_barrage_left);	// 镜像生成白金之星连续拳（朝左）
	atlas_star_platinum_ora_barrage_afterimages_right.load_from_file(_T("resources/characters/jojo/skills/ora_barrage_right/effects/frames/star_platinum_ora_barrage_afterimages_%d.png"), 24);	// 连续拳虚影（朝右）
	flip_atlas(atlas_star_platinum_ora_barrage_afterimages_right, atlas_star_platinum_ora_barrage_afterimages_left);	// 镜像生成连续拳虚影（朝左）
	atlas_hierophant_green_emerald_burst_right.load_from_file(_T("resources/characters/kakyoin/skills/emerald_burst_right/stand/frames/hierophant_green_emerald_burst_%d.png"), 36);	// 法皇范围绿宝石爆发（朝右）
	flip_atlas(atlas_hierophant_green_emerald_burst_right, atlas_hierophant_green_emerald_burst_left);	// 镜像生成法皇范围绿宝石爆发（朝左）
	atlas_hierophant_green_emerald_cannon_right.load_from_file(_T("resources/characters/kakyoin/skills/emerald_cannon_right/stand/frames/hierophant_green_emerald_cannon_%d.png"), 26);	// 法皇绿宝石螺旋炮（朝右）
	flip_atlas(atlas_hierophant_green_emerald_cannon_right, atlas_hierophant_green_emerald_cannon_left);	// 镜像生成法皇绿宝石螺旋炮（朝左）
	atlas_hierophant_green_magic_circle_right.load_from_file(_T("resources/characters/kakyoin/skills/emerald_cannon_right/magic_circle/frames/hierophant_green_magic_circle_%d.png"), 12);	// 螺旋炮魔法阵（朝右）
	flip_atlas(atlas_hierophant_green_magic_circle_right, atlas_hierophant_green_magic_circle_left);	// 镜像生成螺旋炮魔法阵（朝左）
	atlas_hierophant_green_emerald_spiral_right.load_from_file(_T("resources/characters/kakyoin/skills/emerald_cannon_right/spiral_wind/frames/hierophant_green_emerald_spiral_%d.png"), 12);	// 绿宝石螺旋风（朝右）
	flip_atlas(atlas_hierophant_green_emerald_spiral_right, atlas_hierophant_green_emerald_spiral_left);	// 镜像生成绿宝石螺旋风（朝左）
	atlas_hierophant_green_emerald_cannon_hit_right.load_from_file(_T("resources/characters/kakyoin/skills/emerald_cannon_right/hit_splash/frames/hierophant_green_emerald_cannon_hit_%d.png"), 6);	// 螺旋炮命中溅射（朝右）
	flip_atlas(atlas_hierophant_green_emerald_cannon_hit_right, atlas_hierophant_green_emerald_cannon_hit_left);	// 镜像生成螺旋炮命中溅射（朝左）
	loadimage(&img_star_platinum_summon_aura_right, _T("resources/characters/jojo/skills/charged_punch_right/effects/star_platinum_summon_aura_right.png"));	// 白金之星召唤光环（朝右）
	flip_image(&img_star_platinum_summon_aura_right, &img_star_platinum_summon_aura_left);	// 镜像生成白金之星召唤光环（朝左）
	loadimage(&img_star_platinum_charge_vortex_right, _T("resources/characters/jojo/skills/charged_punch_right/effects/star_platinum_charge_vortex_right.png"));	// 白金之星蓄力旋涡（朝右）
	flip_image(&img_star_platinum_charge_vortex_right, &img_star_platinum_charge_vortex_left);	// 镜像生成白金之星蓄力旋涡（朝左）
	loadimage(&img_quill_right, _T("resources/characters/kakyoin/projectiles/quill_right/kakyoin_quill_projectile_right.png"));	// 花京院羽毛笔弹丸（朝右）
	flip_image(&img_quill_right, &img_quill_left);	// 镜像生成羽毛笔弹丸（朝左）
	atlas_quill_break.load_from_file(_T("resources/characters/kakyoin/effects/quill_hit/frames/kakyoin_quill_hit_%d.png"), 4);	// 羽毛笔命中后爆开特效（4帧）
	atlas_emerald_burst.load_from_file(_T("resources/characters/kakyoin/effects/emerald_burst/frames/hierophant_green_emerald_burst_%d.png"), 4);	// 绿宝石爆炸：炸开、碎片散开至消散（4帧）
	

	atlas_run_effect.load_from_file(_T("resources/game_scene/effects/movement/run_effect_%d.png"), 4);	// 奔跑特效动画图集
	atlas_land_effect.load_from_file(_T("resources/game_scene/effects/movement/land_effect_%d.png"), 2);	// 落地特效动画图集

	loadimage(&img_1P_winner, _T("resources/game_scene/ui/1P_winner.png"));	// 1P 获胜图片
	loadimage(&img_2P_winner, _T("resources/game_scene/ui/2P_winner.png"));	// 2P 获胜图片
	loadimage(&img_winner_bar, _T("resources/game_scene/ui/winner_bar.png"));	// 获胜玩家文本背景图片

	loadimage(&img_avatar_Jotaro, _T("resources/selector/avatar_jotaro.png"));	// 空条承太郎头像图片
	loadimage(&img_avatar_Kakyoin, _T("resources/selector/avatar_kakyoin.png"));	// 花京院头像图片

	mciSendString(_T("open resources/bgm_game.mp3 alias bgm_game"), NULL, 0, NULL);	// 对战背景音乐：白金之星处刑曲电音 Remix 原视频完整音轨
	mciSendString(_T("open resources/bgm_menu.mp3 alias bgm_menu"), NULL, 0, NULL);	// 菜单和选角背景音乐：黑帮摇原视频完整音轨
	load_player_sound(_T("jotaro_charged_punch"), _T("resources/jotaro_charged_punch.wav"));	// 承太郎蓄力攻击：参考视频 1:47—1:49 的两声欧拉
	load_player_sound(_T("jotaro_ora_barrage"), _T("resources/jotaro_ora_barrage.wav"));	// 时停后连续拳：参考视频 1:51—1:52.5 的欧拉连喊
	load_player_sound(_T("kakyoin_quill_shoot"), _T("resources/kakyoin_quill_shoot.wav"));	// 羽毛笔普攻的短促发射声
	load_player_sound(_T("kakyoin_quill_hit"), _T("resources/kakyoin_quill_hit.wav"));	// 羽毛笔命中后小范围碎裂声
	load_player_sound(_T("kakyoin_emerald_cast"), _T("resources/kakyoin_emerald_cast.wav"));	// 花京院特殊技能的短促发力语音
	load_player_sound(_T("kakyoin_emerald_burst"), _T("resources/kakyoin_emerald_burst.wav"));	// 八个错峰绿宝石爆炸的释放音效
	load_player_sound(_T("kakyoin_emerald_cannon_cast"), _T("resources/kakyoin_emerald_cannon_cast.wav"));	// 大招发动的绿宝石水花语音
	load_player_sound(_T("kakyoin_emerald_cannon_loop"), _T("resources/kakyoin_emerald_cannon_loop.wav"));	// 螺旋水花炮击的三秒持续发射声
	load_player_sound(_T("kakyoin_emerald_cannon_hit"), _T("resources/kakyoin_emerald_cannon_hit.wav"));	// 炮击命中时的绿宝石溅射声
	mciSendString(_T("open resources/jotaro_time_stop.wav alias jotaro_time_stop"), NULL, 0, NULL);	// 承太郎时停发动音效，来自参考视频第8—10秒
	mciSendString(_T("open resources/jotaro_time_resume.wav alias jotaro_time_resume"), NULL, 0, NULL);	// 时停解除的短反向尾音
	mciSendString(_T("open resources/ui_confirm.wav alias ui_confirm"), NULL, 0, NULL);	// 界面确认音效
	mciSendString(_T("open resources/ui_switch.wav alias ui_switch"), NULL, 0, NULL);	// 选角切换音效
	mciSendString(_T("open resources/ui_win.wav alias ui_win"), NULL, 0, NULL);	// 胜利音效
}

// 仅在游戏窗口获得键盘焦点时切换布局；启动时窗口可能尚未激活。
static void activate_game_english_keyboard(HWND window)
{
	if (!window || GetForegroundWindow() != window) return;
	int count = GetKeyboardLayoutList(0, nullptr);
	std::vector<HKL> layouts(count > 0 ? count : 0);
	if (count > 0) count = GetKeyboardLayoutList(count, layouts.data());
	HKL english = nullptr;
	for (int i = 0; i < count; ++i)
		if (LOWORD((UINT_PTR)layouts[i]) == 0x0409) { english = layouts[i]; break; }
	if (!english) english = LoadKeyboardLayoutW(L"00000409", KLF_ACTIVATE | KLF_SUBSTITUTE_OK);
	if (english && LOWORD((UINT_PTR)english) == 0x0409)
	{
		const DWORD window_thread = GetWindowThreadProcessId(window, nullptr);
		if (window_thread == GetCurrentThreadId())
			ActivateKeyboardLayout(english, KLF_SETFORPROCESS);
		else
			PostMessageW(window, WM_INPUTLANGCHANGEREQUEST, 0, (LPARAM)english);
	}
	HIMC context = ImmGetContext(window);
	if (context) { ImmSetOpenStatus(context, FALSE); ImmReleaseContext(window, context); }
}

int main()
{
	ExMessage msg;
	const int FPS = 60;
	srand((unsigned int)GetTickCount());	// 每次启动使用不同随机序列，普通拳击和踢击随机选择

	load_game_resources();

	initgraph(GAME_WINDOW_WIDTH, GAME_WINDOW_HEIGHT, EW_SHOWCONSOLE);
	const HWND game_window = GetHWnd();
	SetForegroundWindow(game_window);
	
	settextstyle(28, 0, _T("IPix"));
	setbkmode(TRANSPARENT);

	BeginBatchDraw();

	menu_scene = new MenuScene();
	game_scene = new GameScene();
	selector_scene = new SelectorScene();
	scene_manager.set_current_scene(menu_scene);
	bool game_was_focused = false;

	while (true)
	{
		const bool game_is_focused = GetForegroundWindow() == game_window;
		if (game_is_focused && !game_was_focused)
			activate_game_english_keyboard(game_window);
		game_was_focused = game_is_focused;
		DWORD frame_start_time = GetTickCount();

		while (peekmessage(&msg))
		{
			scene_manager.on_input(msg);
		}

		static DWORD last_tick_time = GetTickCount();
		DWORD current_tick_time = GetTickCount();
		DWORD delta_time = current_tick_time - last_tick_time;
		scene_manager.on_update(delta_time);
		last_tick_time = current_tick_time;

		cleardevice();
		scene_manager.on_draw(main_camera);
		FlushBatchDraw();

		DWORD frame_end_time = GetTickCount();
		DWORD frame_delta_time = frame_end_time - frame_start_time;
		if (frame_delta_time < 1000 / FPS)
			Sleep(1000 / FPS - frame_delta_time);
	}

	EndBatchDraw();

	return 0;
}	
	
