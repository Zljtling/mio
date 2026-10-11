#define main game_application_main
#include "../../main.cpp"
#undef main

#include <fcntl.h>
#include <io.h>
#include <cstdio>
#include <cstdlib>

static constexpr int FRAME_RATE = 24;
static constexpr int FRAME_MS = 1000 / FRAME_RATE;

static void send_key(Player* player, BYTE key, UINT message)
{
    ExMessage event = {};
    event.message = message;
    event.vkcode = key;
    player->on_input(event);
}

static void tap(Player* player, BYTE key)
{
    send_key(player, key, WM_KEYDOWN);
    send_key(player, key, WM_KEYUP);
}

static void selector_right(SelectorScene& selector, BYTE key)
{
    ExMessage event = {};
    event.message = WM_KEYUP;
    event.vkcode = key;
    selector.on_input(event);
}

static void start_fight(GameScene& stage, int first, int second)
{
    auto create = [](int character) -> Player*
    {
        if (character == 0) return new JotaroPlayer();
        if (character == 1) return new KakyoinPlayer();
        return new NaiwaPlayer();
    };
    IMAGE* avatars[] = { &img_avatar_Jotaro, &img_avatar_Kakyoin, &img_avatar_Naiwa };
    player_1 = create(first);
    player_2 = create(second);
    player_1->set_id(PlayerID::P1);
    player_2->set_id(PlayerID::P2);
    img_player_1_avatar = avatars[first];
    img_player_2_avatar = avatars[second];
    stage.on_enter();
    player_1->set_position(1380, GAME_STREET_FLOOR_Y - 126);
    player_2->set_position(1540, GAME_STREET_FLOOR_Y - 126);
    player_1->set_mp(100);
    player_2->set_mp(100);
    send_key(player_2, VK_LEFT, WM_KEYDOWN);
    stage.on_update(16);
    send_key(player_2, VK_LEFT, WM_KEYUP);
}

static void draw_chapter(LPCTSTR title, LPCTSTR subtitle)
{
    DebugDrawState state;
    setfillcolor(RGB(7, 13, 31));
    solidroundrect(32, 25, 760, 107, 10, 10);
    setbkmode(TRANSPARENT);
    settextstyle(31, 0, _T("Microsoft YaHei"));
    settextcolor(RGB(255, 221, 105));
    outtextxy(54, 32, title);
    settextstyle(18, 0, _T("Microsoft YaHei"));
    settextcolor(RGB(195, 220, 241));
    outtextxy(56, 76, subtitle);
}

static void scripted_actions(int chapter, int frame)
{
    if (chapter == 0) // 奶娃 vs 承太郎
    {
        if (frame == 20 || frame == 370) tap(player_1, 'F');
        if (frame == 70 || frame == 450) tap(player_1, 'S');
        if (frame == 118) { player_1->set_mp(100); tap(player_1, 'E'); }
        if (frame == 173) tap(player_2, VK_OEM_COMMA);
        if (frame == 225) { player_1->set_mp(100); tap(player_1, 'Q'); }
        if (frame == 390) tap(player_2, VK_OEM_COMMA);
        if (frame == 478) tap(player_1, 'W');
    }
    else if (chapter == 1) // 花京院 vs 承太郎
    {
        if (frame == 24 || frame == 56 || frame == 400) tap(player_1, 'F');
        if (frame == 90 || frame == 455) { player_1->set_mp(100); tap(player_1, 'E'); }
        if (frame == 132 || frame == 490) tap(player_2, VK_OEM_COMMA);
        if (frame == 210) { player_1->set_mp(100); tap(player_1, 'Q'); }
        if (frame == 320) { player_2->set_mp(100); tap(player_2, VK_OEM_PERIOD); }
        if (frame == 445) tap(player_1, 'W');
    }
    else // 承太郎 vs 奶娃
    {
        if (frame == 22 || frame == 70) tap(player_1, 'F');
        if (frame == 95) { player_1->set_mp(100); tap(player_1, 'E'); }
        if (frame == 175) tap(player_2, VK_DOWN);
        if (frame == 230) { player_1->set_mp(100); tap(player_1, 'Q'); }
        if (frame == 256 || frame == 276 || frame == 296 || frame == 320) tap(player_1, 'F');
        if (frame == 405) { player_2->set_mp(100); tap(player_2, VK_OEM_2); }
        if (frame == 505) { player_2->set_mp(100); tap(player_2, VK_OEM_PERIOD); }
        if (frame == 555) tap(player_1, 'W');
    }
}

