#ifndef _EMERALD_H_
#define _EMERALD_H_

#include "attack.h"

extern Atlas atlas_emerald_burst;

// 绿宝石范围爆炸：直接在指定位置生成，没有独立的飞行炸弹本体。
class EmeraldBullet : public AttackArea
{
public:
	EmeraldBullet()
	{
		size.x = 40, size.y = 40;	// 小范围爆炸判定，排除素材外围透明区域
		set_attack_kind(AttackKind::Special);	// 技能不触发普攻累计保护
		set_hit_shake(7.0f, 120);
		damage = 10;				// 每个绿宝石爆炸造成十点伤害

		set_duration(200);
		set_hit_interval(0);		// 一次爆炸对指定目标只命中一次
		set_hit_window(0, 150);	// 最后一帧消散时不再造成新的伤害
		set_atlas(&atlas_emerald_burst, &atlas_emerald_burst, 50, false);
		set_frame_durations({ 40, 50, 60, 50 });
		set_draw_offset(-4, -4);	// 48×48 图片与 40×40 判定区域中心对齐
	}

	~EmeraldBullet() = default;

	void set_center(float x, float y)
	{
		set_position(x - size.x / 2, y - size.y / 2);	// 便于在多个落点生成爆炸
	}
};

#endif // !_EMERALD_H_
