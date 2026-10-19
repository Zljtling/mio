#ifndef _SELECTOR_SCENE_H_
#define _SELECTOR_SCENE_H_

#include "scene.h"
#include "animation.h"
#include "player_id.h"
#include "scene_manager.h"
#include "jotaro_player.h"
#include "kakyoin_player.h"
#include "naiwa_player.h"

extern Player* player_1;
extern Player* player_2;

extern IMAGE img_VS;									//VS 艺术字图片
extern IMAGE img_1P;									//1P 文本图片
extern IMAGE img_2P;									//2P 文本图片
extern IMAGE img_1P_desc;								//1P 键位描述图片
extern IMAGE img_2P_desc;								//2P 键位描述图片
extern IMAGE img_1P_naiwa_desc, img_2P_naiwa_desc;
extern IMAGE img_Naiwa_selector_background_left, img_Naiwa_selector_background_right;
extern IMAGE img_gravestone_left;						//朝向左的墓碑图片
extern IMAGE img_gravestone_right;						//朝向右的墓碑图片
extern IMAGE img_selector_tip;							//选角界面提示信息文本图片
extern IMAGE img_selector_background;					//选角界面背景图
extern IMAGE img_1P_selector_btn_idle_left;				//1P 向左选择按钮默认状态图片
extern IMAGE img_1P_selector_btn_idle_right;			//1P 向右选择按钮默认状态图片
extern IMAGE img_1P_selector_btn_down_left;				//1P 向左选择按钮按下状态图片
extern IMAGE img_1P_selector_btn_down_right;			//1P 向右选择按钮按下状态图片
extern IMAGE img_2P_selector_btn_idle_left;				//2P 向左选择按钮默认状态图片
extern IMAGE img_2P_selector_btn_idle_right;			//2P 向右选择按钮默认状态图片
extern IMAGE img_2P_selector_btn_down_left;				//2P 向左选择按钮按下状态图片
extern IMAGE img_2P_selector_btn_down_right;			//2P 向右选择按钮按下状态图片
extern IMAGE img_Jotaro_selector_background_left;		//选角界面朝向左的空条承太郎背景图片
extern IMAGE img_Jotaro_selector_background_right;		//选角界面朝向右的空条承太郎背景图片
extern IMAGE img_Kakyoin_selector_background_left;		//选角界面朝向左的花京院背景图片
extern IMAGE img_Kakyoin_selector_background_right;		//选角界面朝向右的花京院背景图片

extern Atlas atlas_Jotaro_idle_right;					//空条承太郎朝向右的站立动画图集	
extern Atlas atlas_Kakyoin_idle_right;					//花京院朝向右的站立动画图集
 
extern IMAGE img_avatar_Jotaro;							//空条承太郎头像图片
extern IMAGE img_avatar_Kakyoin;						//花京院头像图片

extern Atlas atlas_naiwa_idle_right;
extern IMAGE img_avatar_Naiwa;
extern IMAGE* img_player_1_avatar;
extern IMAGE* img_player_2_avatar;

extern SceneManager scene_manager;

class SelectorScene : public Scene
{
public:
	SelectorScene() = default;
	~SelectorScene() = default;

