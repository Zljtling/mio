#ifndef _STATUS_BAR_H_
#define _STATUS_BAR_H_

#include "util.h"
#include <algorithm>
#include <cmath>



class StatusBar
{
public:
	StatusBar() = default;
	~StatusBar() = default;

	void set_avatar(IMAGE* img)
	{
		img_avatar = img;
	}

	void set_position(int x, int y)
	{
		position.x = x, position.y = y;
	}

	void set_hp(int val)
	{
		hp = val;

	}

	void set_max_hp(int val) { max_hp = val > 0 ? val : 1; }	// 角色拥有独立的生命上限

	void set_mp(int val)
	{
		mp = val;
	}
	void set_skill_costs(int special, int ultimate)
	{
		special_cost = (std::max)(0, special);
		ultimate_cost = (std::max)(0, ultimate);
	}
	void set_ultimate_ready(bool ready) { ultimate_ready = ready; }
	void set_ultimate_key(LPCTSTR key) { ultimate_key = key; }
	void on_update(int delta) { pulse_ms = (pulse_ms + delta) % 1600; }

	void on_draw()
	{
		putimage_alpha(position.x, position.y, img_avatar);

		setfillcolor(RGB(5, 5, 5));
		solidroundrect(position.x + 100, position.y + 10, position.x + 100 + width + 3 * 2, position.y + 36, 8, 8);
		solidroundrect(position.x + 100, position.y + 45, position.x + 100 + width + 3 * 2, position.y + 71, 8, 8);
		setfillcolor(RGB(67, 47, 47));
		solidroundrect(position.x + 100, position.y + 10, position.x + 100 + width + 3, position.y + 33, 8, 8);
		solidroundrect(position.x + 100, position.y + 45, position.x + 100 + width + 3, position.y + 68, 8, 8);
	
		float hp_bar_width = width * min(max_hp, max(0, hp)) / (float)max_hp;
		float mp_bar_width = width * min(100, max(0, mp)) / 100.0f;
		setfillcolor(RGB(197, 61, 67));
		solidroundrect(position.x + 100, position.y + 10, position.x + 100 + (int)hp_bar_width + 3, position.y + 33, 8, 8);
		setfillcolor(RGB(83, 131, 195));
		solidroundrect(position.x + 100, position.y + 45, position.x + 100 + (int)mp_bar_width + 3, position.y + 68, 8, 8);

		DebugDrawState draw_state;
		const int bar_x = position.x + 103;
		const int bar_y = position.y + 45;
		const bool enough_energy = mp >= ultimate_cost;
		const float pulse = (1.0f + std::sin(pulse_ms * 0.003927f)) * 0.5f;
		for (int value = 10; value < 100; value += 10)
		{
			const int x = bar_x + width * value / 100;
			setlinecolor(RGB(107, 160, 215));
			line(x, bar_y + 17, x, bar_y + 21);
		}
		const int special_x = bar_x + width * (std::min)(100, special_cost) / 100;
		const int ultimate_x = bar_x + width * (std::min)(100, ultimate_cost) / 100;
		setlinecolor(RGB(87, 225, 235));
		line(special_x, bar_y + 1, special_x, bar_y + 23);
		setfillcolor(RGB(87, 225, 235));
		solidcircle(special_x, bar_y - 4, 3);
		const COLORREF ultimate_color = ultimate_ready
			? RGB(255, 225, 105 + (int)(100 * pulse))
			: (enough_energy ? RGB(200, 165, 105) : RGB(230, 191, 83));
		setlinecolor(ultimate_color);
		line(ultimate_x, bar_y, ultimate_x, bar_y + 23);
		setfillcolor(ultimate_color);
		solidcircle(ultimate_x, bar_y - 5, ultimate_ready ? 6 : 4);
		if (ultimate_ready)
		{
			const int shine_x = bar_x + (int)(mp_bar_width * pulse);
			setlinecolor(RGB(176, 222, 255));
			line(shine_x, bar_y + 3, shine_x, bar_y + 20);
			setlinecolor(ultimate_color);
			rectangle(position.x + 99, bar_y - 1, bar_x + width + 3, bar_y + 25);
		}
		setbkmode(TRANSPARENT);
		settextstyle(16, 0, _T("Microsoft YaHei"));
		settextcolor(ultimate_ready ? ultimate_color : RGB(198, 207, 223));
		TCHAR hint[64] = {};
		if (ultimate_ready)
			_stprintf_s(hint, _T("%s 大招就绪"), ultimate_key);
		else if (enough_energy)
			_stprintf_s(hint, _T("%s 大招暂不可用"), ultimate_key);
		else
			_stprintf_s(hint, _T("%s 大招还需 %d 能量"), ultimate_key, ultimate_cost - mp);
		outtextxy(bar_x, position.y + 73, hint);
	}
	
private:
	const int width = 275;			//状态条宽度
	int max_hp = 100;
	int hp = 0;						//需要显示的生命值
	int mp = 0;						//需要显示的能量值
	POINT position = { 0 };			//在窗口中显示的位置
	int special_cost = 0;
	int ultimate_cost = 100;
	bool ultimate_ready = false;
	int pulse_ms = 0;
	LPCTSTR ultimate_key = _T("Q");
	IMAGE* img_avatar = nullptr;	//在窗口中显示的位置
};

#endif // !_STATUS_BAR_H_
