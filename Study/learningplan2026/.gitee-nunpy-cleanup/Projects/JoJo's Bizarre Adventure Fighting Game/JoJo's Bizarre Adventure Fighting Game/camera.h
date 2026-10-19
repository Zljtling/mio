#ifndef _CAMERA_H_
#define _CAMERA_H_

#include "vector2.h"
#include "game_config.h"
#include <algorithm>
#include <cmath>

class Camera
{
public:
	const Vector2& get_position() const { return position; }
	float get_zoom() const { return zoom; }
	int get_view_width() const { return (int)std::ceil(GAME_WINDOW_WIDTH / zoom); }
	int get_view_height() const { return (int)std::ceil(GAME_WINDOW_HEIGHT / zoom); }

	void reset()
	{
		base_position = position = Vector2();
		zoom = 1.0f;
		shake_remaining = shake_duration = shake_elapsed = 0;
		shake_strength = 0;
	}

	void follow(const Vector2& first, const Vector2& second, int delta, bool snap = false)
	{
		const float left = (std::max)(0.0f, (std::min)(first.x, second.x) - 180.0f);
		const float right = (std::min)((float)GAME_WORLD_WIDTH, (std::max)(first.x, second.x) + 308.0f);
		const float top = (std::max)(0.0f, (std::min)(first.y, second.y) - 150.0f);
		const float bottom = (std::min)((float)GAME_WORLD_HEIGHT, (std::max)(first.y, second.y) + 276.0f);
		const float desired_zoom = (std::clamp)((std::min)(GAME_WINDOW_WIDTH / (right - left),
			GAME_WINDOW_HEIGHT / (bottom - top)), 0.5f, 1.0f);
		const float smoothing = snap ? 1.0f : 1.0f - std::exp(-delta / 160.0f);
		// 拉远立即保证双方可见，靠近时平滑恢复。
		zoom = snap || desired_zoom < zoom ? desired_zoom : zoom + (desired_zoom - zoom) * smoothing;
		const float view_width = (float)get_view_width(), view_height = (float)get_view_height();
		const float target_x = (std::clamp)((left + right - view_width) / 2, 0.0f, GAME_WORLD_WIDTH - view_width);
		const float target_y = (std::clamp)((top + bottom - view_height) / 2, 0.0f, GAME_WORLD_HEIGHT - view_height);
		base_position.x += (target_x - base_position.x) * smoothing;
		base_position.y += (target_y - base_position.y) * smoothing;
		base_position.x = (std::clamp)(base_position.x,
			(std::max)(0.0f, right - view_width), (std::min)(left, GAME_WORLD_WIDTH - view_width));
		base_position.y = (std::clamp)(base_position.y,
			(std::max)(0.0f, bottom - view_height), (std::min)(top, GAME_WORLD_HEIGHT - view_height));
		position = base_position;
	}

	void on_update(int delta)
	{
		shake_remaining = (std::max)(0, shake_remaining - delta);
		shake_elapsed += delta;
		position = base_position;
		if (shake_remaining == 0) { shake_strength = 0; return; }
		const float amplitude = shake_strength * shake_remaining / shake_duration / zoom;
		position.x += std::sin(shake_elapsed * 0.13f) * amplitude;
		position.y += std::sin(shake_elapsed * 0.19f + 1.1f) * amplitude * 0.65f;
	}
	 
	void shake(float strength, int duration)
	{
		if (strength <= 0 || duration <= 0) return;
		const float current = shake_duration > 0 ? shake_strength * shake_remaining / shake_duration : 0;
		shake_strength = (std::min)(14.0f, (std::max)(current, strength));
		shake_remaining = shake_duration = (std::max)(shake_remaining, duration);
	}


private:
	Vector2 base_position, position;
	float zoom = 1.0f, shake_strength = 0;
	int shake_remaining = 0, shake_duration = 0, shake_elapsed = 0;
};


#endif // !_CAMERA_H_