int main(int argc, char** argv)
{
    int start = argc > 1 ? std::atoi(argv[1]) : 0;
    int count = argc > 2 ? std::atoi(argv[2]) : 2160;
    if (start < 0 || count <= 0 || start + count > 2160) return 2;
    if (_setmode(_fileno(stdout), _O_BINARY) == -1) return 3;
    std::setvbuf(stdout, nullptr, _IOFBF, 1 << 20);
    std::srand(5);
    load_game_resources();
    initgraph(GAME_WINDOW_WIDTH, GAME_WINDOW_HEIGHT);
    ShowWindow(GetHWnd(), SW_HIDE);
    IMAGE output(GAME_WINDOW_WIDTH, GAME_WINDOW_HEIGHT);
    SetWorkingImage(&output);
    setbkmode(TRANSPARENT);
    MenuScene menu;
    SelectorScene selector;
    GameScene stage;
    bool fighting = false;

    for (int frame = start; frame < start + count; frame++)
    {
        if (frame < 120)
        {
            menu.on_draw(main_camera);
            draw_chapter(_T("JOJO 格斗游戏"), _T("90 秒实机演示 · 三名角色与城市街道战场"));
        }
        else if (frame < 360)
        {
            if (frame == 120) selector.on_enter();
            if (frame == 145) selector_right(selector, 'D');
            if (frame == 185) selector_right(selector, VK_RIGHT);
            if (frame == 230) selector_right(selector, 'D');
            if (frame == 275) selector_right(selector, VK_RIGHT);
            selector.on_update(FRAME_MS);
            selector.on_draw(main_camera);
            draw_chapter(_T("自由选择三名角色"), _T("承太郎 · 花京院 · 奶娃"));
        }
        else if (frame < 2040)
        {
            int chapter = frame < 900 ? 0 : (frame < 1440 ? 1 : 2);
            int beginning = chapter == 0 ? 360 : (chapter == 1 ? 900 : 1440);
            int local = frame - beginning;
            if (local == 0)
            {
                if (fighting) stage.on_exit();
                start_fight(stage, chapter == 0 ? 2 : (chapter == 1 ? 1 : 0),
                    chapter == 1 ? 0 : (chapter == 0 ? 0 : 2));
                fighting = true;
            }
            player_1->set_hp(player_1->get_max_hp());
            player_2->set_hp(player_2->get_max_hp());
            scripted_actions(chapter, local);
            stage.on_update(FRAME_MS);
            stage.on_draw(main_camera);
            if (chapter == 0)
                draw_chapter(_T("奶娃 · 幻影 / 瞬移 / 庇护 / 声波"), _T("声波从自身向外扩散，命中后改变对手形态"));
            else if (chapter == 1)
                draw_chapter(_T("花京院 · 羽毛笔 / 绿宝石 / 结界"), _T("持续攻击与范围技能"));
            else
                draw_chapter(_T("承太郎 · 白金之星 / 时间停止"), _T("近身连打与时停攻势"));
        }
        else
        {
            if (frame == 2040 && fighting) { stage.on_exit(); fighting = false; }
            menu.on_draw(main_camera);
            draw_chapter(_T("开始你的对决"), _T("安装后选择角色，进入 3200 × 1800 城市街道"));
        }
        if (std::fwrite(GetImageBuffer(&output), sizeof(DWORD),
                GAME_WINDOW_WIDTH * GAME_WINDOW_HEIGHT, stdout) != GAME_WINDOW_WIDTH * GAME_WINDOW_HEIGHT)
            return 4;
    }
    if (fighting) stage.on_exit();
    std::fflush(stdout);
    SetWorkingImage(nullptr);
    closegraph();
    return 0;
}
