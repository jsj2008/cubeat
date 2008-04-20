
#include "presenter/game/Multi.hpp"
#include "view/Scene.hpp"
#include "view/AnimatedSprite.hpp"
#include "view/SpriteText.hpp"
#include "view/Menu.hpp"
#include "Accessors.hpp"
#include "EasingEquations.hpp"

#include "presenter/Stage.hpp"
#include "presenter/PlayerView.hpp"
#include "presenter/Map.hpp"
#include "presenter/cube/ViewSprite.hpp"

#include "EventDispatcher.hpp"
#include "Input.hpp"
#include "Player.hpp"
#include "Weapon.hpp"
#include "Sound.hpp"
#include "Conf.hpp"
#include "App.hpp"

#include "utils/Random.hpp"
#include "utils/dictionary.hpp"
#include "utils/MapLoader.hpp"
#include "utils/to_s.hpp"
#include <boost/foreach.hpp>

using namespace psc;
using namespace presenter;
using namespace game;
using namespace easing;
using namespace accessor;
using utils::to_s;
using std::tr1::bind;
using namespace std::tr1::placeholders;

Multi::Multi()
{
}

Multi::~Multi()
{
}

pMulti Multi::init(std::string const& c1p, std::string const& c2p, std::string const& sc)
{
    //App::i().setLoading(1);
    scene_ = psc::view::Scene::create("Multiplayer game");
    scene_->setTo2DView().enableGlobalHittingEvent();     //important

    c1p_ = c1p; c2p_ = c2p; sconf_ = sc;

    data::pViewSpriteSetting s0, s1;

    s0 = data::ViewSpriteSetting::create(159, 684, 64);   //must use config
    s0->push_ally(0).push_enemy(1);
    s1 = data::ViewSpriteSetting::create(740, 684, 64);   //must use config
    s1->push_ally(1).push_enemy(0);

    ///THIS IS IMPORTANT, ALL PLAYERS MUST BE DEFINED FIRST.
    player0_ = ctrl::Player::create(ctrl::Input::getInputByIndex(0), s0->ally_input_ids(), s0->enemy_input_ids());
    player1_ = ctrl::Player::create(ctrl::Input::getInputByIndex(1), s1->ally_input_ids(), s1->enemy_input_ids());

    // setup map0
    data::pMapSetting set0 = data::MapSetting::create();
    map0_ = presenter::Map::create(set0);
    map0_->set_view_master( presenter::cube::ViewSpriteMaster::create(scene_, s0, player0_) );

    // setup map1
    data::pMapSetting set1 = data::MapSetting::create();
    map1_ = presenter::Map::create(set1);
    map1_->set_view_master( presenter::cube::ViewSpriteMaster::create(scene_, s1, player1_) );

    // setup garbage land
    map0_->push_garbage_land(map1_);
    map1_->push_garbage_land(map0_);
    map0_->set_endgame(bind(&Multi::end, this, _1));
    map1_->set_endgame(bind(&Multi::end, this, _1));

    // setup stage & ui & player's view objects:
    stage_ = presenter::Stage::create( sc.size() ? sc : "config/stage/jungle.zzml" );
    setup_ui_by_config( "config/ui/in_game_2p_layout.zzml" );

    vec2 center_pos( uiconf_.I("character_center_x"), uiconf_.I("character_center_y") );
    pview1_ = presenter::PlayerView::create( c1p.size() ? c1p : "config/char/char1.zzml", scene_, center_pos );
    pview2_ = presenter::PlayerView::create( c2p.size() ? c2p : "config/char/char2.zzml", scene_, center_pos );
    pview2_->flipPosition();
    pview1_->setMap( map0_ );
    pview2_->setMap( map1_ );

    min_ = 0, sec_ = 0 ,last_garbage_1p_ = 0, last_garbage_2p_ = 0;

    //start music
    stage_->playBGM();

    //note: bad area
    timer_item_ = pDummy(new int);
    timer_ui_   = pDummy(new int);
    //note: end of bad area

    ctrl::EventDispatcher::i().subscribe_timer(
        bind(&Multi::update_ui_by_second, this), timer_ui_, 1000, -1);
    ctrl::EventDispatcher::i().subscribe_timer(
        bind(&App::setLoading, &App::i(), 100), 100); //stupid and must?
    ctrl::EventDispatcher::i().subscribe_timer(
        bind(&Multi::item_creation, this), timer_item_, 15000);
    return shared_from_this();
}

