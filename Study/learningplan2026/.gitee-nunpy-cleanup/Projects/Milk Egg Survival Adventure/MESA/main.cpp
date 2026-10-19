#include <graphics.h>
#include <string>
#include <vector>
#include <cmath>
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <cwchar> // 用于比较视频播放状态
#include <atomic> // 跨线程传递关闭视频请求

int idx_current_anim = 0;
const int PLAYER_ANIM_NUM = 6; 
const int WINDOW_WIDTH = 1280;
const int WINDOW_HEIGHT = 720;

const int BUTTON_WIDTH = 192;
const int BUTTON_HEIGHT = 75;
IMAGE img_ice;
IMAGE img_bullet;
IMAGE img_hp_bar_bg;
IMAGE img_hp_bar_fill;
IMAGE img_ui_freeze; // 右下角冰冻技能图标
IMAGE img_heal_pickup[3]; // 普通、精英、BOSS 血包，均 24×24
IMAGE img_fire[6];        // 脚底火焰六帧，均 32×32

// 原先速度仍按 144 FPS 的每帧像素换算
constexpr double MOVE_REFERENCE_FPS = 144.0;

enum class EnemyType
{
	Normal = 0,
	Elite = 1,
	Boss = 2
};

#pragma comment(lib, "Winmm.lib")
#pragma comment(lib, "MSIMG32.LIB")

bool running = true;

// 四个界面只允许其中一个处于活动状态
enum class Scene
{
	Menu,       // 主菜单
	Playing,    // 战斗
	GameOver,   // 死亡界面，等待按 1 或点击确认
	Video       // 在游戏窗口内播放视频
};

Scene scene = Scene::Menu;
// EasyX 窗口消息与 main 可能在不同线程，用原子变量传递关闭请求
std::atomic<bool> video_is_playing{ false };
std::atomic<bool> video_close_requested{ false };
std::atomic<WNDPROC> original_game_proc{ nullptr };

LRESULT CALLBACK GameVideoWindowProc(HWND hwnd, UINT message,
	WPARAM wparam, LPARAM lparam)
{
	if (message == WM_CLOSE && video_is_playing.load())
	{
		// 播放中点击右上角 ×：只请求返回菜单，不销毁游戏窗口
		video_close_requested.store(true);
		return 0;
	}

	// 其余消息继续交给 EasyX；这里不调用任何绘图或游戏状态函数
	WNDPROC original = original_game_proc.load();
	if (original)
		return CallWindowProcW(original, hwnd, message, wparam, lparam);
	return DefWindowProcW(hwnd, message, wparam, lparam);
}

void RestoreGameWindowProc(HWND hwnd)
{
	WNDPROC original = original_game_proc.load();
	if (original && IsWindow(hwnd))
		SetWindowLongPtrW(hwnd, GWLP_WNDPROC,
			reinterpret_cast<LONG_PTR>(original));
}

bool InstallVideoWindowSupport(HWND hwnd)
{
	// 父窗口绘图时排除视频子窗口区域，避免相互覆盖造成闪烁
	LONG_PTR style = GetWindowLongPtrW(hwnd, GWL_STYLE);
	SetLastError(0);
	if (SetWindowLongPtrW(hwnd, GWL_STYLE, style | WS_CLIPCHILDREN) == 0 &&
		GetLastError() != 0)
		return false;

	WNDPROC original = reinterpret_cast<WNDPROC>(
		GetWindowLongPtrW(hwnd, GWLP_WNDPROC));
	if (!original)
		return false;
	original_game_proc.store(original);
	SetLastError(0);
	if (SetWindowLongPtrW(hwnd, GWLP_WNDPROC,
		reinterpret_cast<LONG_PTR>(GameVideoWindowProc)) == 0 &&
		GetLastError() != 0)
		return false;

	// 应用新的窗口样式，不移动、不改变尺寸，也不抢焦点
	if (!SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
		SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
		SWP_NOACTIVATE | SWP_FRAMECHANGED))
	{
		RestoreGameWindowProc(hwnd);
		return false;
	}
	return true;
}

// 默认完全显示；opacity 降低时，图标会透出背景而显得变暗
inline void putimage_alpha(int x, int y, IMAGE* img, BYTE opacity = 255)
{
	int w = img->getwidth();
	int h = img->getheight();
	AlphaBlend(GetImageHDC(NULL), x, y, w, h,
		GetImageHDC(img), 0, 0, w, h,
		{ AC_SRC_OVER, 0, opacity, AC_SRC_ALPHA });
}

// 只画图片左侧的一部分，用于血条；源宽和目标宽相同，不拉伸
inline void putimage_alpha_width(int x, int y, IMAGE* img, int width)
{
	width = (std::min)(width, img->getwidth());
	if (width <= 0)
		return;

	int height = img->getheight();
	AlphaBlend(GetImageHDC(NULL), x, y, width, height,
		GetImageHDC(img), 0, 0, width, height,
		{ AC_SRC_OVER, 0, 255, AC_SRC_ALPHA });
}

void RenderFrozenPlayer(int x, int y, IMAGE* frame, int highlight_y);

//动画所使用的“图集”类定义
class Atlas
{
public:
	Atlas(LPCTCH path, int num)
	{
		TCHAR path_file[256];
		for (int i = 0; i < num; i++)
		{
			_stprintf_s(path_file, path, i);

			IMAGE* frame = new IMAGE();
			loadimage(frame, path_file);
			frame_list.push_back(frame);
		}

	}

	~Atlas()
	{
		for (size_t i = 0; i < frame_list.size(); i++)
			delete frame_list[i];
	}

public:
	std::vector<IMAGE*> frame_list;

public:
	Atlas() = default;
};

Atlas* atlas_player_left;
Atlas* atlas_player_right;
// 下标 0、1、2 分别对应普通怪、精英、BOSS
Atlas* atlas_enemy_left[3] = {};
Atlas* atlas_enemy_right[3] = {};

//动画类定义
class Animation
{
public:
	Animation(Atlas* atlas, int interval)
	{
		anim_atlas = atlas;
		interval_ms = interval; //帧间隔
	}

	~Animation() = default;

	//动画计时器
	void Play(int x, int y, int delta,
		bool frozen = false, int highlight_y = 0, bool visible = true)
	{
		if (!frozen)
		{
			timer += delta;
			while (timer >= interval_ms)
			{
				idx_frame = (idx_frame + 1) % anim_atlas->frame_list.size();
				timer -= interval_ms;
			}
		}

		// 不显示的闪烁帧仍然完成上面的动画计时
		if (!visible)
			return;

		IMAGE* frame = anim_atlas->frame_list[idx_frame];
		if (frozen)
			RenderFrozenPlayer(x, y, frame, highlight_y);
		else
			putimage_alpha(x, y, frame);
	}

	// 新一局从第一帧重新播放
	void Reset()
	{
		timer = 0;
		idx_frame = 0;
	}

private:
	int timer = 0; //动画计时器
	int idx_frame = 0;	//动画帧索引
	int interval_ms = 0;	//帧间隔

private:
	Atlas* anim_atlas;
};

//玩家类定义
class Player
{
public:
	const int FRAME_WIDTH = 80;
	const int FRAME_HEIGHT = 80;

	Player()
	{
		loadimage(&img_shadow, _T("img/shadow_player.png"));
		anim_left = new Animation(atlas_player_left, 45);
		anim_right = new Animation(atlas_player_right, 45);
	}

	~Player()
	{
		delete anim_left;
		delete anim_right;
	}

	bool IsFrozen() const { return frozen_remaining_ms > 0; }
	bool IsInvulnerable() const { return invulnerable_ms > 0; }
	bool IsAlive() const { return hp > 0; }
	int GetHP() const { return hp; }
	int GetMaxHP() const { return max_hp; }

