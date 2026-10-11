#ifndef _UTIL_H_
#define _UTIL_H_

#include "camera.h"

#include<graphics.h>
#include <algorithm>
#include <cstdint>
#include <functional>
#include <utility>

#pragma comment(lib,"MSIMG32.LIB")

using ImageDrawFilter = std::function<void(IMAGE&, int, int)>;

inline ImageDrawFilter*& get_image_draw_filter()
{
	static ImageDrawFilter* filter = nullptr;
	return filter;
}

// 每个对象绘制时临时设置滤镜；嵌套的空滤镜用于保留承太郎与白金之星的原色。
class ScopedImageDrawFilter
{
public:
	explicit ScopedImageDrawFilter(ImageDrawFilter new_filter = {})
		: filter(std::move(new_filter)), previous_filter(get_image_draw_filter())
	{
		get_image_draw_filter() = &filter;
	}

	~ScopedImageDrawFilter()
	{
		get_image_draw_filter() = previous_filter;
	}

	ScopedImageDrawFilter(const ScopedImageDrawFilter&) = delete;
	ScopedImageDrawFilter& operator=(const ScopedImageDrawFilter&) = delete;
	ScopedImageDrawFilter(ScopedImageDrawFilter&&) = delete;
	ScopedImageDrawFilter& operator=(ScopedImageDrawFilter&&) = delete;

private:
	ImageDrawFilter filter;
	ImageDrawFilter* previous_filter = nullptr;
};

inline void blend_image_alpha(int dst_x, int dst_y, int width, int height, IMAGE* img,
	int src_x = 0, int src_y = 0, BYTE opacity = 255)
{
	if (!img || width <= 0 || height <= 0 || opacity == 0
		|| img->getwidth() <= 0 || img->getheight() <= 0)
		return;

	ImageDrawFilter* filter = get_image_draw_filter();
	if (!filter || !*filter)
	{
		AlphaBlend(GetImageHDC(GetWorkingImage()), dst_x, dst_y, width, height,
			GetImageHDC(img), src_x, src_y, width, height,
			{ AC_SRC_OVER, 0, opacity, AC_SRC_ALPHA });
		return;
	}

	DWORD* source = GetImageBuffer(img);
	if (!source)
		return;
	IMAGE filtered(width, height);
	DWORD* destination = GetImageBuffer(&filtered);
	if (!destination)
		return;
	std::fill_n(destination, (size_t)width * height, 0u);	// 裁剪范围外保持透明

	// 用较宽的整数计算边界，避免负裁剪坐标与极大坐标相加时溢出。
	std::int64_t left = (std::max)((std::int64_t)0, (std::int64_t)src_x);
	std::int64_t top = (std::max)((std::int64_t)0, (std::int64_t)src_y);
	std::int64_t right = (std::min)((std::int64_t)img->getwidth(), (std::int64_t)src_x + width);
	std::int64_t bottom = (std::min)((std::int64_t)img->getheight(), (std::int64_t)src_y + height);
	if (left < right && top < bottom)
	{
		for (std::int64_t y = top; y < bottom; y++)
		{
			const DWORD* source_row = source + (size_t)y * img->getwidth() + (size_t)left;
			DWORD* destination_row = destination + (size_t)(y - src_y) * width + (size_t)(left - src_x);
			std::copy_n(source_row, (size_t)(right - left), destination_row);
		}
	}
	(*filter)(filtered, dst_x, dst_y);	// 只修改临时源图，保留素材原像素与 alpha
	AlphaBlend(GetImageHDC(GetWorkingImage()), dst_x, dst_y, width, height,
		GetImageHDC(&filtered), 0, 0, width, height,
		{ AC_SRC_OVER, 0, opacity, AC_SRC_ALPHA });
}

inline void putimage_alpha(int dst_x, int dst_y, IMAGE* img)
{
	if (img)
		blend_image_alpha(dst_x, dst_y, img->getwidth(), img->getheight(), img);
}

inline void putimage_alpha(const Camera& camera, int dst_x, int dst_y, IMAGE* img)
{
	if (!img)
		return;
	const Vector2& pos_camera = camera.get_position();
	blend_image_alpha((int)(dst_x - pos_camera.x), (int)(dst_y - pos_camera.y),
		img->getwidth(), img->getheight(), img);
}

inline void putimage_alpha(int dst_x, int dst_y, int width, int height, IMAGE* img, int src_x, int src_y)
{
	if (!img)
		return;
	int w = width > 0 ? width : img->getwidth();
	int h = height > 0 ? height : img->getheight();
	blend_image_alpha(dst_x, dst_y, w, h, img, src_x, src_y);
}

inline void flip_image(IMAGE* src, IMAGE* dst)
{
	int w = src->getwidth();
	int h = src->getheight();
	Resize(dst, w, h);
	DWORD* src_buffer = GetImageBuffer(src);
	DWORD* dst_buffer = GetImageBuffer(dst);
	for (int y = 0; y < h; y++)
	{
		for (int x = 0; x < w; x++)
		{
			int idx_src = y * w + x;
			int idx_dst = y * w + (w - x - 1);
			dst_buffer[idx_dst] = src_buffer[idx_src];
		}
	}
}

inline void line(const Camera& camera, int x1, int y1, int x2, int y2)
{
	const Vector2& pos_camera = camera.get_position();
	line((int)(x1 - pos_camera.x), (int)(y1 - pos_camera.y), (int)(x2 - pos_camera.x), (int)(y2 - pos_camera.y));
}

inline void rectangle(const Camera& camera, int left, int top, int right, int bottom)
{
	const Vector2& pos_camera = camera.get_position();
	rectangle((int)(left - pos_camera.x), (int)(top - pos_camera.y),
		(int)(right - pos_camera.x), (int)(bottom - pos_camera.y));
}

inline void solidcircle(const Camera& camera, int x, int y, int radius)
{
	const Vector2& pos_camera = camera.get_position();
	solidcircle((int)(x - pos_camera.x), (int)(y - pos_camera.y), radius);
}

// 调试绘制结束后恢复画笔和文字设置，避免影响下一帧或其他场景。
class DebugDrawState
{
public:
	DebugDrawState()
	{
		line_color = getlinecolor();
		fill_color = getfillcolor();
		text_color = gettextcolor();
		background_mode = getbkmode();
		getlinestyle(&line_style);
		gettextstyle(&text_style);
		setlinestyle(PS_SOLID, 1);
		settextstyle(16, 0, _T("Microsoft YaHei"));
		setbkmode(TRANSPARENT);
	}

	~DebugDrawState()
	{
		setlinecolor(line_color);
		setfillcolor(fill_color);
		settextcolor(text_color);
		setbkmode(background_mode);
		setlinestyle(&line_style);
		settextstyle(&text_style);
	}

private:
	COLORREF line_color, fill_color, text_color;
	int background_mode;
	LINESTYLE line_style;
	LOGFONT text_style = {};
};

inline void sketch_image(IMAGE* src, IMAGE* dst)
{
	int w = src->getwidth();
	int h = src->getheight();
	Resize(dst, w, h);
	DWORD* src_buffer = GetImageBuffer(src);
	DWORD* dst_buffer = GetImageBuffer(dst);
	for (int y = 0; y < h; y++)
	{
		for (int x = 0; x < w; x++)
		{
			int idx = y * w + x;
			dst_buffer[idx] = BGR(RGB(255, 255, 255)) | (src_buffer[idx] & 0xFF000000);
		}
	}
}
#endif // !_UTIL_H_