	void on_enter()
	{
		mciSendString(_T("play bgm_menu repeat"), NULL, 0, NULL);	// 进入选角继续菜单音乐，单独进入选角也能正常播放
		animation_Jotaro.set_atlas(&atlas_Jotaro_idle_right);
		animation_Kakyoin.set_atlas(&atlas_Kakyoin_idle_right);
		animation_Naiwa.set_atlas(&atlas_naiwa_idle_right);
		animation_Jotaro.set_interval(100);
		animation_Kakyoin.set_interval(100);
		animation_Naiwa.set_interval(100);

		static const int OFFSET_X = 50;

		pos_img_VS.x = (getwidth() - img_VS.getwidth()) / 2;
		pos_img_VS.y = (getheight() - img_VS.getheight()) / 2;
		pos_img_tip.x = (getwidth() - img_selector_tip.getwidth()) / 2;
		pos_img_tip.y = getheight() - 125;
		pos_img_1P.x = (getwidth() / 2 - img_1P.getwidth()) / 2 - OFFSET_X;
		pos_img_1P.y = 35;
		pos_img_2P.x = getwidth() / 2 + (getwidth() / 2 - img_2P.getwidth()) / 2 + OFFSET_X;
		pos_img_2P.y = pos_img_1P.y;
		pos_img_1P_desc.x = (getwidth() / 2 - img_1P_desc.getwidth()) / 2 - OFFSET_X;
		pos_img_1P_desc.y = getheight() - 225; // 两行提示位于选角框下方、确认提示上方
		pos_img_2P_desc.x = getwidth() / 2 + (getwidth() / 2 - img_2P_desc.getwidth()) / 2 + OFFSET_X;
		pos_img_2P_desc.y = pos_img_1P_desc.y;
		pos_img_1P_gravestone.x = (getwidth() / 2 - img_gravestone_right.getwidth()) / 2 - OFFSET_X;
		pos_img_1P_gravestone.y = pos_img_1P.y + img_1P.getheight() + 35;
		pos_img_2P_gravestone.x = getwidth() / 2 + (getwidth() / 2 - img_gravestone_left.getwidth()) / 2 + OFFSET_X;
		pos_img_2P_gravestone.y = pos_img_1P_gravestone.y;
		pos_animation_1P.x = (getwidth() / 2 - atlas_Jotaro_idle_right.get_image(0)->getwidth()) / 2 - OFFSET_X;
		pos_animation_1P.y = pos_img_1P_gravestone.y + 80;
		pos_animation_2P.x = getwidth() / 2 + (getwidth() / 2 - atlas_Jotaro_idle_right.get_image(0)->getwidth()) / 2 + OFFSET_X;
		pos_animation_2P.y = pos_animation_1P.y;
		pos_img_1P_name.y = pos_animation_1P.y + 135;
		pos_img_2P_name.y = pos_img_1P_name.y;
		pos_1P_selector_btn_left.x = pos_img_1P_gravestone.x - img_1P_selector_btn_idle_left.getwidth();
		pos_1P_selector_btn_left.y = pos_img_1P_gravestone.y + (img_gravestone_right.getheight() - img_1P_selector_btn_idle_left.getheight()) / 2;
		pos_1P_selector_btn_right.x = pos_img_1P_gravestone.x + img_gravestone_right.getwidth();
		pos_1P_selector_btn_right.y = pos_1P_selector_btn_left.y;
		pos_2P_selector_btn_left.x = pos_img_2P_gravestone.x - img_2P_selector_btn_idle_left.getwidth();
		pos_2P_selector_btn_left.y = pos_1P_selector_btn_left.y;
		pos_2P_selector_btn_right.x = pos_img_2P_gravestone.x + img_gravestone_left.getwidth();
		pos_2P_selector_btn_right.y = pos_1P_selector_btn_left.y;
	}

	void on_update(int delta)
	{
		animation_Jotaro.on_update(delta);
		animation_Kakyoin.on_update(delta);
		animation_Naiwa.on_update(delta);

		selector_background_scor11_offset_x += 5;
		if (selector_background_scor11_offset_x >= img_Jotaro_selector_background_left.getwidth())
			selector_background_scor11_offset_x = 0;
	}