	const POINT& GetPosition() const { return position; }

	POINT GetCenter() const
	{
		return { position.x + FRAME_WIDTH / 2,
				 position.y + FRAME_HEIGHT / 2 };
	}

	// 开始技能：冻结 2 秒，无敌合计 5 秒，冷却 10 秒
	bool TryCastFreeze()
	{
		if (!IsAlive() || IsFrozen() || freeze_cooldown_ms > 0)
			return false;

		frozen_remaining_ms = 2000;
		frozen_elapsed_ms = 0;
		invulnerable_ms = (std::max)(invulnerable_ms, 5000);
		freeze_cooldown_ms = 10000;
		blink_elapsed_ms = 0;
		freeze_attack_pending = true; // 让主循环只结算一次伤害
		return true;
	}

	bool ConsumeFreezeAttack()
	{
		bool pending = freeze_attack_pending;
		freeze_attack_pending = false;
		return pending;
	}

	// 所有状态计时都使用实际经过的毫秒数
	void Update(int delta)
	{
		if (IsFrozen())
			frozen_elapsed_ms += (std::min)(delta, frozen_remaining_ms);

		frozen_remaining_ms = (std::max)(0, frozen_remaining_ms - delta);
		invulnerable_ms = (std::max)(0, invulnerable_ms - delta);
		freeze_cooldown_ms = (std::max)(0, freeze_cooldown_ms - delta);
		blink_elapsed_ms += delta;
	}

	bool TakeDamage(int damage)
	{
		if (!IsAlive() || IsInvulnerable() || damage <= 0)
			return false;

		hp = (std::max)(0, hp - damage);
		invulnerable_ms = 1000; // 受伤后保护 1 秒
		blink_elapsed_ms = 0;
		return true;
	}

	void Heal(int amount)
	{
		if (IsAlive() && amount > 0)
			hp = (std::min)(max_hp, hp + amount);
	}

	void IncreaseMaxHP(int amount)
	{
		if (amount <= 0)
			return;

		max_hp += amount;
		hp = (std::min)(max_hp, hp + amount); // 同时增加当前血量
	}

	void ProcessEvent(const ExMessage& msg)
	{
		if (msg.message == WM_KEYDOWN)
		{
			switch (msg.vkcode)
			{
			case VK_UP:    is_move_up = true; break;
			case VK_DOWN:  is_move_down = true; break;
			case VK_LEFT:  is_move_left = true; break;
			case VK_RIGHT: is_move_right = true; break;
			case 'F':
				if (!freeze_key_down)
					TryCastFreeze();
				freeze_key_down = true; // 防止按住 F 自动重复释放
				break;
			}
		}
		else if (msg.message == WM_KEYUP)
		{
			switch (msg.vkcode)
			{
			case VK_UP:    is_move_up = false; break;
			case VK_DOWN:  is_move_down = false; break;
			case VK_LEFT:  is_move_left = false; break;
			case VK_RIGHT: is_move_right = false; break;
			case 'F':      freeze_key_down = false; break;
			}
		}
	}

	void Move(int delta)
	{
		if (IsFrozen() || !IsAlive())
			return;

		int dir_x = is_move_right - is_move_left;
		int dir_y = is_move_down - is_move_up;
		double length = std::sqrt(double(dir_x * dir_x + dir_y * dir_y));

		if (length > 0.0)
		{
			double step = SPEED * MOVE_REFERENCE_FPS * delta / 1000.0;
			pos_x += dir_x / length * step;
			pos_y += dir_y / length * step;
		}

		// 小数位置保存到下一帧，绘制时才取整
		pos_x = (std::max)(0.0, (std::min)(
			pos_x, double(WINDOW_WIDTH - FRAME_WIDTH)));
		pos_y = (std::max)(0.0, (std::min)(
			pos_y, double(WINDOW_HEIGHT - FRAME_HEIGHT)));

		position.x = static_cast<LONG>(std::lround(pos_x));
		position.y = static_cast<LONG>(std::lround(pos_y));
	}

	void Draw(int delta)
	{
		putimage_alpha(position.x + (FRAME_WIDTH - 32) / 2,
			position.y + FRAME_HEIGHT - 8, &img_shadow);

		if (!IsFrozen())
		{
			int dir_x = is_move_right - is_move_left;
			if (dir_x < 0) facing_left = true;
			else if (dir_x > 0) facing_left = false;
		}

		Animation* current = facing_left ? anim_left : anim_right;
		int highlight_y = (frozen_elapsed_ms / 8) % (FRAME_HEIGHT + 5) - 5;

		// 无敌期间每 100 毫秒切换显示/隐藏
		bool visible = !IsInvulnerable() || (blink_elapsed_ms / 100) % 2 == 0;
		current->Play(position.x, position.y, delta,
			IsFrozen(), highlight_y, visible);
	}

	// 主循环最后绘制血条，避免被敌人或子弹遮住
	void DrawHealthBar() const
	{
		int bar_x = position.x + (FRAME_WIDTH - 64) / 2;
		int bar_y = (std::max)(0, int(position.y) - 16);
		putimage_alpha(bar_x, bar_y, &img_hp_bar_bg);

		int red_width = 60 * hp / max_hp;
		putimage_alpha_width(bar_x + 2, bar_y + 2,
			&img_hp_bar_fill, red_width);
	}

	void DrawFreezeHUD() const
	{
		const int icon_size = 64;
		const int margin = 20;
		const int label_height = 22;
		int x = WINDOW_WIDTH - margin - icon_size;
		int y = WINDOW_HEIGHT - margin - label_height - icon_size;

		bool cooling = freeze_cooldown_ms > 0;
		// 冷却图标变暗；正在冻结时保持亮度，便于看出技能正在释放
		BYTE opacity = (cooling && !IsFrozen()) ? 110 : 255;
		putimage_alpha(x, y, &img_ui_freeze, opacity);

		// 保存文本设置，防止影响分数等其他文字
		LOGFONT old_font;
		gettextstyle(&old_font);
		COLORREF old_color = gettextcolor();
		int old_bk_mode = getbkmode();
		setbkmode(TRANSPARENT);

		settextstyle(16, 0, _T("Microsoft YaHei"));
		settextcolor(RGB(255, 255, 255));
		outtextxy(x + 4, y + 3, _T("F")); // 快捷键提示

		if (cooling && !IsFrozen())
		{
			TCHAR text[16];
			int seconds = (freeze_cooldown_ms + 999) / 1000; // 向上取整
			_stprintf_s(text, _T("%d"), seconds);
			settextstyle(24, 0, _T("Microsoft YaHei"));
			outtextxy(x + (icon_size - textwidth(text)) / 2,
				y + (icon_size - textheight(text)) / 2, text);
		}

		LPCTSTR label = IsFrozen() ? _T("冻结中") :
			(cooling ? _T("冷却中") : _T("冰冻"));
		settextstyle(18, 0, _T("Microsoft YaHei"));
		settextcolor(RGB(210, 245, 255));
		outtextxy(x + (icon_size - textwidth(label)) / 2,
			y + icon_size + 2, label);

		// 恢复调用之前的设置
		settextstyle(&old_font);
		settextcolor(old_color);
		setbkmode(old_bk_mode);
	}

	// 新一局只重置状态，继续使用已经加载的动画资源
	void Reset()
	{
		position = { 500, 500 };
		pos_x = 500.0;
		pos_y = 500.0;
		is_move_up = is_move_down = false;
		is_move_left = is_move_right = false;
		facing_left = false;
		freeze_key_down = false;
		hp = max_hp = 50;
		frozen_remaining_ms = frozen_elapsed_ms = 0;
		invulnerable_ms = blink_elapsed_ms = 0;
		freeze_cooldown_ms = 0;
		freeze_attack_pending = false;
		anim_left->Reset();
		anim_right->Reset();
	}

private:
	const double SPEED = 3.0;
	IMAGE img_shadow;
	Animation* anim_left;
	Animation* anim_right;

