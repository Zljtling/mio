#ifndef _TIME_STOP_EFFECT_H_
#define _TIME_STOP_EFFECT_H_

#include "util.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

// 时停波以发动时的位置为中心扩散，圆内场景转换为保留原始明暗的灰度。
class TimeStopEffect
{
public:
	void start(const Vector2& new_center, int new_duration, float new_max_radius, int new_expansion_ms = 700)
	{
		reset();
		if (!std::isfinite(new_center.x) || !std::isfinite(new_center.y)
			|| !std::isfinite(new_max_radius) || new_max_radius <= 0 || new_duration <= 0)
			return;

		center = new_center;
		duration = new_duration;
		max_radius = new_max_radius;
		expansion_ms = (std::max)(new_expansion_ms, 0);
		active = true;
		fade_elapsed = 0;
		// 非正的扩散时间表示直接覆盖范围，避免除以零。
		radius = expansion_ms > 0 ? 0 : max_radius;
	}

	void reset()
	{
		active = false;
		fade_elapsed = recovery_ms;
		center = Vector2();
		duration = 0;
		elapsed = 0;
		expansion_ms = 0;
		max_radius = 0;
		radius = 0;
	}

	// 冻结与欧拉状态按五秒结束；画面单独用 150ms 恢复颜色，不延长角色冻结。
	void finish()
	{
		if (!active) return;
		active = false;
		fade_elapsed = 0;
	}

	bool get_visible() const { return active || fade_elapsed < recovery_ms; }

	void on_update(int delta)
	{
		if (delta <= 0) return;
		if (!active)
		{
			fade_elapsed = (int)(std::min)((std::int64_t)fade_elapsed + delta, (std::int64_t)recovery_ms);
			return;
		}
		const std::int64_t next = (std::int64_t)elapsed + delta;
		elapsed = (int)(std::min)(next, (std::int64_t)duration);
		const double progress = expansion_ms > 0
			? (std::min)((double)elapsed / expansion_ms, 1.0) : 1.0;
		radius = (float)(max_radius * progress);
		if (elapsed >= duration)
		{
			finish();
			fade_elapsed = (int)(std::min)(next - duration, (std::int64_t)recovery_ms);
		}
	}

	bool get_active() const
	{
		return active;
	}

	int get_remaining() const
	{
		return active ? duration - elapsed : 0;
	}

	int get_elapsed() const
	{
		return elapsed;
	}

	float get_radius() const
	{
		return active ? radius : 0;
	}

	const Vector2& get_center() const
	{
		return center;
	}

	bool check_collision(const Vector2& position, const Vector2& size) const
	{
		if (!active || !std::isfinite(position.x) || !std::isfinite(position.y)
			|| !std::isfinite(size.x) || !std::isfinite(size.y) || size.x <= 0 || size.y <= 0)
			return false;

		// 最近点位于角色的碰撞箱上，圆周接触边界时即视为波纹已经抵达角色。
		const double closest_x = std::clamp((double)center.x, (double)position.x, (double)position.x + size.x);
		const double closest_y = std::clamp((double)center.y, (double)position.y, (double)position.y + size.y);
		const double dx = closest_x - center.x;
		const double dy = closest_y - center.y;
		return dx * dx + dy * dy <= (double)radius * radius;
	}

	void apply_grayscale(IMAGE* image, const Camera& camera, bool transparent_layer = false) const
	{
		apply_grayscale_at(image, camera.get_position(), transparent_layer);
	}

	// 源精灵图的局部坐标使用其左上角世界位置，过滤后仍沿用原本的 AlphaBlend 绘制。
	void apply_grayscale_at(IMAGE* image, const Vector2& world_origin, bool transparent_layer = true) const
	{
		if (active) apply_gray_strength(image, world_origin, transparent_layer, 1.0f);
	}