	void on_draw(const Camera& camera)
	{
		IMAGE* img_p1_selector_background = nullptr;
		IMAGE* img_p2_selector_background = nullptr;

		switch (player_type_1)
		{
		case PlayerType::Jotaro:
			img_p1_selector_background = &img_Jotaro_selector_background_right;
			break;
		case PlayerType::Kakyoin:
			img_p1_selector_background = &img_Kakyoin_selector_background_right;
			break;
		case PlayerType::Naiwa:
			img_p1_selector_background = &img_Naiwa_selector_background_right;
			break;
		default:
			img_p1_selector_background = &img_Jotaro_selector_background_right;
			break;
		}

		switch (player_type_2)
		{
		case PlayerType::Jotaro:
			img_p2_selector_background = &img_Jotaro_selector_background_left;
			break;
		case PlayerType::Kakyoin:
			img_p2_selector_background = &img_Kakyoin_selector_background_left;
			break;
		case PlayerType::Naiwa:
			img_p2_selector_background = &img_Naiwa_selector_background_left;
			break;
		default:
			img_p2_selector_background = &img_Jotaro_selector_background_left;
			break;
		}

		putimage(0, 0, &img_selector_background);

		// 奶娃的新底纹为不透明白色图案，绘制时降到与现有角色底纹相同的亮度。
		const BYTE opacity_1 = player_type_1 == PlayerType::Naiwa ? 128 : 255;
		const BYTE opacity_2 = player_type_2 == PlayerType::Naiwa ? 128 : 255;
		blend_image_alpha(selector_background_scor11_offset_x - img_p1_selector_background->getwidth(), 0,
			img_p1_selector_background->getwidth(), img_p1_selector_background->getheight(), img_p1_selector_background, 0, 0, opacity_1);
		blend_image_alpha(selector_background_scor11_offset_x, 0,
			img_p1_selector_background->getwidth() - selector_background_scor11_offset_x,
			img_p1_selector_background->getheight(), img_p1_selector_background, 0, 0, opacity_1);
		blend_image_alpha(getwidth() - img_p2_selector_background->getwidth(), 0,
			img_p2_selector_background->getwidth() - selector_background_scor11_offset_x,
			img_p2_selector_background->getheight(), img_p2_selector_background, selector_background_scor11_offset_x, 0, opacity_2);
		blend_image_alpha(getwidth() - selector_background_scor11_offset_x, 0,
			img_p2_selector_background->getwidth(), img_p2_selector_background->getheight(), img_p2_selector_background, 0, 0, opacity_2);

		putimage_alpha(pos_img_VS.x, pos_img_VS.y, &img_VS);

		putimage_alpha(pos_img_1P.x, pos_img_1P.y, &img_1P);
		putimage_alpha(pos_img_2P.x, pos_img_2P.y, &img_2P);
		putimage_alpha(pos_img_1P_gravestone.x, pos_img_1P_gravestone.y, &img_gravestone_right);
		putimage_alpha(pos_img_2P_gravestone.x, pos_img_2P_gravestone.y, &img_gravestone_left);

		switch (player_type_1)
		{
		case PlayerType::Jotaro:
			animation_Jotaro.on_draw(camera, pos_animation_1P.x, pos_animation_1P.y);
			pos_img_1P_name.x = pos_img_1P_gravestone.x + (img_gravestone_right.getwidth() - textwidth(str_Jotaro_name)) / 2;
			outtextxy_shaded(pos_img_1P_name.x, pos_img_1P_name.y, str_Jotaro_name);
			break;
		case PlayerType::Kakyoin:
			animation_Kakyoin.on_draw(camera, pos_animation_1P.x, pos_animation_1P.y);
			pos_img_1P_name.x = pos_img_1P_gravestone.x + (img_gravestone_right.getwidth() - textwidth(str_Kakyoin_name)) / 2;
			outtextxy_shaded(pos_img_1P_name.x, pos_img_1P_name.y, str_Kakyoin_name);
			break;
		case PlayerType::Naiwa:
			animation_Naiwa.on_draw(camera, pos_animation_1P.x, pos_animation_1P.y);
			pos_img_1P_name.x = pos_img_1P_gravestone.x + (img_gravestone_right.getwidth() - textwidth(str_Naiwa_name)) / 2;
			outtextxy_shaded(pos_img_1P_name.x, pos_img_1P_name.y, str_Naiwa_name);
			break;
		}

		switch (player_type_2)
		{
		case PlayerType::Jotaro:
			animation_Jotaro.on_draw(camera, pos_animation_2P.x, pos_animation_2P.y);
			pos_img_2P_name.x = pos_img_2P_gravestone.x + (img_gravestone_left.getwidth() - textwidth(str_Jotaro_name)) / 2;
			outtextxy_shaded(pos_img_2P_name.x, pos_img_2P_name.y, str_Jotaro_name);
			break;
		case PlayerType::Kakyoin:
			animation_Kakyoin.on_draw(camera, pos_animation_2P.x, pos_animation_2P.y);
			pos_img_2P_name.x = pos_img_2P_gravestone.x + (img_gravestone_left.getwidth() - textwidth(str_Kakyoin_name)) / 2;
			outtextxy_shaded(pos_img_2P_name.x, pos_img_2P_name.y, str_Kakyoin_name);
			break;
		case PlayerType::Naiwa:
			animation_Naiwa.on_draw(camera, pos_animation_2P.x, pos_animation_2P.y);
			pos_img_2P_name.x = pos_img_2P_gravestone.x + (img_gravestone_left.getwidth() - textwidth(str_Naiwa_name)) / 2;
			outtextxy_shaded(pos_img_2P_name.x, pos_img_2P_name.y, str_Naiwa_name);
			break;
		}

		putimage_alpha(pos_1P_selector_btn_left.x, pos_1P_selector_btn_left.y,
			is_btn_1P_left_down ? &img_1P_selector_btn_down_left : &img_1P_selector_btn_idle_left);
		putimage_alpha(pos_1P_selector_btn_right.x, pos_1P_selector_btn_right.y,
			is_btn_1P_right_down ? &img_1P_selector_btn_down_right : &img_1P_selector_btn_idle_right);
		putimage_alpha(pos_2P_selector_btn_left.x, pos_2P_selector_btn_left.y,
			is_btn_2P_left_down ? &img_2P_selector_btn_down_left : &img_2P_selector_btn_idle_left);
		putimage_alpha(pos_2P_selector_btn_right.x, pos_2P_selector_btn_right.y,
			is_btn_2P_right_down ? &img_2P_selector_btn_down_right : &img_2P_selector_btn_idle_right);

		putimage_alpha(pos_img_1P_desc.x, pos_img_1P_desc.y,
			player_type_1 == PlayerType::Naiwa ? &img_1P_naiwa_desc : &img_1P_desc);
		putimage_alpha(pos_img_2P_desc.x, pos_img_2P_desc.y,
			player_type_2 == PlayerType::Naiwa ? &img_2P_naiwa_desc : &img_2P_desc);

		putimage_alpha(pos_img_tip.x, pos_img_tip.y, &img_selector_tip);
	}

