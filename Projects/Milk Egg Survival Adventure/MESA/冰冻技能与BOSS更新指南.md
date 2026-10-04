# 冰冻图标、输入修复与 BOSS 血条更新指南

基于 2026-10-04 读取的 main.cpp，共 1101 行。本文只提供手动修改指导，没有修改 main.cpp 或项目配置。插入代码后行号会变化，优先 Ctrl+F 搜索代码名称。
本次不要重复替换上次指南中的整个 Player / Enemy 类，按下面的小范围修改即可。

## 当前状态和本次目标

| 内容 | 当前文件 | 修改后 |
|---|---|---|
| 玩家初始血量 | 已是 50/50（第 341 行） | 保持 50/50 |
| 子弹数量 | 3（第 956 行） | 4，等间距，继续逆时针 |
| 子弹角速度 | 2 弧度/秒（第 781 行） | 3 弧度/秒 |
| BOSS 冲刺速度 | 8（第 488 行） | 5 |
| BOSS 血条 | 尚未绘制 | BOSS 上方 64×12，红条按剩余比例裁切 |
| 冰冻技能图标 | 尚未加载 | 右下角 64×64，F 提示、冷却变暗和秒数 |
| F 键触发 | WM_KEYDOWN 消息 | 保留原分支，开始游戏时切换英文键盘 |

技能冻结 2 秒、结束后继续无敌 3 秒、冷却 10 秒、范围伤害 10，以及其他怪物/积分/回血规则沿用当前代码。
图标素材为 img/ui_freeze.png。复用 img/hp_bar_bg.png 和 img/hp_bar_fill.png，不需要新 BOSS 血条图片。

## 1. 三项基础数值

### 1.1 BOSS 冲刺速度

搜索 Enemy::Move 中的 case BossPhase::Dash，原第 488 行：

~~~cpp
double step = 8.0 * MOVE_REFERENCE_FPS * delta / 1000.0;
~~~

替换为：

~~~cpp
// BOSS 冲刺速度下调为 5；距离仍为 300 像素
double step = 5.0 * MOVE_REFERENCE_FPS * delta / 1000.0;
~~~

不要修改 ChasePlayer 中的普通追踪速度。速度 5 按原项目 144 FPS 基准换算为每秒 720 像素，300 像素冲刺约 0.42 秒。

### 1.2 子弹数量

搜索 main() 中的 bullet_list(3)，原第 956 行，替换为：

~~~cpp
std::vector<Bullet> bullet_list(4); // 四颗子弹，相邻间隔 90 度
~~~

现有 UpdateBullets 用 2*PI/bullet_list.size() 自动等分角度，CheckBulletHits 也按实际数量保存碰撞状态，两处循环不用额外修改。

### 1.3 子弹旋转速度和玩家血量

在 UpdateBullets 中搜索 ANGULAR_SPEED（原第 781 行），改成：

~~~cpp
const double ANGULAR_SPEED = 3.0; // 弧度/秒，比原来快 50%
~~~

旋转一圈约 2.09 秒。保留 position.y = center_y - ... 的减号，它使屏幕上的旋转方向为逆时针。

Player 的原第 341 行已经是以下内容，确认保留即可：

~~~cpp
int hp = 50, max_hp = 50; // 出生时当前血量和上限均为 50
~~~

每 50 分调用 IncreaseMaxHP(10) 的逻辑不变，因此获得 50 分时上限为 60；已有伤势仍会保留。

## 2. 进入游戏时自动切换英文键盘（按用户最新选择）

保留当前 Player::ProcessEvent 中的 F 键分支和 freeze_key_down 防重复变量。本节取代此前的 PollFreezeInput 方案。

搜索 class StartGameButton : public Button，当前约第 673 行。
只替换这个类中的 OnClick()（当前第 681～686 行），不要替换 Button 基类或 QuitGameButton 中的同名方法：

~~~cpp
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

    is_game_started = true;
    mciSendString(_T("play bgm repeat from 0"), NULL, 0, NULL);
}
~~~