void Multi::setup_ui_by_config( std::string const& path )
{
    uiconf_ = utils::map_any::construct( utils::fetchConfig( path ) );
    utils::map_any const& base = uiconf_.M("base");
    ui_layout_ = view::Menu::create( base.S("layout_tex"), scene_, base.I("w"), base.I("h") );
    ui_layout_->set<Alpha>(192);

    utils::map_any const& misc = uiconf_.M("misc");
    BOOST_FOREACH(utils::pair_any const& it, misc) {
        std::string    const& key  = boost::any_cast<std::string const>(it.first);
        utils::map_any const& attr = boost::any_cast<utils::map_any const>(it.second);
        ui_layout_->
            addSpriteText(key, attr.S("text"), attr.S("font"), 0, attr.I("fsize"), attr.I("center") )
           .getSpriteText(key).set<Pos2D>( vec2(attr.I("x"), attr.I("y")) );
    }
}

void Multi::update_ui(){
    int new_garbage_1p_ = map0_->garbage_left() + map1_->current_sum_of_attack();
    int new_garbage_2p_ = map1_->garbage_left() + map0_->current_sum_of_attack();
    ui_layout_->getSpriteText("gar1p").showNumber(new_garbage_1p_, 0);
    ui_layout_->getSpriteText("gar2p").showNumber(new_garbage_2p_, 0);
    ui_layout_->getSpriteText("scr1p").showNumber(map0_->score(), 5);
    ui_layout_->getSpriteText("scr2p").showNumber(map1_->score(), 5);
    ui_layout_->getSpriteText("wep1p1").showNumber(player0_->weapon(0)->ammo(), 2);
    ui_layout_->getSpriteText("wep1p2").showNumber(player0_->weapon(1)->ammo(), 2);
    ui_layout_->getSpriteText("wep1p3").showNumber(player0_->weapon(2)->ammo(), 2);
    ui_layout_->getSpriteText("wep2p1").showNumber(player1_->weapon(0)->ammo(), 2);
    ui_layout_->getSpriteText("wep2p2").showNumber(player1_->weapon(1)->ammo(), 2);
    ui_layout_->getSpriteText("wep2p3").showNumber(player1_->weapon(2)->ammo(), 2);

    for( int i = 0; i <= 2; ++i ) { //note: not flexible, only for test.
        if( i == player0_->wepid() )
            ui_layout_->getSpriteText("wep1p"+to_s(i+1)).set<Scale>(vec3(2,2,2));
        else ui_layout_->getSpriteText("wep1p"+to_s(i+1)).set<Scale>(vec3(1,1,1));

        if( i == player1_->wepid() )
            ui_layout_->getSpriteText("wep2p"+to_s(i+1)).set<Scale>(vec3(2,2,2));
        else ui_layout_->getSpriteText("wep2p"+to_s(i+1)).set<Scale>(vec3(1,1,1));
    }

    if( pview1_->getState() == presenter::PlayerView::HIT &&
        last_garbage_1p_ > new_garbage_1p_ ) stage_->hitGroup(1);
    if( pview2_->getState() == presenter::PlayerView::HIT &&
        last_garbage_2p_ > new_garbage_2p_ ) stage_->hitGroup(2);

    last_garbage_1p_ = new_garbage_1p_;
    last_garbage_2p_ = new_garbage_2p_;
}

void Multi::update_ui_by_second(){
    ++sec_;
    if( sec_ > 59 ) ++min_, sec_ = 0;
    std::string sec = to_s(sec_); if( sec.size() < 2 ) sec = "0" + sec;
    std::string min = to_s(min_); if( min.size() < 2 ) min = "0" + min;
    ui_layout_->getSpriteText("time").changeText( min + ":" + sec );
}