	void on_input(const ExMessage& msg)
	{
		switch (msg.message)
		{
		case WM_KEYDOWN:
			switch (msg.vkcode)
			{
				// 'A'
			case 0x41:
				is_btn_1P_left_down = true;
				break;
				// 'D'
			case 0x44:
				is_btn_1P_right_down = true;
				break;
				// '+'
			case VK_LEFT:
				is_btn_2P_left_down = true;
				break;

			case VK_RIGHT:
				is_btn_2P_right_down = true;
				break;
			}
			break;
		case WM_KEYUP:
			switch (msg.vkcode)
			{
				// 'A'
			case 0x41:
				is_btn_1P_left_down = false;
				player_type_1 = (PlayerType)(((int)PlayerType::Invalid + (int)player_type_1 - 1) % (int)PlayerType::Invalid);
				mciSendString(_T("play ui_switch from 0"), NULL, 0, NULL);
				break;
				// 'D'
			case 0x44:
				is_btn_1P_right_down = false;
				player_type_1 = (PlayerType)(((int)player_type_1 + 1) % (int)PlayerType::Invalid);
				mciSendString(_T("play ui_switch from 0"), NULL, 0, NULL);
				break;
				// '<-'
			case VK_LEFT:
				is_btn_2P_left_down = false;
				player_type_2 = (PlayerType)(((int)PlayerType::Invalid + (int)player_type_2 - 1) % (int)PlayerType::Invalid);
				mciSendString(_T("play ui_switch from 0"), NULL, 0, NULL);
				break;
				// '->'
			case VK_RIGHT:
				is_btn_2P_right_down = false;
				player_type_2 = (PlayerType)(((int)player_type_2 + 1) % (int)PlayerType::Invalid);
				mciSendString(_T("play ui_switch from 0"), NULL, 0, NULL);
				break;
			case VK_RETURN:
				scene_manager.switch_to(SceneManager::SceneType::Game);
				mciSendString(_T("play ui_confirm from 0"), NULL, 0, NULL);
				break;
			}
			break;
		default:
			break;
		}
	}
	void on_exit()
	{
		switch (player_type_1)
		{
		case PlayerType:: Jotaro :
			player_1 = new JotaroPlayer();
			img_player_1_avatar = &img_avatar_Jotaro;
			break;
		case PlayerType:: Kakyoin :
			player_1 = new KakyoinPlayer();
			img_player_1_avatar = &img_avatar_Kakyoin;
			break;
		case PlayerType::Naiwa:
			player_1 = new NaiwaPlayer();
			img_player_1_avatar = &img_avatar_Naiwa;
			break;
		}
		player_1->set_id(PlayerID::P1);

		switch (player_type_2)
		{
		case PlayerType::Jotaro:
			player_2 = new JotaroPlayer();
			img_player_2_avatar = &img_avatar_Jotaro;
			break;
		case PlayerType::Kakyoin:
			player_2 = new KakyoinPlayer();
			img_player_2_avatar = &img_avatar_Kakyoin;
			break;
		case PlayerType::Naiwa:
			player_2 = new NaiwaPlayer();
			img_player_2_avatar = &img_avatar_Naiwa;
			break;
		}
		player_2->set_id(PlayerID::P2);

		mciSendString(_T("stop bgm_menu"), NULL, 0, NULL);
	}

private:
	enum class PlayerType
	{
		Jotaro = 0,
		Kakyoin,
		Naiwa,
		Invalid
	};

private:
	POINT pos_img_VS = { 0 };						//VS 艺术字图片位置
	POINT pos_img_tip = { 0 };						//选角界面提示信息文本图片位置
	POINT pos_img_1P = { 0 };						//1P 文本图片位置
	POINT pos_img_2P = { 0 };						//2P 文本图片位置
	POINT pos_img_1P_desc = { 0 };					//1P 键位描述图片位置
	POINT pos_img_2P_desc = { 0 };					//2P 键位描述图片位置
	POINT pos_img_1P_name = { 0 };					//1P 名称图片位置
	POINT pos_img_2P_name = { 0 };					//2P 名称图片位置
	POINT pos_animation_1P = { 0 };					//1P 动画位置
	POINT pos_animation_2P = { 0 };					//2P 动画位置
	POINT pos_img_1P_gravestone = { 0 };			//1P 墓碑图片位置
	POINT pos_img_2P_gravestone = { 0 };			//2P 墓碑图片位置
	POINT pos_1P_selector_btn_left = { 0 };			//1P 向左选择按钮位置
	POINT pos_1P_selector_btn_right = { 0 };		//1P 向右选择按钮位置
	POINT pos_2P_selector_btn_left = { 0 };			//2P 向左选择按钮位置
	POINT pos_2P_selector_btn_right = { 0 };		//2P 向右选择按钮位置