	// 背景在波前做径向伸缩；人物只做变色，避免扭曲精灵的手脚和衣领。
	void apply_visual_at(IMAGE* image, const Vector2& world_origin, bool background = false) const
	{
		if (!get_visible() || !image || image->getwidth() <= 0 || image->getheight() <= 0) return;
		const bool expanding = active && expansion_ms > 0 && elapsed < expansion_ms;
		if (expanding && background) apply_ripple(image, world_origin, true);
		const float gray_strength = active ? 1.0f : 1.0f - (float)fade_elapsed / recovery_ms;
		apply_gray_strength(image, world_origin, true, gray_strength);
		if (expanding) apply_ripple(image, world_origin, false);
	}

private:
	void apply_gray_strength(IMAGE* image, const Vector2& world_origin, bool transparent_layer, float strength) const
	{
		if (!image || image->getwidth() <= 0 || image->getheight() <= 0)
			return;

		const double screen_x = (double)center.x - world_origin.x;
		const double screen_y = (double)center.y - world_origin.y;
		if (!std::isfinite(screen_x) || !std::isfinite(screen_y))
			return;

		DWORD* buffer = GetImageBuffer(image);
		if (!buffer)
			return;

		const int width = image->getwidth();
		const int height = image->getheight();
		const double radius_squared = (double)radius * radius;
		const double first_row = (std::max)(0.0, std::ceil(screen_y - radius));
		const double last_row = (std::min)((double)height - 1, std::floor(screen_y + radius));
		if (first_row > last_row)
			return;

		// 只扫描圆与图片相交的行，并进一步裁剪每一行的横向覆盖区间。
		for (int y = (int)first_row; y <= (int)last_row; y++)
		{
			const double dy = y - screen_y;
			const double row_radius_squared = radius_squared - dy * dy;
			if (row_radius_squared < 0)
				continue;

			const double row_radius = std::sqrt(row_radius_squared);
			const double first_column = (std::max)(0.0, std::ceil(screen_x - row_radius));
			const double last_column = (std::min)((double)width - 1, std::floor(screen_x + row_radius));
			if (first_column > last_column)
				continue;

			DWORD* row = buffer + (std::size_t)y * width;
			for (int x = (int)first_column; x <= (int)last_column; x++)
			{
				const DWORD pixel = row[x];
				const DWORD alpha = pixel & 0xFF000000;
				if (transparent_layer && alpha == 0)
					continue;

				// EasyX 缓冲区为 0xAARRGGBB；按亮度加权，而不是直接把颜色变成纯黑或纯白。
				const DWORD red = (pixel >> 16) & 0xFF;
				const DWORD green = (pixel >> 8) & 0xFF;
				const DWORD blue = pixel & 0xFF;
				const DWORD luminance = (77 * red + 150 * green + 29 * blue + 128) >> 8;
				if (strength >= 1.0f)
				{
					row[x] = alpha | (luminance << 16) | (luminance << 8) | luminance;
					continue;
				}
				const DWORD r = (DWORD)std::lround(red + (luminance - (double)red) * strength);
				const DWORD g = (DWORD)std::lround(green + (luminance - (double)green) * strength);
				const DWORD b = (DWORD)std::lround(blue + (luminance - (double)blue) * strength);
				row[x] = alpha | (r << 16) | (g << 8) | b;
			}
		}
	}

	static DWORD tint(DWORD pixel, int red, int green, int blue, float amount)
	{
		const DWORD alpha = pixel >> 24;
		if (!alpha) return pixel;
		// 色彩叠加仍使用预乘 Alpha，半透明边缘不会发黑或变成不透明方块。
		const int weight = std::clamp((int)(amount * 256 + 0.5f), 0, 256);
		const DWORD r = ((((pixel >> 16) & 255) * (256 - weight) + red * alpha / 255 * weight + 128) >> 8);
		const DWORD g = ((((pixel >> 8) & 255) * (256 - weight) + green * alpha / 255 * weight + 128) >> 8);
		const DWORD b = (((pixel & 255) * (256 - weight) + blue * alpha / 255 * weight + 128) >> 8);
		return (alpha << 24) | (r << 16) | (g << 8) | b;
	}

