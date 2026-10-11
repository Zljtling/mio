#define main game_application_main
#include "../../main.cpp"
#undef main
#include <iostream>
#include <stdexcept>

void require(bool condition, const char* description)
{
    if (!condition) throw std::runtime_error(description);
    std::cout << "PASS " << description << '\n';
}
void key(Player& player, BYTE value, bool down)
{
    ExMessage message = {};
    message.message = down ? WM_KEYDOWN : WM_KEYUP;
    message.vkcode = value;
    player.on_input(message);
}
void frames(GameScene& scene, int count)
{
    for (int i = 0; i < count; ++i) scene.on_update(16);
}
int main()
{
    try
    {
        load_game_resources();
        initgraph(GAME_WINDOW_WIDTH, GAME_WINDOW_HEIGHT);
        ShowWindow(GetHWnd(), SW_HIDE);
        IMAGE output(GAME_WINDOW_WIDTH, GAME_WINDOW_HEIGHT);
        SetWorkingImage(&output);
        player_1 = new NaiwaPlayer(); player_1->set_id(PlayerID::P1);
        player_2 = new KakyoinPlayer(); player_2->set_id(PlayerID::P2);
        img_player_1_avatar = &img_avatar_Naiwa; img_player_2_avatar = &img_avatar_Kakyoin;
        GameScene stage;
        stage.on_enter(); frames(stage, 4);
        require(img_city_street.getwidth() == 3200 && img_city_street.getheight() == 1800, "city assets loaded at world size");
        require(platform_list.size() == 23 && img_platform_large.getwidth() == 512 && img_platform_small.getwidth() == 288, "trimmed ledges and continuous ground loaded");
        require(std::abs(player_1->get_position().y + 126 - GAME_STREET_FLOOR_Y) < 1, "spawn rests on street");
        stage.on_draw(main_camera);
        saveimage(_T("outputs/city-street/street-preview.png"), &output);

        player_1->set_position(350, GAME_STREET_FLOOR_Y - 126); frames(stage, 2);
        player_1->on_jump(); frames(stage, 70);
        require(std::abs(player_1->get_position().y + 126 - 1360) < 1, "jump reaches first exterior ledge");
        player_1->on_jump(); frames(stage, 70);
        require(std::abs(player_1->get_position().y + 126 - 1180) < 1, "jump reaches next exterior ledge");
        player_1->set_position(0, 1414); player_2->set_position(3072, 334); frames(stage, 2);
        require(main_camera.get_view_width() == 3200, "camera fits fighters at opposite world edges");
        stage.on_draw(main_camera);
        saveimage(_T("outputs/city-street/world-preview.png"), &output);
        key(*player_1, 'A', true); frames(stage, 20); key(*player_1, 'A', false);
        require(player_1->get_position().x == 0, "movement stays within world bounds");

        player_1->set_position(1200, 1414); player_2->set_position(1872, 1414); frames(stage, 2);
        player_1->apply_naiwa_transform(7000);
        const float before = player_1->get_position().x;
        key(*player_1, 'D', true); frames(stage, 10); key(*player_1, 'D', false);
        require(std::abs(player_1->get_position().x - before - 24) < 0.1f, "transformed movement uses 0.15 px/ms");
        frames(stage, 440);
        player_1->set_mp(100); player_1->on_attack_ex(); frames(stage, 50);
        player_1->set_mp(0);
        auto* hit = new AttackArea();
        hit->set_attack_kind(Bullet::AttackKind::Special);
        hit->set_position(0, 0); hit->set_size(100, 126); hit->set_damage(10);
        hit->set_duration(1000); hit->set_collide_target(PlayerID::P1);
        bullet_list.push_back(hit);
        Camera settled;
        main_camera = settled; main_camera.follow(player_1->get_position(), player_2->get_position(), 0, true);
        const Vector2 quiet = main_camera.get_position();
        player_1->on_update(16); main_camera.on_update(16);
        require(main_camera.get_position().x == quiet.x && main_camera.get_position().y == quiet.y, "miss does not shake camera");
        const int hp = player_1->get_hp();
        const Vector2 collision = player_1->get_collision_position();
        hit->set_position(collision.x, collision.y);
        player_1->on_update(16); main_camera.on_update(16);
        require(player_1->get_hp() == hp - 3 && player_1->get_mp() == 2, "guard reduces damage 70 percent and passive returns 2 energy");
        require(main_camera.get_position().x != quiet.x || main_camera.get_position().y != quiet.y, "guard impact shakes camera");
        main_camera.on_update(500);
        require(main_camera.get_position().x == quiet.x && main_camera.get_position().y == quiet.y, "shake returns to followed camera position");
        stage.on_exit();

        player_1 = new NaiwaPlayer(); player_1->set_id(PlayerID::P1);
        player_2 = new KakyoinPlayer(); player_2->set_id(PlayerID::P2);
        stage.on_enter(); player_2->set_position(1440, 1414); frames(stage, 2);
        player_1->set_mp(100); player_1->on_attack_exxxx(); frames(stage, 32);
        require(player_2->get_transformed() && player_2->get_hp() < player_2->get_max_hp(), "Naiwa expanding wave hits and transforms enemy");
        NaiwaSoundwaveHit* soundwave = nullptr;
        for (Bullet* bullet : bullet_list)
            if (auto* wave = dynamic_cast<NaiwaSoundwaveHit*>(bullet)) soundwave = wave;
        require(soundwave && soundwave->max_radius == 500, "Naiwa wave retains configured world radius");
        main_camera.follow(player_1->get_position(), player_2->get_position(), 0, true);
        const Vector2 wave_camera = main_camera.get_position();
        main_camera.on_update(16);
        require(main_camera.get_position().x != wave_camera.x || main_camera.get_position().y != wave_camera.y, "Naiwa ultimate actual hit shakes camera");
        stage.on_draw(main_camera);
        saveimage(_T("outputs/city-street/ultimate-preview.png"), &output);
        stage.on_exit();

        const auto check_hit_shake = [](Bullet& attack, const char* description)
        {
            main_camera.reset(); attack.on_collide(); main_camera.on_update(16);
            require(main_camera.get_position().x != 0 || main_camera.get_position().y != 0, description);
        };
        PunchWindBullet punch; check_hit_shake(punch, "Jotaro special hit shake");
        OraBarrageBullet ora; check_hit_shake(ora, "Jotaro ultimate attack hit shake");
        EmeraldBullet emerald; check_hit_shake(emerald, "Kakyoin special hit shake");
        HierophantBullet cannon; check_hit_shake(cannon, "Kakyoin ultimate hit shake");
        require(character_stat_int("kakyoin", "special_mp_gain", -1) == 2, "emerald hit returns 2 energy");
        SetWorkingImage(nullptr); closegraph();
        std::cout << "CITY VERIFICATION COMPLETE\n";
        return 0;
    }
    catch (const std::exception& error) { std::cerr << "FAIL " << error.what() << '\n'; return 1; }
}