	Animation animation_Jotaro;						//空条承太郎动画
	Animation animation_Kakyoin;					//花京院动画
	Animation animation_Naiwa;

	PlayerType player_type_1 = PlayerType::Jotaro;	//1P 角色类型
	PlayerType player_type_2 = PlayerType::Kakyoin;	//2P 角色类型

	LPCTSTR str_Jotaro_name = _T("空条承太郎");		//空条承太郎名称
	LPCTSTR str_Kakyoin_name = _T("花京院典明");		//花京院典明名称
	LPCTSTR str_Naiwa_name = _T("奶娃");

	int selector_background_scor11_offset_x = 0;	//选角界面背景滚动偏移量

	bool is_btn_1P_left_down = false;				// 1P 向左切换按钮是否按下
	bool is_btn_1P_right_down = false;				//1P 向右切换按钮是否按下
	bool is_btn_2P_left_down = false;				//2P 向左切换按钮是否按下
	bool is_btn_2P_right_down = false;				// 2P 向右切换按钮是否按下

private:
	void outtextxy_shaded(int x, int y, LPCTSTR str)
	{
		settextcolor(RGB(45, 45, 45));
		outtextxy(x + 3, y + 3, str);
		settextcolor(RGB(255, 255, 255));
		outtextxy(x, y, str);
	}

};


#endif // !_SELECTOR_SCENE_H_