	POINT position = { 500, 500 };
	double pos_x = 500.0;
	double pos_y = 500.0;
	bool is_move_up = false, is_move_down = false;
	bool is_move_left = false, is_move_right = false;
	bool facing_left = false;
	bool freeze_key_down = false;

	int hp = 50, max_hp = 50; // 出生时当前血量和上限均为 50
	int frozen_remaining_ms = 0, frozen_elapsed_ms = 0;
	int invulnerable_ms = 0, blink_elapsed_ms = 0;
	int freeze_cooldown_ms = 0;
	bool freeze_attack_pending = false;
};

//子弹类定义
class Bullet
{
public:
	// position 仍表示子弹中心
	POINT position = { 0, 0 };

	Bullet() = default;
	~Bullet() = default;

	void Draw() const
	{
		int draw_x = position.x - img_bullet.getwidth() / 2;
		int draw_y = position.y - img_bullet.getheight() / 2;

		putimage_alpha(draw_x, draw_y, &img_bullet);
	}
};

//灼烧
struct BurnState
{
	int remaining_ms = 0;
	int damage_elapsed_ms = 0;
	int animation_ms = 0;

	bool IsActive() const { return remaining_ms > 0; }

	void IgniteByBullet()
	{
		if (!IsActive())
		{
			damage_elapsed_ms = 0;
			animation_ms = 0;
		}
		// 只刷新持续时间，保留伤害节拍，避免频繁命中阻止灼烧扣血
		remaining_ms = 3000;
	}

	int Update(int delta)
	{
		if (!IsActive() || delta <= 0)
			return 0;

		// 即使某帧卡顿，也只结算尚未结束的燃烧时间
		int active_time = (std::min)(delta, remaining_ms);
		remaining_ms -= active_time;
		animation_ms += active_time;
		damage_elapsed_ms += active_time;
		int damage = damage_elapsed_ms / 500; // 每满 500 毫秒扣 1 血
		damage_elapsed_ms %= 500;             // 保留不足 500 毫秒的余量
		return damage;
	}

	void InheritFrom(const BurnState& source)
	{
		// 已燃烧的敌人不能靠互相传火延长时间
		if (IsActive() || !source.IsActive())
			return;

		remaining_ms = source.remaining_ms;
		damage_elapsed_ms = source.damage_elapsed_ms;
		animation_ms = source.animation_ms;
	}
};

//敌人类定义
class Enemy
{
public:
	explicit Enemy(EnemyType enemy_type = EnemyType::Normal)
		: type(enemy_type)
	{
		int index = static_cast<int>(type);
		const int hp_table[3] = { 1, 20, 200 }; // 普通、精英、BOSS
		const int score_table[3] = { 1, 5, 10 };
		const int damage_table[3] = { 1, 3, 10 };
		const double speed_table[3] = { 2.5, 1.0, 1.0 };

		hp = hp_table[index];
		max_hp = hp; // BOSS 上限记录为 20，不随扣血变化
		score_value = score_table[index];
		contact_damage = damage_table[index];
		speed = speed_table[index];

		loadimage(&img_shadow, _T("img/shadow_enemy.png"));
		anim_left = new Animation(atlas_enemy_left[index], 45);
		anim_right = new Animation(atlas_enemy_right[index], 45);

		// 在地图四边之外出生
		switch (std::rand() % 4)
		{
		case 0: pos_x = std::rand() % WINDOW_WIDTH; pos_y = -FRAME_HEIGHT; break;
		case 1: pos_x = std::rand() % WINDOW_WIDTH; pos_y = WINDOW_HEIGHT; break;
		case 2: pos_x = -FRAME_WIDTH; pos_y = std::rand() % WINDOW_HEIGHT; break;
		case 3: pos_x = WINDOW_WIDTH; pos_y = std::rand() % WINDOW_HEIGHT; break;
		}
	}

	~Enemy()
	{
		delete anim_left;
		delete anim_right;
	}

	EnemyType GetType() const { return type; }
	int GetScore() const { return score_value; }
	int GetContactDamage() const { return contact_damage; }
	bool CheckAlive() const { return hp > 0; }

	POINT GetCenter() const
	{
		return {
			static_cast<LONG>(std::lround(pos_x + FRAME_WIDTH / 2.0)),
			static_cast<LONG>(std::lround(pos_y + FRAME_HEIGHT / 2.0))
		};
	}

	const BurnState& GetBurnState() const { return burn; }

	void ReceiveBurn(const BurnState& source)
	{
		if (CheckAlive())
			burn.InheritFrom(source);
	}

	void UpdateBurn(int delta)
	{
		if (CheckAlive())
			Hurt(burn.Update(delta));
	}

	bool CheckEnemyCollision(const Enemy& other) const
	{
		// 敌人矩形相互接触就可传火
		return pos_x <= other.pos_x + FRAME_WIDTH &&
			pos_x + FRAME_WIDTH >= other.pos_x &&
			pos_y <= other.pos_y + FRAME_HEIGHT &&
			pos_y + FRAME_HEIGHT >= other.pos_y;
	}

	void Hurt(int damage)
	{
		if (CheckAlive() && damage > 0)
			hp = (std::max)(0, hp - damage);
	}

	bool CheckBulletCollision(const Bullet& bullet) const
	{
		// 32×32 子弹近似半径 14 的圆，检测圆和敌人矩形
		double nearest_x = (std::max)(pos_x,
			(std::min)(double(bullet.position.x), pos_x + FRAME_WIDTH));
		double nearest_y = (std::max)(pos_y,
			(std::min)(double(bullet.position.y), pos_y + FRAME_HEIGHT));
		double dx = bullet.position.x - nearest_x;
		double dy = bullet.position.y - nearest_y;
		return dx * dx + dy * dy <= 14.0 * 14.0;
	}

	void CheckBulletHits(const std::vector<Bullet>& bullets)
	{
		if (!CheckAlive())
			return;

		if (bullet_touching.size() != bullets.size())
			bullet_touching.assign(bullets.size(), false);

		for (size_t i = 0; i < bullets.size(); i++)
		{
			bool touching = CheckBulletCollision(bullets[i]);

			// 直接伤害仍只在每颗子弹刚接触时扣 1 血
			if (touching && !bullet_touching[i])
				Hurt(1);

			// 持续接触子弹也能刷新火焰；已死亡敌人不再点燃
			if (touching && CheckAlive())
				burn.IgniteByBullet();

			bullet_touching[i] = touching;
			if (!CheckAlive())
				break;
		}
	}

	bool CheckPlayerCollision(const Player& player) const
	{
		const POINT& p = player.GetPosition();
		return pos_x < p.x + player.FRAME_WIDTH &&
			pos_x + FRAME_WIDTH > p.x &&
			pos_y < p.y + player.FRAME_HEIGHT &&
			pos_y + FRAME_HEIGHT > p.y;
	}

	bool IsInFreezeRange(const POINT& center, double radius) const
	{
		// 按敌人中心是否进入技能圆形范围判断
		double dx = pos_x + FRAME_WIDTH / 2.0 - center.x;
		double dy = pos_y + FRAME_HEIGHT / 2.0 - center.y;
		return dx * dx + dy * dy <= radius * radius;
	}

