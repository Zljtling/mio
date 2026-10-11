#ifndef _QUILL_BULLET_H_
#define _QUILL_BULLET_H_

#include "bullet.h"
#include "audio.h"
#include "animation.h"

#include <mmsystem.h>
#include <cstdint>

#pragma comment(lib, "winmm.lib")

extern IMAGE img_quill_left;
extern IMAGE img_quill_right;
extern Atlas atlas_quill_break;

class QuillBullet : public Bullet
{
public:
	QuillBullet()
	{
		size.x = 32, size.y = 16;	// 羽毛笔实际素材尺寸

		set_attack_kind(AttackKind::Normal);	// 羽毛笔按普攻规则累计保护
		damage = 3;

		animation_break.set_atlas(&atlas_quill_break);
		animation_break.set_interval(40);	// 四帧碎裂动画，共 160ms
		animation_break.set_loop(false);
		animation_break.set_callback([&]() { can_remove = true; });
	}
	~QuillBullet() = default;

	void on_collide() override
	{
		if (has_collided || can_remove)
			return;

		has_collided = true;		// 同一颗羽毛笔只触发一次命中
		valid = false;
		velocity.x = 0, velocity.y = 0;
		animation_break.reset();
		timer_break = 0;
		if (atlas_quill_break.get_size() <= 0)
			can_remove = true;	// 未加载动画时直接回收，避免访问空图集

		Bullet::on_collide();

		play_player_sound(get_source_id(), _T("kakyoin_quill_hit"), 0, -1, false);	// 同一颗羽毛笔只播放一次命中碎裂声

	}

	void on_update(int delta) override
	{		
		if (get_time_stopped() || can_remove || delta <= 0)
			return;

		if (valid)
		{
			position += velocity * (float)delta;
			if (check_if_exceeds_screen())
				remove();
		}
		else
		{
			// 命中后停止飞行；按固定步长更新，保留每次更新剩余的毫秒数。
			if (atlas_quill_break.get_size() <= 0)
			{
				remove();
				return;
			}
			timer_break += delta;
			while (timer_break >= 40 && !can_remove)
			{
				timer_break -= 40;
				animation_break.on_update(40);
			}
		}
	}

	void on_draw(const Camera& camera) const override
	{
		if (can_remove)
			return;

		if (valid)
		{
			IMAGE* image = velocity.x < 0 ? &img_quill_left : &img_quill_right;
			if (image->getwidth() > 0 && image->getheight() > 0)
				putimage_alpha(camera, (int)position.x, (int)position.y, image);
		}
		else if (atlas_quill_break.get_size() > 0)
		{
			IMAGE* frame = atlas_quill_break.get_image(0);
			int x = (int)(position.x + size.x / 2 - frame->getwidth() / 2.0f);
			int y = (int)(position.y + size.y / 2 - frame->getheight() / 2.0f);
			animation_break.on_draw(camera, x, y);	// 碎裂特效对齐羽毛笔中心
		}
	}

private:
	Animation animation_break;	//羽毛笔爆裂动画
	std::int64_t timer_break = 0;	// 碎裂动画累计时间
	bool has_collided = false;	// 是否已经触发命中回调和音效
};

#endif // !_QUILL_BULLET_H_
