#define main game_application_main
#include "../../main.cpp"
#undef main
#include <iostream>

int main()
{
    load_game_resources();
    initgraph(GAME_WINDOW_WIDTH, GAME_WINDOW_HEIGHT);
    ShowWindow(GetHWnd(), SW_HIDE);
    IMAGE output(GAME_WINDOW_WIDTH, GAME_WINDOW_HEIGHT);
    SetWorkingImage(&output);
    setbkmode(TRANSPARENT);

    player_1 = new JotaroPlayer();
    player_2 = new NaiwaPlayer();
    player_1->set_id(PlayerID::P1);
    player_2->set_id(PlayerID::P2);
    img_player_1_avatar = &img_avatar_Jotaro;
    img_player_2_avatar = &img_avatar_Naiwa;
    if (player_1->get_max_hp() != 300 || player_2->get_max_hp() != 330
        || player_1->get_ultimate_cost() != 70 || player_2->get_ultimate_cost() != 50)
        return 1;

    GameScene stage;
    stage.on_enter();
    stage.on_update(16);
    stage.on_draw(main_camera);
    saveimage(_T("outputs/balance-timing/hud-not-ready.png"), &output);

    player_1->set_mp(70);
    player_2->set_mp(50);
    if (!player_1->can_release_ultimate() || !player_2->can_release_ultimate()) return 2;
    stage.on_update(400);
    stage.on_draw(main_camera);
    saveimage(_T("outputs/balance-timing/hud-ready.png"), &output);

    dynamic_cast<NaiwaPlayer*>(player_2)->on_attack_exxxx();
    player_2->set_mp(50);
    if (player_2->can_release_ultimate()) return 3;
    stage.on_update(16);
    stage.on_draw(main_camera);
    saveimage(_T("outputs/balance-timing/hud-busy.png"), &output);
    stage.on_exit();

    KakyoinPlayer kakyoin;
    if (kakyoin.get_max_hp() != 250 || kakyoin.get_ultimate_cost() != 80) return 4;
    kakyoin.set_mp(80);
    if (!kakyoin.can_release_ultimate()) return 5;

    SetWorkingImage(nullptr);
    closegraph();
    std::cout << "PASS latest CSV stats, three ultimate thresholds, ready and busy HUD states\n";
    return 0;
}