	void Move(const Player& player, int delta)
	{
		if (!CheckAlive())
			return;

		if (type != EnemyType::Boss)
		{
			ChasePlayer(player, delta);
			return;
		}

		switch (boss_phase)
		{
		case BossPhase::Chase:
			ChasePlayer(player, delta);
			phase_timer_ms += delta;
			if (phase_timer_ms >= 3000) // 追踪 3 秒后开始冲刺
				BeginDash(player);
			break;

		case BossPhase::Dash:
		{
			// BOSS 冲刺速度下调为 5；距离仍为 300 像素
			double step = 5.0 * MOVE_REFERENCE_FPS * delta / 1000.0;
			step = (std::min)(step, dash_remaining);
			pos_x += dash_dir_x * step;
			pos_y += dash_dir_y * step;
			dash_remaining -= step;
			if (dash_remaining <= 0.0001)
			{
				boss_phase = BossPhase::Rest;
				phase_timer_ms = 0;
			}
			break;
		}

		case BossPhase::Rest:
			// 休息期间位置不变
			phase_timer_ms += delta;
			if (phase_timer_ms >= 2000) // 休息 2 秒后恢复追踪
			{
				boss_phase = BossPhase::Chase;
				phase_timer_ms = 0;
			}
			break;
		}
	}

	void Draw(int delta)
	{
		int x = static_cast<int>(std::lround(pos_x));
		int y = static_cast<int>(std::lround(pos_y));
		putimage_alpha(x + (FRAME_WIDTH - 48) / 2,
			y + FRAME_HEIGHT - 35, &img_shadow);

		int anim_delta = (type == EnemyType::Boss &&
			boss_phase == BossPhase::Rest) ? 0 : delta;
		Animation* current = facing_left ? anim_left : anim_right;
		current->Play(x, y, anim_delta);

		// 火焰覆盖脚部少量区域，80 毫秒一帧，共六帧循环
		if (burn.IsActive() && CheckAlive())
		{
			int frame = (burn.animation_ms / 80) % 6;
			putimage_alpha(x + (FRAME_WIDTH - 32) / 2,
				y + FRAME_HEIGHT - 28, &img_fire[frame]);
		}
	}

	void DrawHealthBar() const
	{
		// 普通怪不画血条，精英和 BOSS 都显示自己的血量
		if (type == EnemyType::Normal || !CheckAlive())
			return;

		int x = static_cast<int>(std::lround(pos_x));
		int y = static_cast<int>(std::lround(pos_y));

		// BOSS 尚在画面外出生时，不把血条硬贴到窗口上边缘
		if (x + FRAME_WIDTH <= 0 || x >= WINDOW_WIDTH ||
			y + FRAME_HEIGHT <= 0 || y >= WINDOW_HEIGHT)
			return;

		int bar_x = x + (FRAME_WIDTH - 64) / 2;
		int bar_y = (std::max)(0, y - 16);
		putimage_alpha(bar_x, bar_y, &img_hp_bar_bg);

		// 例如 10/20 血时画 30 像素红条，其余部分露出黑底
		int red_width = 60 * hp / max_hp;
		putimage_alpha_width(bar_x + 2, bar_y + 2,
			&img_hp_bar_fill, red_width);
	}

private:
	enum class BossPhase { Chase, Dash, Rest };

	void ChasePlayer(const Player& player, int delta)
	{
		POINT center = player.GetCenter();
		double dx = center.x - (pos_x + FRAME_WIDTH / 2.0);
		double dy = center.y - (pos_y + FRAME_HEIGHT / 2.0);
		double length = std::sqrt(dx * dx + dy * dy);

		if (dx < 0.0) facing_left = true;
		else if (dx > 0.0) facing_left = false;

		if (length <= 0.0001)
			return;

		double step = speed * MOVE_REFERENCE_FPS * delta / 1000.0;
		step = (std::min)(step, length);
		pos_x += dx / length * step;
		pos_y += dy / length * step;
	}

	void BeginDash(const Player& player)
	{
		POINT center = player.GetCenter();
		double dx = center.x - (pos_x + FRAME_WIDTH / 2.0);
		double dy = center.y - (pos_y + FRAME_HEIGHT / 2.0);
		double length = std::sqrt(dx * dx + dy * dy);

		phase_timer_ms = 0;
		if (length <= 0.0001)
		{
			// 中心重合时无法确定方向，直接休息
			boss_phase = BossPhase::Rest;
			return;
		}

		// 只在冲刺开始时计算方向，冲刺中不再追踪玩家
		dash_dir_x = dx / length;
		dash_dir_y = dy / length;
		dash_remaining = 300.0;
		boss_phase = BossPhase::Dash;
		if (dx < 0.0) facing_left = true;
		else if (dx > 0.0) facing_left = false;
	}

private:
	const int FRAME_WIDTH = 80, FRAME_HEIGHT = 80;
	EnemyType type;
	int hp = 1, max_hp = 1; // 每个敌人实例都有自己的当前血量和上限
	int score_value = 1, contact_damage = 1;
	double speed = 2.0;
	double pos_x = 0.0, pos_y = 0.0;
	bool facing_left = false;

	IMAGE img_shadow;
	Animation* anim_left;
	Animation* anim_right;
	std::vector<bool> bullet_touching;
	BurnState burn; // 每个敌人独立保存火焰剩余时间和扣血节拍

	BossPhase boss_phase = BossPhase::Chase;
	int phase_timer_ms = 0;
	double dash_dir_x = 0.0, dash_dir_y = 0.0;
	double dash_remaining = 0.0;
};

//按钮类定义
class Button
{
public:
	Button(RECT rect, LPCTSTR path_img_idle, LPCTSTR path_img_hovered, LPCTSTR path_img_pushed)
	{
		region = rect;

		loadimage(&img_idle, path_img_idle);
		loadimage(&img_hovered, path_img_hovered);
		loadimage(&img_pushed, path_img_pushed);
	}

	~Button() = default;

	void ProcessEvent(const ExMessage& msg)
	{
		switch (msg.message)
		{
		case WM_MOUSEMOVE:
			if (status == Status::Idle && CheckCursorHit(msg.x, msg.y))
				status = Status::Hovered;
			else if (status == Status::Hovered && !CheckCursorHit(msg.x, msg.y))
				status = Status::Idle;
			break;
		case WM_LBUTTONDOWN:
			if (CheckCursorHit(msg.x, msg.y))
				status = Status::Pushed;
			break;
		case WM_LBUTTONUP:
			if (status == Status::Pushed)
				OnClick();
			break;
		default:
			break;
		}
	}

	void Draw()
	{
		switch (status)
		{
		case Status::Idle:
			putimage(region.left, region.top, &img_idle);
			break;
		case Status::Hovered:
			putimage(region.left, region.top, &img_hovered);
			break;
		case Status::Pushed:
			putimage(region.left, region.top, &img_pushed);
			break;
		}
	}

	// 返回菜单时，清除上次点击留下的按下状态
	void ResetState()
	{
		status = Status::Idle;
	}

protected:
	virtual void OnClick() = 0;

private:
	enum class Status
	{
		Idle = 0,
		Hovered,
		Pushed
	};

private:
	RECT region;
	IMAGE img_idle;
	IMAGE img_hovered;
	IMAGE img_pushed;
	Status status = Status::Idle;

private:
	//检测鼠标点击
	bool CheckCursorHit(int x, int y)
	{
		return x >= region.left && x <= region.right && y >= region.top && y <= region.bottom;
	}
};