	void apply_ripple(IMAGE* image, const Vector2& origin, bool distort) const
	{
		const double cx = (double)center.x - origin.x, cy = (double)center.y - origin.y;
		if (!std::isfinite(cx) || !std::isfinite(cy)) return;
		const int width = image->getwidth(), height = image->getheight();
		DWORD* pixels = GetImageBuffer(image);
		if (!pixels) return;
		std::vector<DWORD> source;
		if (distort) source.assign(pixels, pixels + (std::size_t)width * height);
		const double band = 38.0;
		const double inner = (std::max)(0.0, radius - band), outer = radius + band;
		const double first = (std::max)(0.0, std::ceil(cy - outer));
		const double last = (std::min)((double)height - 1, std::floor(cy + outer));
		if (first > last) return;
		for (int y = (int)first; y <= (int)last; ++y)
		{
			const double dy = y - cy;
			const double span = std::sqrt((std::max)(0.0, outer * outer - dy * dy));
			const double left = (std::max)(0.0, std::ceil(cx - span));
			const double right = (std::min)((double)width - 1, std::floor(cx + span));
			if (left > right) continue;
			for (int x = (int)left; x <= (int)right; ++x)
			{
				const double dx = x - cx, squared = dx * dx + dy * dy;
				if (squared < inner * inner) continue;
				const double distance = std::sqrt(squared), offset = distance - radius;
				if (std::abs(offset) >= band || distance < 1) continue;
				DWORD& pixel = pixels[(std::size_t)y * width + x];
				if (distort)
				{
					// 同一世界坐标的波前伸缩，幅度最高 8px，取样限制在背景边界内。
					const double shift = std::sin(offset / band * 3.141592653589793) * 8;
					const int sx = std::clamp((int)(x + dx / distance * shift + 0.5), 0, width - 1);
					const int sy = std::clamp((int)(y + dy / distance * shift + 0.5), 0, height - 1);
					pixel = source[(std::size_t)sy * width + sx];
				}
				else
				{
					const float glow = (float)(1 - std::abs(offset) / band) * 0.42f;
					pixel = offset < 0 ? tint(pixel, 150, 95, 255, glow) : tint(pixel, 110, 220, 255, glow);
				}
			}
		}
		if (!distort && elapsed < 90)
		{
			const float flash = 0.16f * (1 - elapsed / 90.0f);
			for (std::size_t i = 0; i < (std::size_t)width * height; ++i)
				pixels[i] = tint(pixels[i], 245, 235, 255, flash);
		}
	}

public:
	void draw_ring(const Camera& camera) const
	{
		if (!active || expansion_ms <= 0 || elapsed >= expansion_ms + 160 || getwidth() <= 0 || getheight() <= 0) return;
		const double x = (double)center.x - camera.get_position().x;
		const double y = (double)center.y - camera.get_position().y;
		if (!std::isfinite(x) || !std::isfinite(y)) return;
		const double far_x = (std::max)(std::abs(x), std::abs(x - getwidth()));
		const double far_y = (std::max)(std::abs(y), std::abs(y - getheight()));
		const double far_radius = std::sqrt(far_x * far_x + far_y * far_y);
		DebugDrawState draw_state;
		for (int layer = 2; layer >= 0; --layer)
		{
			const int age = elapsed - layer * 80;
			if (age <= 0) continue;
			const double r = max_radius * (std::min)(age / (double)expansion_ms, 1.0);
			if (r > far_radius + 12 || x + r < -12 || x - r > getwidth() + 12
				|| y + r < -12 || y - r > getheight() + 12
				|| std::abs(x) + r > (std::numeric_limits<int>::max)() - 16.0
				|| std::abs(y) + r > (std::numeric_limits<int>::max)() - 16.0) continue;
			const int ix = (int)std::lround(x), iy = (int)std::lround(y), ir = (int)std::lround(r);
			setlinestyle(PS_SOLID, layer == 0 ? 9 : 3);
			setlinecolor(layer == 0 ? RGB(90, 45, 150) : RGB(115 - layer * 20, 80, 155));
			circle(ix, iy, ir);
			setlinestyle(PS_SOLID, layer == 0 ? 5 : 1);
			setlinecolor(layer == 0 ? RGB(160, 205, 255) : RGB(140, 155, 200));
			circle(ix, iy, ir);
			if (layer == 0)
			{
				setlinestyle(PS_SOLID, 2);
				setlinecolor(RGB(255, 240, 180));
				circle(ix, iy, ir);
			}
		}
	}

private:
	Vector2 center;
	bool active = false;
	static constexpr int recovery_ms = 150;
	int fade_elapsed = recovery_ms;		// 技能结束后的颜色恢复计时，与冻结状态分开
	int duration = 0;
	int elapsed = 0;
	int expansion_ms = 0;
	float max_radius = 0;
	float radius = 0;
};

#endif // !_TIME_STOP_EFFECT_H_
