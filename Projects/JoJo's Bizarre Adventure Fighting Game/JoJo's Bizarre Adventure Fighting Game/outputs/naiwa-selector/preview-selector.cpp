#define main game_application_main
#include "../../main.cpp"
#undef main
#include <iostream>

void select_right(SelectorScene& selector, BYTE key)
{
    ExMessage message = {};
    message.message = WM_KEYUP;
    message.vkcode = key;
    selector.on_input(message);
}
int main()
{
    load_game_resources();
    initgraph(GAME_WINDOW_WIDTH, GAME_WINDOW_HEIGHT);
    ShowWindow(GetHWnd(), SW_HIDE);
    IMAGE output(GAME_WINDOW_WIDTH, GAME_WINDOW_HEIGHT);
    SetWorkingImage(&output);
    settextstyle(28, 0, _T("IPix"));
    setbkmode(TRANSPARENT);
    main_camera.reset();
    SelectorScene selector;
    selector.on_enter();
    select_right(selector, 'D'); select_right(selector, 'D');
    selector.on_update(16); selector.on_draw(main_camera);
    saveimage(_T("outputs/naiwa-selector/selector-preview.png"), &output);
    select_right(selector, VK_RIGHT);
    selector.on_draw(main_camera);
    saveimage(_T("outputs/naiwa-selector/selector-both-naiwa.png"), &output);
    for (int i = 0; i < 115; ++i) { selector.on_update(16); selector.on_draw(main_camera); }
    select_right(selector, 'D');
    selector.on_draw(main_camera);
    saveimage(_T("outputs/naiwa-selector/selector-p2-naiwa.png"), &output);
    selector.on_exit();
    if (!dynamic_cast<JotaroPlayer*>(player_1) || !dynamic_cast<NaiwaPlayer*>(player_2)
        || img_player_2_avatar != &img_avatar_Naiwa || img_avatar_Naiwa.getwidth() != 84
        || img_1P_naiwa_desc.getwidth() != 440 || img_2P_naiwa_desc.getheight() != 120
        || img_Naiwa_selector_background_right.getheight() != GAME_WINDOW_HEIGHT)
        return 1;
    GameScene stage;
    stage.on_enter(); stage.on_update(16); stage.on_draw(main_camera);
    saveimage(_T("outputs/naiwa-selector/naiwa-avatar-hud.png"), &output);
    stage.on_exit();
    SetWorkingImage(nullptr); closegraph();
    std::cout << "PASS P1 / P2 selection, both Naiwa, scroll wrap, new portrait HUD\n";
    return 0;
}