//血包类型，掉落，拾取与绘制
struct HealPickup
{
	EnemyType source_type; // 决定外观和恢复量
	POINT position;       // 血包中心
};
void TryDropHealPickup(const Enemy& enemy, std::vector<HealPickup>& pickups)
{
	const int chance_table[3] = { 1, 30, 100 };
	int index = static_cast<int>(enemy.GetType());
	if (std::rand() % 100 >= chance_table[index])
		return;

	POINT center = enemy.GetCenter();
	// 边缘击杀时，把血包保持在可拾取的画面内
	center.x = (std::max)(LONG(12),
		(std::min)(center.x, LONG(WINDOW_WIDTH - 12)));
	center.y = (std::max)(LONG(12),
		(std::min)(center.y, LONG(WINDOW_HEIGHT - 12)));
	pickups.push_back({ enemy.GetType(), center });
}
void UpdateHealPickups(std::vector<HealPickup>& pickups, Player& player)
{
	// 满血时留在地上，不浪费血包；死亡后不能靠血包复活
	if (!player.IsAlive() || player.GetHP() >= player.GetMaxHP())
		return;

	const POINT& p = player.GetPosition();
	for (size_t i = 0; i < pickups.size(); )
	{
		const HealPickup& pickup = pickups[i];
		double nearest_x = (std::max)(double(p.x),
			(std::min)(double(pickup.position.x), double(p.x + player.FRAME_WIDTH)));
		double nearest_y = (std::max)(double(p.y),
			(std::min)(double(pickup.position.y), double(p.y + player.FRAME_HEIGHT)));
		double dx = pickup.position.x - nearest_x;
		double dy = pickup.position.y - nearest_y;
		if (dx * dx + dy * dy > 12.0 * 12.0)
		{
			i++;
			continue;
		}

		int heal = 2;
		if (pickup.source_type == EnemyType::Elite)
			heal = (player.GetMaxHP() + 9) / 10; // 上限的 10%，向上取整
		else if (pickup.source_type == EnemyType::Boss)
			heal = player.GetMaxHP();           // 加上整个上限，最终回满

		player.Heal(heal); // 原 Heal 已限制不会超过上限
		pickups[i] = pickups.back();
		pickups.pop_back();
		if (player.GetHP() >= player.GetMaxHP())
			return;
	}
}
void DrawHealPickups(const std::vector<HealPickup>& pickups)
{
	for (const HealPickup& pickup : pickups)
	{
		int index = static_cast<int>(pickup.source_type);
		putimage_alpha(pickup.position.x - 12,
			pickup.position.y - 12, &img_heal_pickup[index]);
	}
}

//开始游戏按钮
class StartGameButton : public Button
{
public:
	StartGameButton(RECT rect, LPCTSTR path_img_idle,LPCTSTR path_img_hovered,LPCTSTR path_img_pushed)
		: Button(rect, path_img_idle, path_img_hovered,path_img_pushed){}
	~StartGameButton() = default;

protected:
	void OnClick()
	{
		// 00000409 表示英文（美国）键盘，进入游戏时加载并激活
		HKL english_layout = LoadKeyboardLayoutW(L"00000409", KLF_ACTIVATE);
		if (english_layout != nullptr)
		{
			// 通知 EasyX 游戏窗口使用这个输入布局
			PostMessage(GetHWnd(), WM_INPUTLANGCHANGEREQUEST, 0,
				reinterpret_cast<LPARAM>(english_layout));
		}

		scene = Scene::Playing; // 主循环负责重置新局和播放背景音乐
	}
};

//退出游戏按钮
class QuitGameButton : public Button
{
public:
	QuitGameButton(RECT rect, LPCTSTR path_img_idle, LPCTSTR path_img_hovered, LPCTSTR path_img_pushed)
		: Button(rect, path_img_idle, path_img_hovered, path_img_pushed) {}
	~QuitGameButton() = default;

protected:
	void OnClick()
	{
		running = false;
	}
};

//统计活着的敌人
int CountAliveEnemies(const std::vector<Enemy*>& enemies, EnemyType type)
{
	int count = 0;
	for (const Enemy* enemy : enemies)
		if (enemy->CheckAlive() && enemy->GetType() == type)
			count++;
	return count;
}

//火焰传播
void SpreadBurning(std::vector<Enemy*>& enemies)
{
	// 快照确保结果不依赖敌人在 vector 中的排列顺序
	std::vector<BurnState> source_states;
	source_states.reserve(enemies.size());
	for (const Enemy* enemy : enemies)
		source_states.push_back(enemy->GetBurnState());

	std::vector<BurnState> incoming(enemies.size());
	for (size_t target = 0; target < enemies.size(); target++)
	{
		if (!enemies[target]->CheckAlive() || source_states[target].IsActive())
			continue;

		for (size_t source = 0; source < enemies.size(); source++)
		{
			if (source == target || !enemies[source]->CheckAlive() ||
				!source_states[source].IsActive())
				continue;

			if (enemies[target]->CheckEnemyCollision(*enemies[source]) &&
				source_states[source].remaining_ms > incoming[target].remaining_ms)
				incoming[target] = source_states[source];
		}
	}

	for (size_t i = 0; i < enemies.size(); i++)
		enemies[i]->ReceiveBurn(incoming[i]);
}

//生成新的敌人
void TryGenerateEnemy(std::vector<Enemy*>& enemy_list, int delta, bool reset = false)
{
	static int timer_ms = 0;
	if (reset)
	{
		timer_ms = 0; // 清除上一局遗留的刷怪计时
		return;
	}
	// 以下从 timer_ms += delta 开始的原代码继续保留
	timer_ms += delta;
	while (timer_ms >= 700)
	{
		timer_ms -= 700;
		int normal_count = CountAliveEnemies(enemy_list, EnemyType::Normal);
		int elite_count = CountAliveEnemies(enemy_list, EnemyType::Elite);
		if (normal_count >= 30 && elite_count >= 7)
			continue;

		// 保留原来约 25% 的精英选择概率；一种满额则改刷另一种
		bool choose_elite = std::rand() % 4 == 0;
		if (elite_count >= 7) choose_elite = false;
		if (normal_count >= 30) choose_elite = true;
		enemy_list.push_back(new Enemy(
			choose_elite ? EnemyType::Elite : EnemyType::Normal));
	}
}
void TryGenerateBoss(std::vector<Enemy*>& enemy_list,
	int score, int next_boss_score)
{
	if (score >= next_boss_score &&
		CountAliveEnemies(enemy_list, EnemyType::Boss) == 0)
		enemy_list.push_back(new Enemy(EnemyType::Boss));
}

//击杀掉落函数
void HandleEnemyDeaths(std::vector<Enemy*>& enemy_list,
	std::vector<HealPickup>& pickups, int& score, int& next_boss_score)
{
	bool boss_killed = false;
	for (size_t i = 0; i < enemy_list.size(); )
	{
		Enemy* enemy = enemy_list[i];
		if (enemy->CheckAlive())
		{
			i++;
			continue;
		}

		score += enemy->GetScore();
		TryDropHealPickup(*enemy, pickups); // 每只死亡敌人只抽一次掉落
		if (enemy->GetType() == EnemyType::Boss)
			boss_killed = true;

		mciSendString(_T("play hit from 0"), NULL, 0, NULL);
		std::swap(enemy_list[i], enemy_list.back());
		enemy_list.pop_back();
		delete enemy;
	}

	// 包含本帧全部击杀积分；BOSS 存活期间累计的分数不排队补刷
	if (boss_killed)
		next_boss_score = score + 50;
}

//更新子弹位置
void UpdateBullets(std::vector<Bullet>& bullet_list, const Player& player,
	int delta, bool reset_angle = false)
{
	if (bullet_list.empty())
		return;

	const double PI = 3.141592653589793;
	const double RADIUS = 110.0;
	const double ANGULAR_SPEED = 6.0; // 弧度/秒，比原来快 50%

	static double angle = 0.0;
	if (reset_angle)
		angle = 0.0; // 新局恢复初始旋转角度

	angle = std::fmod(
		angle + ANGULAR_SPEED * delta / 1000.0,
		2.0 * PI);

	const POINT& pos = player.GetPosition();

	int center_x = pos.x + player.FRAME_WIDTH / 2;
	int center_y = pos.y + player.FRAME_HEIGHT / 2;

	double interval = 2.0 * PI / bullet_list.size();

	for (size_t i = 0; i < bullet_list.size(); i++)
	{
		double current_angle = angle + interval * i;

		bullet_list[i].position.x =
			center_x + static_cast<int>(
				std::lround(RADIUS * std::cos(current_angle)));

		bullet_list[i].position.y =
			center_y - static_cast<int>(
				std::lround(RADIUS * std::sin(current_angle)));
	}
}

