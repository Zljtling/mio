#ifndef _ORABARRAGE_H_
#define _ORABARRAGE_H_

#include "attack.h"

extern Atlas atlas_star_platinum_ora_barrage_afterimages_left;
extern Atlas atlas_star_platinum_ora_barrage_afterimages_right;

// 欧拉连续拳只显示独立拳头虚影，白金之星本体由角色技能动画显示。
class OraBarrageBullet : public AttackArea
{
public:
	OraBarrageBullet()
	{
		size.x = 172, size.y = 114;	// 虚影的初始判定范围，避免把整张透明画布作为伤害区域
		set_attack_kind(AttackKind::Ultimate);	// 技能不触发普攻累计保护
		set_hit_shake(6.0f, 100);
		damage = 5;					// 每次命中的伤害，整体强度还取决于命中间隔

		set_duration(480);			// 24 帧一轮；需要持续连打时可使用 set_duration 延长
		set_hit_interval(80);		// 每 80ms 最多命中一次，不按虚影数量计算伤害
		set_atlas(&atlas_star_platinum_ora_barrage_afterimages_left,
			&atlas_star_platinum_ora_barrage_afterimages_right, 20, true);
		set_draw_offset(-10, -8);
	}

	~OraBarrageBullet() = default;
};

#endif // !_ORABARRAGE_H_