//note: temp code
void Multi::end(pMap lose_map)
{
    timer_item_.reset();
    timer_ui_.reset();
    if( item_ ) item_->setPickable(false);
    ctrl::EventDispatcher::i().clear_btn_event();
    ctrl::EventDispatcher::i().clear_obj_event( scene_ );
    Sound::i().stopAll();
    map0_->take_out_warning().stop_dropping();
    map1_->take_out_warning().stop_dropping();

    Sound::i().play("3/3c/win.mp3");
    blocker_ = view::Sprite::create("blocker", scene_, Conf::i().SCREEN_W, 350, true);
    blocker_->set<Pos2D>( vec2(Conf::i().SCREEN_W/2, Conf::i().SCREEN_H/2) );
    blocker_->setDepth(-100).set<GradientDiffuse>(0).tween<Linear, Alpha>(0, 100, 500u);

    win_t_  = view::Sprite::create("win", scene_, 384, 192, true);
    lose_t_ = view::Sprite::create("lose", scene_, 384, 192, true);

    vec2 pos1 = vec2(Conf::i().SCREEN_W/4,   Conf::i().SCREEN_H/2);
    vec2 pos2 = vec2(Conf::i().SCREEN_W/4*3, Conf::i().SCREEN_H/2);
    if( lose_map == map0_ ) {
        lose_t_->set<Pos2D>( pos1 );
        win_t_->set<Pos2D>( pos2 );
    }
    else {
        lose_t_->set<Pos2D>( pos2 );
        win_t_->set<Pos2D>( pos1 );
    }
    vec3 v0(0,0,0), v1(1,1,1);
    win_t_->setDepth(-450).tween<OElastic, Scale>(v0, v1, 1000u, 0);
    lose_t_->setDepth(-450).tween<OElastic, Scale>(v0, v1, 1000u, 0);

    end_text_ = view::SpriteText::create("play again?", scene_, "Star Jedi", 30, true);
    end_text2_= view::SpriteText::create("a:yes / b:no", scene_, "Star Jedi", 30, true);
    end_text_->set<Pos2D> ( vec2(Conf::i().SCREEN_W/2, Conf::i().SCREEN_H/2 + 50) );
    end_text2_->set<Pos2D>( vec2(Conf::i().SCREEN_W/2, Conf::i().SCREEN_H/2 + 100) );
    end_text_-> setDepth(-450).set<Alpha>(0).tween<Linear, Alpha>(0, 255, 500u, 0, 0, 1000);
    end_text2_->setDepth(-450).set<Alpha>(0).tween<Linear, Alpha>(0, 255, 500u, 0, 0, 1000);

    ctrl::EventDispatcher::i().subscribe_timer(bind(&Multi::setup_end_button, this), 1000);
}

void Multi::setup_end_button()
{
    std::tr1::function<void(int, int)> clicka = bind(&Multi::reinit, this);
    std::tr1::function<void(int, int)> clickb = bind(&Multi::end_sequence1, this);
    btn_reinit_ = pDummy(new int);
    BOOST_FOREACH(ctrl::Input const* input, ctrl::Input::getInputs()) {
        ctrl::EventDispatcher::i().subscribe_btn_event(
            clicka, btn_reinit_, &input->trig1(), ctrl::BTN_PRESS);
        ctrl::EventDispatcher::i().subscribe_btn_event(
            clickb, btn_reinit_, &input->trig2(), ctrl::BTN_PRESS);
    }
}

void Multi::end_sequence1()
{
    Sound::i().play("4/4c.wav");
    btn_reinit_.reset();
    App::i().launchMainMenu();
    std::cout << "game_multiplayer end call finished.\n";
}

void Multi::reinit()
{
    Sound::i().play("4/4b.wav");
    btn_reinit_.reset();
    ctrl::EventDispatcher::i().subscribe_timer(
        bind(&App::launchMultiplayer, &App::i(), c1p_, c2p_, sconf_), 500);
    std::cout << "game_multiplayer end call finished.\n";
}

//note: not very elegant.
void Multi::item_creation()
{
    using namespace std::tr1::placeholders;
    Sound::i().play("3/3f/item.mp3");
    item_ = view::AnimatedSprite::create("itembox", scene_, 64, 64, true);
    item_->playAnime("moving", 500, -1).setDepth(-60);

    std::tr1::function<void(int)> const cb1 = bind(&Multi::eat_item, this, player0_, _1);
    std::tr1::function<void(int)> const cb2 = bind(&Multi::eat_item, this, player1_, _1);
    view::pSprite body_ = item_;
    player0_->subscribe_shot_event(body_, cb1);
    player1_->subscribe_shot_event(body_, cb2);

    int y = utils::random(192) + 32;
    std::tr1::function<void()> endcall = bind(&Multi::item_destruction, this);
    item_->tween<OElastic, Scale>(vec3(0,0,0), vec3(1,1,1), 1000u);
    if( utils::random(2) )
        item_->tween<Linear, Pos2D>(vec2(32, y), vec2(Conf::i().SCREEN_W+64, y), 4000u, 0, endcall);
    else
        item_->tween<Linear, Pos2D>(vec2(Conf::i().SCREEN_W-32, y), vec2(-64, y), 4000u, 0, endcall);
}

void Multi::eat_item(ctrl::pPlayer p, int)
{
    item_->setPickable(false);
    item_->tween<Linear, Alpha>(0, 400u);
    item_->tween<OQuad, Scale>(vec3(1.3,1.3,1.3), 400u);
    p->eat_item();
}

void Multi::item_destruction()
{
    timer_item_ = pDummy(new int);
    ctrl::EventDispatcher::i().subscribe_timer(
        bind(&Multi::item_creation, this), timer_item_, 15000);
}

void Multi::cycle()
{
    pview1_->cycle();
    pview2_->cycle();
    update_ui();
    stage_->cycle();
    scene_->redraw();
    map0_->redraw().cycle();
    map1_->redraw().cycle();
}