//绘制玩家得分
void DrawPlayerScore(int score)
{
	static TCHAR text[64];
	_stprintf_s(text, _T("当前玩家得分:%d"), score);
	
	setbkmode(TRANSPARENT);
	settextcolor(RGB(255, 85, 185));
	outtextxy(10, 10, text);
}

//玩家冻结代码
void RenderFrozenPlayer(int x, int y, IMAGE* frame, int highlight_y)
{
	// 处理副本，避免把原始动画帧永久染成冰蓝色
	IMAGE img_current_frame(*frame);

	int width = img_current_frame.getwidth();
	int height = img_current_frame.getheight();

	// 纹理尺寸必须与动画帧一致，避免访问越界
	if (img_ice.getwidth() != width ||
		img_ice.getheight() != height)
	{
		putimage_alpha(x, y, frame);
		return;
	}

	DWORD* ice = GetImageBuffer(&img_ice);
	DWORD* pixels = GetImageBuffer(&img_current_frame);

	const float RATIO = 0.25f;
	const float THRESHOLD = 0.84f;
	const int THICKNESS = 5;

	for (int row = 0; row < height; row++)
	{
		for (int col = 0; col < width; col++)
		{
			int idx = row * width + col;

			DWORD original = pixels[idx];
			DWORD alpha = original & 0xFF000000;

			if (alpha == 0)
				continue;

			BYTE r = static_cast<BYTE>(
				GetBValue(original) * RATIO +
				GetBValue(ice[idx]) * (1 - RATIO));

			BYTE g = static_cast<BYTE>(
				GetGValue(original) * RATIO +
				GetGValue(ice[idx]) * (1 - RATIO));

			BYTE b = static_cast<BYTE>(
				GetRValue(original) * RATIO +
				GetRValue(ice[idx]) * (1 - RATIO));

			float brightness =
				(r / 255.0f) * 0.2126f +
				(g / 255.0f) * 0.7152f +
				(b / 255.0f) * 0.0722f;

			if (row >= highlight_y &&
				row < highlight_y + THICKNESS &&
				brightness >= THRESHOLD)
			{
				r = g = b = 255;
			}

			pixels[idx] = BGR(RGB(r, g, b)) | alpha;
		}
	}

	putimage_alpha(x, y, &img_current_frame);
}

//生成右向动画帧
void GenerateRightAtlas(const Atlas& left, Atlas& right)
{
	for (IMAGE* left_frame : left.frame_list)
	{
		int width = left_frame->getwidth();
		int height = left_frame->getheight();

		IMAGE* right_frame = new IMAGE();
		Resize(right_frame, width, height);

		DWORD* src = GetImageBuffer(left_frame);
		DWORD* dst = GetImageBuffer(right_frame);

		for (int y = 0; y < height; y++)
		{
			for (int x = 0; x < width; x++)
			{
				int src_idx = y * width + x;
				int dst_idx = y * width + (width - x - 1);

				dst[dst_idx] = src[src_idx];
			}
		}

		right.frame_list.push_back(right_frame);
	}
}

// 死亡界面的确认按钮范围
const RECT GAME_OVER_CONFIRM = { 544, 430, 736, 505 };

// 在 EasyX 窗口内创建视频子窗口，使用已转换的 AVI
class EndingVideo
{
public:
	~EndingVideo() { Close(); }

	bool Start(HWND parent)
	{
		Close();
		wchar_t full_path[MAX_PATH] = {};
		DWORD length = GetFullPathNameW(L"video\\game_over.avi",
			MAX_PATH, full_path, nullptr);
		if (length == 0 || length >= MAX_PATH ||
			GetFileAttributesW(full_path) == INVALID_FILE_ATTRIBUTES)
		{
			MessageBoxW(parent, L"找不到 video/game_over.avi。请检查调试工作目录。",
				L"视频播放失败", MB_OK | MB_ICONERROR);
			return false;
		}

		// 路径有空格也能打开；style child 把画面嵌入游戏窗口
		std::wstring command = L"open \"" + std::wstring(full_path) +
			L"\" type avivideo alias ending_video style child parent " +
			std::to_wstring(reinterpret_cast<UINT_PTR>(parent));
		if (!Execute(command.c_str(), parent))
			return false;
		opened = true;

		std::wstring size = L"0 0 " + std::to_wstring(WINDOW_WIDTH) +
			L" " + std::to_wstring(WINDOW_HEIGHT);
		if (!Execute(L"set ending_video time format milliseconds", parent) ||
			!Execute((L"put ending_video window at " + size).c_str(), parent) ||
			!Execute((L"put ending_video destination at " + size).c_str(), parent) ||
			!Execute(L"window ending_video state show", parent) ||
			!Execute(L"play ending_video from 0", parent))
		{
			Close();
			return false;
		}
		return true;
	}

	// 不阻塞主循环；正常播放完毕后，MCI 状态会变成 stopped
	bool IsFinished(HWND parent)
	{
		if (!opened)
			return true;
		wchar_t mode[64] = {};
		MCIERROR error = mciSendStringW(L"status ending_video mode",
			mode, 64, nullptr);
		if (error != 0)
		{
			ShowError(error, parent);
			return true; // 查询失败也会关闭视频并返回菜单
		}
		return std::wcscmp(mode, L"stopped") == 0;
	}

	void Close()
	{
		if (!opened)
			return;
		mciSendStringW(L"stop ending_video", nullptr, 0, nullptr);
		mciSendStringW(L"close ending_video", nullptr, 0, nullptr);
		opened = false;
	}

private:
	bool opened = false;

	static void ShowError(MCIERROR error, HWND parent)
	{
		wchar_t detail[256] = {};
		mciGetErrorStringW(error, detail, 256);
		std::wstring text = L"视频播放失败：";
		text += detail;
		MessageBoxW(parent, text.c_str(), L"视频播放", MB_OK | MB_ICONERROR);
	}

	static bool Execute(const wchar_t* command, HWND parent)
	{
		MCIERROR error = mciSendStringW(command, nullptr, 0, nullptr);
		if (error == 0)
			return true;
		ShowError(error, parent);
		return false;
	}
};

// 代替原来的阻塞式 MessageBox，让死亡界面可以接收按键
void DrawGameOverPanel(int score)
{
	LOGFONT old_font;
	gettextstyle(&old_font);
	COLORREF old_text = gettextcolor();
	COLORREF old_fill = getfillcolor();
	COLORREF old_line = getlinecolor();
	int old_bk = getbkmode();

	setbkmode(TRANSPARENT);
	setfillcolor(RGB(17, 23, 30));
	solidroundrect(320, 180, 960, 540, 20, 20);
	setlinecolor(RGB(89, 174, 185));
	roundrect(320, 180, 960, 540, 20, 20);

	settextcolor(RGB(255, 228, 133));
	settextstyle(38, 0, _T("Microsoft YaHei"));
	LPCTSTR title = _T("游戏结束");
	outtextxy((WINDOW_WIDTH - textwidth(title)) / 2, 215, title);

	TCHAR text[128];
	_stprintf_s(text, _T("最终得分：%d"), score);
	settextstyle(26, 0, _T("Microsoft YaHei"));
	settextcolor(RGB(245, 245, 245));
	outtextxy((WINDOW_WIDTH - textwidth(text)) / 2, 290, text);

	settextstyle(22, 0, _T("Microsoft YaHei"));
	LPCTSTR hint = _T("按 1 观看视频，播放结束后返回主菜单");
	outtextxy((WINDOW_WIDTH - textwidth(hint)) / 2, 350, hint);

	setfillcolor(RGB(222, 180, 57));
	solidroundrect(GAME_OVER_CONFIRM.left, GAME_OVER_CONFIRM.top,
		GAME_OVER_CONFIRM.right, GAME_OVER_CONFIRM.bottom, 12, 12);
	settextcolor(RGB(25, 32, 25));
	LPCTSTR confirm = _T("确认");
	outtextxy((GAME_OVER_CONFIRM.left + GAME_OVER_CONFIRM.right -
		textwidth(confirm)) / 2,
		(GAME_OVER_CONFIRM.top + GAME_OVER_CONFIRM.bottom -
			textheight(confirm)) / 2, confirm);

	settextstyle(&old_font);
	settextcolor(old_text);
	setfillcolor(old_fill);
	setlinecolor(old_line);
	setbkmode(old_bk);
}

