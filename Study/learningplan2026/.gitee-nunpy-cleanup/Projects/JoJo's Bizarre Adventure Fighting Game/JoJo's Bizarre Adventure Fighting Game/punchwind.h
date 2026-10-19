#ifndef _PUNCHWIND_H_
#define _PUNCHWIND_H_

#include "attack.h"

extern Atlas atlas_star_platinum_punch_wind_left;
extern Atlas atlas_star_platinum_punch_wind_right;

// 蓄力拳的独立拳风：出拳时生成，位置由技能设置，不作为飞行弹丸移动。
class PunchWindBullet : public AttackArea
{
public:
	PunchWindBullet()
	{
		size.x = 280, size.y = 100;	// 初始判定范围，可根据技能距离使用 set_size 调整
		set_attack_kind(AttackKind::Special);	// 技能不触发普攻累计保护
		set_hit_shake(12.0f, 180);
		damage = 30;

		set_duration(360);
		set_hit_interval(0);			// 一拳只结算一次伤害
		set_hit_window(0, 210);		// 尾部残影与消散阶段只显示，不继续造成伤害
		set_atlas(&atlas_star_platinum_punch_wind_left,
			&atlas_star_platinum_punch_wind_right, 40, false);
		set_frame_durations({ 20, 20, 30, 80, 30, 70, 50, 60 });
		set_draw_offset(-20, -30);	// 320×160 图片对齐判定区域
	}

	~PunchWindBullet() = default;
};

#endif // !_PUNCHWIND_H_