00000409 是 Windows 的英文（美国）键盘布局标识；LoadKeyboardLayoutW 加载并激活该布局。WM_INPUTLANGCHANGEREQUEST 通知窗口切换输入布局。
参考：[Microsoft LoadKeyboardLayout 文档](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-loadkeyboardlayouta)、[窗口切换输入语言消息](https://learn.microsoft.com/en-us/windows/win32/winmsg/wm-inputlangchangerequest)。

在本项目中 graphics.h 已提供 Windows API 声明，不需要新增头文件或插件。
当前 main.cpp 尚未接入 PollFreezeInput，所以只修改上述 OnClick() 即可。
如果已经照旧版指南添加了 PollFreezeInput、freeze_input_active 和主循环中的 player.PollFreezeInput(...)，删除这三处，并恢复原来的 F 按下/松开分支。

此方案在点击开始游戏时切换一次。若进入游戏后自行切回中文输入法，原来的 F 消息处理仍可能受输入法影响。

## 3. 添加右下角冰冻技能图标

### 3.1 声明和加载图片

文件开头原第 19 行 IMAGE img_hp_bar_fill; 后添加：

~~~cpp
IMAGE img_ui_freeze; // 右下角冰冻技能图标
~~~

main() 中，原第 923 行 loadimage(&img_hp_bar_fill, ...) 后添加：

~~~cpp
loadimage(&img_ui_freeze, _T("img/ui_freeze.png"), 64, 64);
~~~

运行时工作目录与现有 img/background.png 一致即可。

### 3.2 让透明绘图支持整体变暗

搜索文件顶部的 putimage_alpha（原第 38～44 行），将这个函数整体替换为：

~~~cpp
// 默认完全显示；opacity 降低时，图标会透出背景而显得变暗
inline void putimage_alpha(int x, int y, IMAGE* img, BYTE opacity = 255)
{
    int w = img->getwidth();
    int h = img->getheight();
    AlphaBlend(GetImageHDC(NULL), x, y, w, h,
        GetImageHDC(img), 0, 0, w, h,
        { AC_SRC_OVER, 0, opacity, AC_SRC_ALPHA });
}
~~~

现有三参数调用依旧使用 255；技能冷却时单独传 110。
putimage_alpha_width 不需要修改。

### 3.3 给 Player 添加 HUD 绘制方法

在 Player 中，搜索 void DrawHealthBar() const（原第 316 行）。
在该方法结束后、Player 的 private: 之前（原第 327 行之前）插入：

~~~cpp
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
~~~

当前 1280×720 窗口里，图标左上角约为 (1196, 614)，距离右边 20 像素，下方留有状态文字。
这个图标是状态显示，用 F 释放技能，不增加鼠标点击释放。

## 4. 给 BOSS 添加独立血条

### 4.1 为每只敌人保存自己的血量上限

搜索 Enemy 类 private: 中原第 576 行：

~~~cpp
int hp = 1, score_value = 1, contact_damage = 1;
~~~

替换为：

~~~cpp
int hp = 1, max_hp = 1; // 每个敌人实例都有自己的当前血量和上限
int score_value = 1, contact_damage = 1;
~~~

在 Enemy 构造函数中，搜索原第 380 行 hp = hp_table[index];，紧接着添加：

~~~cpp
max_hp = hp; // BOSS 上限记录为 20，不随扣血变化
~~~

### 4.2 添加 BOSS 血条绘制方法

在 Enemy 的 void Draw(int delta) 方法结束后（原第 525 行之后）、
第一个 private:（原第 527 行）之前插入：

~~~cpp
void DrawHealthBar() const
{
    // 普通怪和精英不显示本次新增的 BOSS 血条
    if (type != EnemyType::Boss || !CheckAlive())
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
~~~

Enemy 和 Player 都有 DrawHealthBar() 是允许的，它们属于不同的类。
本方法只读取 Enemy 自己的 hp / max_hp，不会跟玩家共享血量；多个 BOSS 也分别显示。

## 5. 主循环接入所有显示

在 main() 中，找到 cleardevice(); 后的 if (is_game_started) 绘制分支。
这是原第 1056～1066 行；不要替换原第 997 行的游戏逻辑更新分支。

把这段绘制分支替换为：

~~~cpp
if (is_game_started)
{
    putimage(0, 0, &img_background);
    player.Draw(delta);

    for (Enemy* enemy : enemy_list)
        enemy->Draw(delta);

    for (const Bullet& bullet : bullet_list)
        bullet.Draw();

    // 血条放在角色和子弹之后绘制，避免被遮住
    for (Enemy* enemy : enemy_list)
        enemy->DrawHealthBar();

    DrawPlayerScore(score);
    player.DrawHealthBar();
    player.DrawFreezeHUD(); // HUD 最后绘制，固定在右下角
}
~~~

紧跟后面的 else 菜单绘制分支、FlushBatchDraw()、限帧和资源清理保持原有内容。
IMAGE img_ui_freeze 是全局对象，无须额外 new/delete。

## 6. 修改后逐项调试

1. 开始前保持中文输入法，点击开始游戏后确认任务栏切到 ENG / 英文（美国）；按 F 时玩家立即进入冻结和无敌闪烁，图标显示“冻结中”。
2. 2 秒后角色恢复移动，图标变暗并显示剩余冷却秒数；10 秒结束时图标恢复亮度并显示“冰冻”。
3. 一直按住 F 不应重复释放；冷却结束后松开再按可再次释放。
4. 重新启动游戏，保持中文输入法进入菜单，再点击开始；确认每次开始都会切到英文键盘。
5. 玩家周围有 4 颗子弹，相邻约 90 度，逆时针旋转，一圈约 2.09 秒。
6. 玩家刚出生时 GetHP()/GetMaxHP() 均为 50，血条全红。可在 VS “监视”窗口查看，不需要为测试新增游戏代码。
7. 首次跨过 50 分出现 BOSS，20/20 时血条全红；一次子弹伤害后为 19/20，红条宽 57 像素。
8. BOSS 满血时只吃一次冰冻伤害，应变成 10/20（如同时被子弹命中会更低），血条显示约一半红色。
9. BOSS 平常追踪 5 秒后锁定方向，速度 5 冲刺 300 像素，休息 3 秒。每个 BOSS 血条独立。
10. 获得 50 分后玩家上限 60，后续每个 50 分门槛继续加 10；回血规则仍按当前实现。

## 常见编译/显示问题

| 现象 | 检查位置 |
|---|---|
| DrawFreezeHUD 不是 Player 成员 | 方法是否放在 Player 的 public: 内、类结束 }; 之前 |
| img_ui_freeze 未定义 | 是否添加了全局 IMAGE |
| putimage_alpha 不接受 4 个参数 | 顶部 helper 是否已经整体替换并加 BYTE opacity = 255 |
| Enemy 的 max_hp 未定义 | 第 4.1 步是否添加了字段并在构造函数赋值 |
| BOSS 血条永远全红或比例错误 | max_hp = hp 只放构造函数，不要放 Hurt() 或 DrawHealthBar() |
| 图标不显示 | 检查 img/ui_freeze.png、loadimage 调用和 DrawFreezeHUD() 调用 |
| F 还是不响应 | 检查是否修改了 StartGameButton 的 OnClick，任务栏是否切到英文，F 分支是否保留，技能冷却是否为 0 |
| 中文显示乱码 | 保留当前 Unicode 字符集和 /utf-8 项目选项，源文件保存为 UTF-8 |

素材已生成并检查为 64×64、带透明背景。上一版指南的 HUD、数值和血条代码曾在临时副本通过 MSVC 语法检查。当前键盘方案按用户新选择改为进入时切换英文，需要手动接入后验证输入布局切换。main.cpp 与项目配置没有改动；未链接或运行该副本。你手动接入后的完整编译和中文输入法运行效果，仍需按上面的步骤在 VS 中验证。