// 在启动时固定资源查找目录，之后不再改变它
bool UseApplicationDirectory()
{
	wchar_t path[32768] = {};
	DWORD length = GetModuleFileNameW(nullptr, path, _countof(path));
	if (length == 0 || length >= _countof(path))
		return false;

	wchar_t* last_slash = std::wcsrchr(path, L'\\');
	if (!last_slash)
		return false;
	*(last_slash + 1) = L'\0'; // 保留末尾反斜杠，也支持 exe 位于磁盘根目录
	return SetCurrentDirectoryW(path) != FALSE;
}

// ID 1 来自 packaging/app_icon.rc；同时设置标题栏和任务栏图标
void ApplyApplicationIcon(HWND hwnd)
{
	HINSTANCE instance = GetModuleHandleW(nullptr);
	HICON large_icon = static_cast<HICON>(LoadImageW(instance,
		MAKEINTRESOURCEW(1), IMAGE_ICON,
		GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), LR_SHARED));
	HICON small_icon = static_cast<HICON>(LoadImageW(instance,
		MAKEINTRESOURCEW(1), IMAGE_ICON,
		GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_SHARED));

	if (large_icon)
		SendMessageW(hwnd, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(large_icon));
	if (small_icon)
		SendMessageW(hwnd, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(small_icon));
	SetWindowTextW(hwnd, L"奶蛋求生");
}

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
	if (!UseApplicationDirectory())
	{
		MessageBoxW(nullptr, L"无法确定游戏资源目录。", L"启动失败",
			MB_OK | MB_ICONERROR);
		return 1;
	}

	initgraph(1280, 720);
	ApplyApplicationIcon(GetHWnd()); // 这一行新增：应用图标与窗口标题
	if (!InstallVideoWindowSupport(GetHWnd()))
	{
		MessageBoxW(GetHWnd(), L"无法设置视频窗口的显示与关闭处理。",
			L"窗口初始化失败", MB_OK | MB_ICONERROR);
		RestoreGameWindowProc(GetHWnd());
		closegraph();
		return 1;
	}
	loadimage(&img_ice, _T("img/ice.png"), 80, 80);
	loadimage(&img_bullet, _T("img/bullet.png"), 32, 32);
	loadimage(&img_hp_bar_bg, _T("img/hp_bar_bg.png"), 64, 12);
	loadimage(&img_hp_bar_fill, _T("img/hp_bar_fill.png"), 60, 8);
	loadimage(&img_ui_freeze, _T("img/ui_freeze.png"), 64, 64);
	const LPCTSTR heal_paths[3] =
	{
		_T("img/heal_normal.png"),
		_T("img/heal_elite.png"),
		_T("img/heal_boss.png")
	};
	for (int i = 0; i < 3; i++)
		loadimage(&img_heal_pickup[i], heal_paths[i], 24, 24);
	for (int i = 0; i < 6; i++)
	{
		TCHAR path[128];
		_stprintf_s(path, _T("img/fire_%d.png"), i);
		loadimage(&img_fire[i], path, 32, 32);
	}
	std::srand(static_cast<unsigned>(std::time(nullptr)));
	atlas_player_left = new Atlas(_T("img/player_left_%d.png"), 6);
	atlas_player_right = new Atlas();
	GenerateRightAtlas(*atlas_player_left, *atlas_player_right);
	const LPCTSTR enemy_left_paths[3] =
	{
		_T("img/enemy_left_%d.png"),
		_T("art_resources/enemy_variant_2/enemy_left_%d.png"),
		_T("art_resources/enemy_variant_3/enemy_left_%d.png")
	};

	for (int i = 0; i < 3; i++)
	{
		atlas_enemy_left[i] = new Atlas(enemy_left_paths[i], 6);
		atlas_enemy_right[i] = new Atlas();
		GenerateRightAtlas(*atlas_enemy_left[i], *atlas_enemy_right[i]);
	}

	mciSendString(_T("open mus/bgm.mp3 alias bgm"), NULL, 0, NULL);
	mciSendString(_T("open mus/hit_original_2s.wav alias hit"), NULL, 0, NULL);
	mciSendString(_T("open mus/dead.wav alias dead"), NULL, 0, NULL);

	int score = 0;
	int next_hp_score_milestone = 50; // 玩家成长：总分每 50 分
	int next_boss_score = 50;         // BOSS：首只 50 分，之后击杀后再累计 50
	Player player;
	ExMessage msg;
	IMAGE img_menu;
	IMAGE img_background;

	std::vector<Enemy*> enemy_list;
	std::vector<HealPickup> heal_pickups; // 地上等待玩家拾取的血包
	std::vector<Bullet> bullet_list(4); // 四颗子弹，相邻间隔 90 度

	RECT region_btn_start_game, region_btn_quit_game;

	region_btn_start_game.left = (WINDOW_WIDTH - BUTTON_WIDTH) / 2;
	region_btn_start_game.right = region_btn_start_game.left + BUTTON_WIDTH;
	region_btn_start_game.top = 430;
	region_btn_start_game.bottom = region_btn_start_game.top + BUTTON_HEIGHT;
	
	region_btn_quit_game.left = (WINDOW_WIDTH - BUTTON_WIDTH) / 2;
	region_btn_quit_game.right = region_btn_quit_game.left + BUTTON_WIDTH;
	region_btn_quit_game.top = 550;
	region_btn_quit_game.bottom = region_btn_quit_game.top + BUTTON_HEIGHT;

	StartGameButton btn_start_game = StartGameButton(region_btn_start_game,
		_T("img/ui_start_idle.png"), _T("img/ui_start_hovered.png"), _T("img/ui_start_pushed.png"));
	QuitGameButton btn_quit_game = QuitGameButton(region_btn_quit_game,
		_T("img/ui_quit_idle.png"), _T("img/ui_quit_hovered.png"), _T("img/ui_quit_pushed.png"));

	loadimage(&img_menu, _T("img/menu.png"));
	loadimage(&img_background, _T("img/background.png"));
	
	EndingVideo ending_video;

	// 每次返回菜单或开始新局，都清理上一局的状态
	auto ResetRound = [&]()
		{
			for (Enemy* enemy : enemy_list)
				delete enemy;
			enemy_list.clear();
			heal_pickups.clear();
			player.Reset();
			score = 0;
			next_hp_score_milestone = 50;
			next_boss_score = 50;
			TryGenerateEnemy(enemy_list, 0, true);
			UpdateBullets(bullet_list, player, 0, true);
			btn_start_game.ResetState();
			btn_quit_game.ResetState();
			mciSendString(_T("stop bgm"), nullptr, 0, nullptr);
			mciSendString(_T("stop hit"), nullptr, 0, nullptr);
			mciSendString(_T("stop dead"), nullptr, 0, nullptr);
		};

	auto ReturnToMenu = [&]()
		{
			ResetRound();
			scene = Scene::Menu;
		};

	BeginBatchDraw();
	DWORD last_time = GetTickCount();
	while (running)
	{
		// 视频由原生子窗口绘制；这一分支不清屏、不刷新 EasyX 画面
		if (scene == Scene::Video)
		{
			MSG native_msg = {};
			while (PeekMessageW(&native_msg, nullptr, 0, 0, PM_REMOVE))
			{
				if (native_msg.message == WM_QUIT)
				{
					running = false; // 真正的线程退出消息仍正常处理
					break;
				}
				TranslateMessage(&native_msg);
				DispatchMessageW(&native_msg);
			}
			while (peekmessage(&msg)) {} // 视频中的输入不传给游戏或菜单

			bool close_requested = video_close_requested.exchange(false);
			if (running && (close_requested || ending_video.IsFinished(GetHWnd())))
			{
				// 点击 × 和自然播放完毕共用这一条返回菜单流程
				ending_video.Close();
				video_is_playing.store(false);
				video_close_requested.store(false);
				ReturnToMenu();
				flushmessage();
				last_time = GetTickCount();

				// 视频子窗口移除后立即画菜单，避免停留在黑屏
				cleardevice();
				putimage(0, 0, &img_menu);
				btn_start_game.Draw();
				btn_quit_game.Draw();
				FlushBatchDraw();
			}
			Sleep(10);
			continue;
		}

		DWORD start_time = GetTickCount();
		int delta = static_cast<int>(start_time - last_time);
		last_time = start_time;

		while (peekmessage(&msg))
		{
			if (scene == Scene::Menu)
			{
				btn_start_game.ProcessEvent(msg);
				if (scene == Scene::Playing)
				{
					ResetRound();
					mciSendString(_T("play bgm repeat from 0"), nullptr, 0, nullptr);
					last_time = GetTickCount();
					delta = 0;
					flushmessage();
					break;
				}
				btn_quit_game.ProcessEvent(msg);
			}
			else if (scene == Scene::Playing)
			{
				player.ProcessEvent(msg);
			}
			else if (scene == Scene::GameOver)
			{
				if (msg.message == WM_KEYDOWN &&
					(msg.vkcode == '1' || msg.vkcode == VK_NUMPAD1))
				{
					mciSendString(_T("stop dead"), nullptr, 0, nullptr);
					mciSendString(_T("stop hit"), nullptr, 0, nullptr);
					cleardevice();
					FlushBatchDraw(); // 打开视频前只刷新一次黑底，仍保持批量绘制
					video_close_requested.store(false);
					video_is_playing.store(true);
					if (ending_video.Start(GetHWnd()))
						scene = Scene::Video;
					else
					{
						video_is_playing.store(false);
						video_close_requested.store(false);
						ReturnToMenu(); // 播放失败也返回菜单
					}
					flushmessage();
					last_time = GetTickCount();
					delta = 0;
					break;
				}
				if (msg.message == WM_LBUTTONUP &&
					msg.x >= GAME_OVER_CONFIRM.left && msg.x <= GAME_OVER_CONFIRM.right &&
					msg.y >= GAME_OVER_CONFIRM.top && msg.y <= GAME_OVER_CONFIRM.bottom)
				{
					ReturnToMenu();
					flushmessage();
					last_time = GetTickCount();
					delta = 0;
					break;
				}
			}
		}

		if (!running)
			break;
		if (scene == Scene::Video)
			continue;

		if (scene == Scene::Playing)
		{
			player.Update(delta);
			player.Move(delta);
			UpdateBullets(bullet_list, player, delta);
			TryGenerateEnemy(enemy_list, delta);

			// 先结算上一帧已有火焰的伤害，再处理本帧新的子弹接触
			for (Enemy* enemy : enemy_list)
			{
				enemy->UpdateBurn(delta);
				enemy->Move(player, delta);
			}

			if (player.ConsumeFreezeAttack())
			{
				POINT center = player.GetCenter();
				for (Enemy* enemy : enemy_list)
					if (enemy->CheckAlive() && enemy->IsInFreezeRange(center, 250.0))
						enemy->Hurt(10);
			}

			for (Enemy* enemy : enemy_list)
				enemy->CheckBulletHits(bullet_list);
			SpreadBurning(enemy_list);

			HandleEnemyDeaths(enemy_list, heal_pickups, score, next_boss_score);

			// 玩家每 50 分成长独立计算，不与 BOSS 的生成门槛绑定
			while (score >= next_hp_score_milestone)
			{
				player.IncreaseMaxHP(10);
				next_hp_score_milestone += 50;
			}
			TryGenerateBoss(enemy_list, score, next_boss_score);
			UpdateHealPickups(heal_pickups, player);

			int contact_damage = 0;
			for (Enemy* enemy : enemy_list)
				if (enemy->CheckAlive() && enemy->CheckPlayerCollision(player))
					contact_damage = (std::max)(contact_damage, enemy->GetContactDamage());
			player.TakeDamage(contact_damage);


			if (!player.IsAlive())
			{
				mciSendString(_T("stop bgm"), nullptr, 0, nullptr);
				mciSendString(_T("play dead from 0"), nullptr, 0, nullptr);
				scene = Scene::GameOver; // 死亡只切换界面，进程继续运行
				flushmessage(); // 不让死亡前排队的输入立即跳过死亡界面
			}
		}

		cleardevice();
		if (scene == Scene::Playing)
		{
			putimage(0, 0, &img_background);
			DrawHealPickups(heal_pickups); // 血包铺在地面层
			player.Draw(delta);
			for (Enemy* enemy : enemy_list)
				enemy->Draw(delta);        // 含脚底火焰
			for (const Bullet& bullet : bullet_list)
				bullet.Draw();
			for (Enemy* enemy : enemy_list)
				enemy->DrawHealthBar();    // 精英和 BOSS 均显示
			DrawPlayerScore(score);
			player.DrawHealthBar();
			player.DrawFreezeHUD();
		}
		else if (scene == Scene::GameOver)
		{
			// 保留死亡时的战斗画面；delta=0 不继续推进动画
			putimage(0, 0, &img_background);
			DrawHealPickups(heal_pickups);
			player.Draw(0);
			for (Enemy* enemy : enemy_list)
				enemy->Draw(0);
			for (const Bullet& bullet : bullet_list)
				bullet.Draw();
			for (Enemy* enemy : enemy_list)
				enemy->DrawHealthBar();
			DrawGameOverPanel(score);
		}
		else
		{
			putimage(0, 0, &img_menu);
			btn_start_game.Draw();
			btn_quit_game.Draw();
		}
		FlushBatchDraw();

		DWORD delta_time = GetTickCount() - start_time;
		if (delta_time < 1000 / 144)
			Sleep(1000 / 144 - delta_time);
	}

	// 退出时清理场上尚未死亡的敌人
	for (Enemy* enemy : enemy_list)
		delete enemy;
	enemy_list.clear();

	delete atlas_player_left;
	delete atlas_player_right;

	for (int i = 0; i < 3; i++)
	{
		delete atlas_enemy_left[i];
		delete atlas_enemy_right[i];
	}

	ending_video.Close();
	video_is_playing.store(false);
	video_close_requested.store(false);
	mciSendString(_T("close bgm"), nullptr, 0, nullptr);
	mciSendString(_T("close hit"), nullptr, 0, nullptr);
	mciSendString(_T("close dead"), nullptr, 0, nullptr);
	RestoreGameWindowProc(GetHWnd()); // 退出前恢复 EasyX 原窗口处理函数
	EndBatchDraw();                 // 只在真正退出游戏时结束批量绘制
	closegraph();

	return 0;
}