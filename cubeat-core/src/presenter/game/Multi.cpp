
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
#include "ctrl/AIPlayer.hpp"
#include "Weapon.hpp"
#include "audio/Sound.hpp"
#include "Conf.hpp"
#include "App.hpp"

#include "utils/Random.hpp"
#include "utils/dictionary.hpp"
#include "utils/MapLoader.hpp"
#include "utils/to_s.hpp"
#include "utils/Logger.hpp"
#include <boost/foreach.hpp>
#include <algorithm>

using namespace psc;
using namespace presenter;
using namespace game;
using namespace easing;
using namespace accessor;
using utils::to_s;
using namespace std::tr1::placeholders;

Multi::Multi()
{
}

Multi::~Multi()
{
    std::cout << "Multiplayer Game destructing ..." << std::endl;
    std::cout << " player0 use count: " << player0_.use_count() << std::endl;
    std::cout << " player1 use count: " << player1_.use_count() << std::endl;
}

pMulti Multi::init(std::string const& c1p, std::string const& c2p,
                   std::string const& sc, int num_of_cpu, int ai_level)
{
    //App::i().setLoading(1);
    scene_ = psc::view::Scene::create("game");
    //scene_->setTo2DView().enableGlobalHittingEvent(); //2011.03.28 weapon temporary removal
    scene_->setTo2DView();

    c1p_ = c1p; c2p_ = c2p; sconf_ = sc; num_of_cpu_ = num_of_cpu; ai_level_ = ai_level;

    gameplay_ = Conf::i().config_of("gameplay/multi");

    data::pViewSetting s0, s1;

    s0 = data::ViewSetting::create(64);   //must use config
    s0->x_offset(159).y_offset(684);
    s1 = data::ViewSetting::create(64);   //must use config
    s1->x_offset(740).y_offset(684);

    std::string ai_temp[4] =
        {"ai/easy.lua", "ai/normal.lua", "ai/hard.lua", "ai/insane.lua"};

    ///THIS IS IMPORTANT, ALL PLAYERS MUST BE DEFINED FIRST.
    ctrl::Input* input0 = ctrl::InputMgr::i().getInputByIndex(0);
    ctrl::Input* input1 = ctrl::InputMgr::i().getInputByIndex(1);
    if( num_of_cpu == 0 ) {
        player0_ = ctrl::Player::create(input0, 0);
        player1_ = ctrl::Player::create(input1, 1);
    }
    else if( num_of_cpu == 1 ) {
        input1->setControlledByAI(true);
        player0_ = ctrl::Player::create(input0, 0);
        player1_ = ctrl::AIPlayer::create(input1, 1, ai_temp[ai_level_]);
    }
    else {
        input0->setControlledByAI(true);
        input1->setControlledByAI(true);
        std::random_shuffle(ai_temp, ai_temp + 4);
        player0_ = ctrl::AIPlayer::create(input0, 0, ai_temp[0]);
        player1_ = ctrl::AIPlayer::create(input1, 1, ai_temp[1]);
    }
    player0_->push_ally(0).push_enemy(1);
    player1_->push_ally(1).push_enemy(0);

    // setup player settings
    player0_->set_config(gameplay_.M("player1").M("weapon"));
    player1_->set_config(gameplay_.M("player2").M("weapon"));

    // setup map0
    data::pMapSetting set0 = data::MapSetting::create( gameplay_.M("player1") );
    map0_ = presenter::Map::create(set0);
    //map0_ = utils::MapLoader::load(0); //temp: this is for exciting demo.
    map0_->set_view_master( presenter::cube::ViewSpriteMaster::create(scene_, s0, player0_) );

    // setup map1
    data::pMapSetting set1 = data::MapSetting::create( gameplay_.M("player2") );
    map1_ = presenter::Map::create(set1);
    //map1_ = utils::MapLoader::load(1); //temp: this is for exciting demo.
    map1_->set_view_master( presenter::cube::ViewSpriteMaster::create(scene_, s1, player1_) );

    // setup garbage land
    map0_->push_garbage_land(map1_);
    map1_->push_garbage_land(map0_);
    map0_->lose_event(bind(&Multi::end, this, ref(map0_)));
    map1_->lose_event(bind(&Multi::end, this, ref(map1_)));

    ///NEW: MAKE PLAYER KNOWS ABOUT MAP
    std::vector< presenter::wpMap > map_list;
    map_list.push_back(map0_);
    map_list.push_back(map1_);
    player0_->setMapList( map_list );
    player1_->setMapList( map_list );

    // setup stage & ui & player's view objects:
    stage_ = presenter::Stage::create( sc.size() ? sc : "stage/jungle" );
    setup_ui_by_config( c1p, c2p, "ui/in_game_2p_layout" );

    min_ = 0, sec_ = 0 ,last_garbage_1p_ = 0, last_garbage_2p_ = 0;

    //start music
    stage_->playBGM();

    //note: bad area
    //timer_item_ = pDummy(new int);                  //2011.03.25 item temporarily removed
    timer_ui_   = pDummy(new int);
    //note: end of bad area
    using std::tr1::bind;

    ctrl::EventDispatcher::i().get_timer_dispatcher("game")->subscribe(
        bind(&Multi::update_ui_by_second, this), timer_ui_, 1000, -1);
    ctrl::EventDispatcher::i().get_timer_dispatcher("game")->subscribe(
        bind(&App::setLoading, &App::i(), 100), 100); //stupid and must?
    //ctrl::EventDispatcher::i().get_timer_dispatcher("game")->subscribe(     //2011.03.25 item temporarily removed
    //    bind(&Multi::item_creation, this), timer_item_, 15000);

    BOOST_FOREACH(ctrl::Input const* input, ctrl::InputMgr::i().getInputs()) {
        ctrl::EventDispatcher::i().subscribe_btn_event(
            bind(&Multi::pause, this, input), shared_from_this(), &input->pause(), ctrl::BTN_PRESS);
    }

    if( num_of_cpu_ > 0 )
        player1_->startThinking();
    if( num_of_cpu_ > 1 )
        player0_->startThinking();

    blocker_ = view::Sprite::create("blocker", scene_, Conf::i().SCREEN_W() ,350, true);
    blocker_->set<Pos2D>( vec2(Conf::i().SCREEN_W() /2, Conf::i().SCREEN_H() /2) );
    blocker_->setDepth(-50).set<Alpha>(100).set<GradientDiffuse>(0).setPickable(false).set<Visible>(false);

    if( num_of_cpu_ == 2 ) {
        blocker_->set<Visible>(true);
        pause_note_text_ = view::SpriteText::create("press middle button to\npause and leave", scene_, "Star Jedi", 30, true);
        pause_note_text_->set<Pos2D>( vec2(Conf::i().SCREEN_W() /2, Conf::i().SCREEN_H() /2) );
        pause_note_text_->tween<SineCirc, Alpha>(0, 3000u, -1);
        pause_note_text_->setDepth(-100).setPickable(false);
    }

    return shared_from_this();
}

void Multi::setup_ui_by_config( std::string const& c1p, std::string const& c2p, std::string const& path )
{
    uiconf_ = Conf::i().config_of(path);
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

    vec2 center_pos( uiconf_.I("character_center_x"), uiconf_.I("character_center_y") );
    pview1_ = presenter::PlayerView::create( c1p.size() ? c1p : "char/char1.zzml", scene_, center_pos );
    pview2_ = presenter::PlayerView::create( c2p.size() ? c2p : "char/char2.zzml", scene_, center_pos );
    pview2_->flipPosition();
    pview1_->setMap( map0_ );
    pview2_->setMap( map1_ );
    pview1_->setInput( ctrl::InputMgr::i().getInputByIndex(0) ); //temp: for pview to know input for rumbling wiimote
    pview2_->setInput( ctrl::InputMgr::i().getInputByIndex(1) ); //temp: for pview to know input for rumbling wiimote

    utils::map_any const& gauge_conf = uiconf_.M("heatgauge");
    vec2 gauge1_pos( gauge_conf.I("x_1p"), gauge_conf.I("y") );
    vec2 gauge2_pos( gauge_conf.I("x_2p"), gauge_conf.I("y") );
    heatgauge1_ = view::Sprite::create("heatgauge1", scene_, gauge_conf.I("w"), gauge_conf.I("h"), false);
    heatgauge2_ = view::Sprite::create("heatgauge2", scene_, gauge_conf.I("w"), gauge_conf.I("h"), false);
    heatgauge1_->set<Pos2D>( gauge1_pos ).set<ColorDiffuseVec3>( vec3(0,255,0) ).set<Alpha>(128)
                .set<Rotation>(vec3(0, 0, gauge_conf.I("rotation")));
    heatgauge2_->set<Pos2D>( gauge2_pos ).set<ColorDiffuseVec3>( vec3(0,255,0) ).set<Alpha>(128)
                .set<Rotation>(vec3(0, 0, gauge_conf.I("rotation")));

    ui_layout_->setPickable(false);

    gauge1_flag_ = gauge2_flag_ = false;
}

void Multi::update_heatgauge(ctrl::pPlayer player, view::pSprite gauge, bool& out_flag)
{
    gauge->set<Scale>( vec3(player->heat(), 1, 1) );

    if( !player->is_overheat() ) {
        out_flag = false;
        if( player->heat() < 0.5 ) {
            gauge->set<Green>(255);
            gauge->set<Red>( player->heat()*2*255 );
        }
        else {
            gauge->set<Green>( 255 - (player->heat()-0.5)*2*255 );
            gauge->set<Red>(255);
        }
    }
    else if( !out_flag ) {
        out_flag = true;
        gauge->tween<SineCirc, ColorDiffuseVec3>(vec3(255,255,255), player->overheat_downtime()/4, 3);
    }
}

void Multi::update_ui(){
    int new_garbage_1p_ = map0_->garbage_left() + map1_->current_sum_of_attack();
    int new_garbage_2p_ = map1_->garbage_left() + map0_->current_sum_of_attack();
    ui_layout_->getSpriteText("gar1p").showNumber(new_garbage_1p_, 0);
    ui_layout_->getSpriteText("gar2p").showNumber(new_garbage_2p_, 0);
    ui_layout_->getSpriteText("scr1p").showNumber(map0_->score(), 5);
    ui_layout_->getSpriteText("scr2p").showNumber(map1_->score(), 5);

    //2011.03.25 weapon temporarily removed

    //ui_layout_->getSpriteText("wep1p1").showNumber(player0_->weapon(0)->ammo(), 2);
    //ui_layout_->getSpriteText("wep1p2").showNumber(player0_->weapon(1)->ammo(), 2);
    //ui_layout_->getSpriteText("wep1p3").showNumber(player0_->weapon(2)->ammo(), 2);
    //ui_layout_->getSpriteText("wep2p1").showNumber(player1_->weapon(0)->ammo(), 2);
    //ui_layout_->getSpriteText("wep2p2").showNumber(player1_->weapon(1)->ammo(), 2);
    //ui_layout_->getSpriteText("wep2p3").showNumber(player1_->weapon(2)->ammo(), 2);

    //for( int i = 0; i <= 2; ++i ) { //note: not flexible, only for test.
    //    if( i == player0_->wepid() )
    //        ui_layout_->getSpriteText("wep1p"+to_s(i+1)).set<Scale>(vec3(2,2,2));
    //    else ui_layout_->getSpriteText("wep1p"+to_s(i+1)).set<Scale>(vec3(1,1,1));
    //
    //    if( i == player1_->wepid() )
    //        ui_layout_->getSpriteText("wep2p"+to_s(i+1)).set<Scale>(vec3(2,2,2));
    //    else ui_layout_->getSpriteText("wep2p"+to_s(i+1)).set<Scale>(vec3(1,1,1));
    //}

    if( pview1_->getState() == presenter::PlayerView::HIT &&
        last_garbage_1p_ > new_garbage_1p_ ) stage_->hitGroup(1);
    if( pview2_->getState() == presenter::PlayerView::HIT &&
        last_garbage_2p_ > new_garbage_2p_ ) stage_->hitGroup(2);

    update_heatgauge(player0_, heatgauge1_, gauge1_flag_);
    update_heatgauge(player1_, heatgauge2_, gauge2_flag_);

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

void Multi::cleanup()
{
    timer_item_.reset();
    timer_ui_.reset();
    btn_pause_.reset();
    if( item_ ) item_.reset();
    ctrl::EventDispatcher::i().clear_btn_event();
    ctrl::EventDispatcher::i().clear_obj_event( scene_ );
    audio::Sound::i().stopAll();
    map0_->stop_dropping();
    map1_->stop_dropping();

    ctrl::InputMgr::i().getInputByIndex(0)->setControlledByAI(false);
    ctrl::InputMgr::i().getInputByIndex(1)->setControlledByAI(false);
    player0_->stopAllActions();
    player1_->stopAllActions();
}

//note: temp code
void Multi::end(pMap lose_map)
{
    cleanup();

    if( pause_note_text_) pause_note_text_->set<Visible>(false);
    blocker_->tween<Linear, Alpha>(0, 100, 500u).set<Visible>(true);

    win_t_  = view::Sprite::create("win", scene_, 384, 192, true);
    lose_t_ = view::Sprite::create("lose", scene_, 384, 192, true);

    vec2 pos1 = vec2(Conf::i().SCREEN_W() /4,   Conf::i().SCREEN_H() /2);
    vec2 pos2 = vec2(Conf::i().SCREEN_W() /4*3, Conf::i().SCREEN_H() /2);
    if( lose_map == map0_ ) {
        lose_t_->set<Pos2D>( pos1 );
        win_t_->set<Pos2D>( pos2 );
        if( num_of_cpu_ == 1 )
            audio::Sound::i().playBuffer("3/3c/lose.wav");
        else
            audio::Sound::i().playBuffer("3/3c/win.wav");
    }
    else {
        lose_t_->set<Pos2D>( pos2 );
        win_t_->set<Pos2D>( pos1 );
        audio::Sound::i().playBuffer("3/3c/win.wav");
    }
    vec3 v0(0,0,0), v1(1,1,1);
    win_t_->setDepth(-450).tween<OElastic, Scale>(v0, v1, 1000u, 0);
    lose_t_->setDepth(-450).tween<OElastic, Scale>(v0, v1, 1000u, 0);

    end_text_ = view::SpriteText::create("play again?", scene_, "Star Jedi", 30, true);
    end_text2_= view::SpriteText::create("\nyes: left click\nleave: right click", scene_, "Star Jedi", 30, true);
    end_text_->set<Pos2D> ( vec2(Conf::i().SCREEN_W() /2, Conf::i().SCREEN_H() /2 + 50) );
    end_text2_->set<Pos2D>( vec2(Conf::i().SCREEN_W() /2, Conf::i().SCREEN_H() /2 + 100) );
    end_text_-> set<Alpha>(0).setDepth(-450).tween<Linear, Alpha>(0, 255, 500u, 0, 0, 1000);
    end_text2_->set<Alpha>(0).setDepth(-450).tween<Linear, Alpha>(0, 255, 500u, 0, 0, 1000);

    ctrl::EventDispatcher::i().get_timer_dispatcher("game")->subscribe(
        bind(&Multi::setup_end_button, this), 1000);
}

void Multi::setup_end_button()
{
    std::tr1::function<void(int, int)> clicka = bind(&Multi::reinit, this);
    std::tr1::function<void(int, int)> clickb = bind(&Multi::end_sequence1, this);
    btn_reinit_ = pDummy(new int);
    BOOST_FOREACH(ctrl::Input const* input, ctrl::InputMgr::i().getInputs()) {
        ctrl::EventDispatcher::i().subscribe_btn_event(
            clicka, btn_reinit_, &input->trig1(), ctrl::BTN_PRESS);
        ctrl::EventDispatcher::i().subscribe_btn_event(
            clickb, btn_reinit_, &input->trig2(), ctrl::BTN_PRESS);
    }
}

void Multi::end_sequence1()
{
    audio::Sound::i().playBuffer("4/4c.wav");
    btn_reinit_.reset();
    stage_->releaseResource(); //release when player isn't going to replay
    App::i().launchMainMenu();
    std::cout << "game_multiplayer end call finished." << std::endl;
}

void Multi::pause_quit()
{
    ctrl::EventDispatcher::i().get_timer_dispatcher("game")->start();
    audio::Sound::i().pauseAll(false);
    btn_pause_.reset(); //reset button event subscribed by this handle.
    ctrl::EventDispatcher::i().get_timer_dispatcher("game")->subscribe(
        bind(&Multi::cleanup, this), shared_from_this(), 1); //1 ms
        //because we call this here.. it's gonna cleanup a lot of things.
        //it's better we delay this call.
    ctrl::EventDispatcher::i().get_timer_dispatcher("game")->subscribe(
        bind(&Multi::end_sequence1, this), shared_from_this(), 100); //100 ms
}

void Multi::reinit()
{
    using std::tr1::bind;
    audio::Sound::i().playBuffer("4/4b.wav");
    btn_reinit_.reset();
    ctrl::EventDispatcher::i().get_timer_dispatcher("global")->subscribe(
        bind(&App::launchMultiplayer, &App::i(), c1p_, c2p_, sconf_, num_of_cpu_, ai_level_), 500);
    std::cout << "game_multiplayer end call finished." << std::endl;
}

//note: not very elegant.
void Multi::item_creation()
{
    audio::Sound::i().playBuffer("3/3f/item.wav");
    item_ = view::AnimatedSprite::create("itembox", scene_, 64, 64, true);
    item_->playAnime("moving", 500, -1).setDepth(-60);

    ctrl::wpPlayer wp0 = player0_;
    ctrl::wpPlayer wp1 = player1_;
    std::tr1::function<void(int)> const cb1 = std::tr1::bind(&Multi::eat_item, this, wp0, std::tr1::placeholders::_1);
    std::tr1::function<void(int)> const cb2 = std::tr1::bind(&Multi::eat_item, this, wp1, std::tr1::placeholders::_1);
    view::pSprite body_ = item_;
    player0_->subscribe_shot_event(body_, cb1);
    player1_->subscribe_shot_event(body_, cb2);

    int y = utils::random(192) + 32;
    std::tr1::function<void()> endcall = bind(&Multi::item_destruction, this);
    item_->tween<OElastic, Scale>(vec3(0,0,0), vec3(1,1,1), 1000u);
    if( utils::random(2) )
        item_->tween<Linear, Pos2D>(vec2(32, y), vec2(Conf::i().SCREEN_W() + 64, y), 4000u, 0, endcall);
    else
        item_->tween<Linear, Pos2D>(vec2(Conf::i().SCREEN_W() - 32, y), vec2(-64, y), 4000u, 0, endcall);
}

void Multi::eat_item(ctrl::wpPlayer wp, int)
{
    if( ctrl::pPlayer p = wp.lock() ) {
        item_->setPickable(false);
        item_->tween<Linear, Alpha>(0, 400u);
        item_->tween<OQuad, Scale>(vec3(1.3,1.3,1.3), 400u);

        utils::map_any iset = gameplay_.M("item");
        int percent = utils::random(100);

        if( percent < iset.I("wep1_chance") ) {
            p->weapon(0)->ammo( p->weapon(0)->ammo()+iset.I("wep1_ammo") );
        }
        else if( percent < iset.I("wep1_chance") + iset.I("wep2_chance") ) {
            p->weapon(0)->ammo( p->weapon(1)->ammo()+iset.I("wep2_ammo") );
        }
        else {
            p->weapon(0)->ammo( p->weapon(2)->ammo()+iset.I("wep3_ammo") );
        }
        audio::Sound::i().playBuffer("1/e/getitem.wav");
    }
}

void Multi::item_destruction()
{
    timer_item_.reset();
    timer_item_ = pDummy(new int);
    ctrl::EventDispatcher::i().get_timer_dispatcher("game")->subscribe(
        bind(&Multi::item_creation, this), timer_item_, 15000);
}

void do_nothing(int, int){};

void Multi::pause(ctrl::Input const* controller)
{
    if( btn_pause_ ) return; //it's already paused, don't do anything if this is called again.

    if( pause_note_text_) pause_note_text_->set<Visible>(false);

    if( !pause_text_ || !pause_text2_ ) {
        pause_text_ = view::SpriteText::create("back to menu?", scene_, "Star Jedi", 30, true);
        pause_text_->set<Pos2D>( vec2(Conf::i().SCREEN_W() /2, Conf::i().SCREEN_H() /2 + 60) );
        pause_text_->setDepth(-450).setPickable(false);
        pause_text2_ = view::SpriteText::create("\nyes: left click\nno: right click", scene_, "Star Jedi", 30, true);
        pause_text2_->set<Pos2D>( vec2(Conf::i().SCREEN_W() /2, Conf::i().SCREEN_H() /2 + 100) );
        pause_text2_->setDepth(-450).setPickable(false);
    }
    pause_text_->set<Visible>(true);
    pause_text2_->set<Visible>(true);

    if( !pause_t_ ) {
        pause_t_ = view::Sprite::create("pause", scene_, 384, 192, true);
        pause_t_->set<Pos2D>( vec2(Conf::i().SCREEN_W()/2, Conf::i().SCREEN_H()/2 - 50) );
        pause_t_->setDepth(-450).setPickable(false);
    }
    pause_t_->set<Visible>(true);

    blocker_->set<Alpha>(100).set<Visible>(true);

    ctrl::EventDispatcher::i().get_timer_dispatcher("game")->stop();
    audio::Sound::i().pauseAll(true);
    scene_->allowPicking(false);
    if( item_ ) item_->setPickable(false);

    std::tr1::function<void(int, int)> clicka = bind(&Multi::pause_quit, this);
    std::tr1::function<void(int, int)> clickb = bind(&Multi::resume, this, controller);

    btn_pause_ = pDummy(new int);

    BOOST_FOREACH(ctrl::Input const* input, ctrl::InputMgr::i().getInputs()) {
        ctrl::EventDispatcher::i().subscribe_btn_event(
            do_nothing, shared_from_this(), &input->trig1(), ctrl::BTN_PRESS); //assign null
        ctrl::EventDispatcher::i().subscribe_btn_event(
            do_nothing, shared_from_this(), &input->trig2(), ctrl::BTN_PRESS); //assign null
        ctrl::EventDispatcher::i().subscribe_btn_event(
            do_nothing, shared_from_this(), &input->pause(), ctrl::BTN_PRESS); //assign null
    }

    ctrl::EventDispatcher::i().subscribe_btn_event(
        clicka, btn_pause_, &controller->trig1(), ctrl::BTN_PRESS);
    ctrl::EventDispatcher::i().subscribe_btn_event(
        clickb, btn_pause_, &controller->trig2(), ctrl::BTN_PRESS);
    ctrl::EventDispatcher::i().subscribe_btn_event(
        clickb, btn_pause_, &controller->pause(), ctrl::BTN_PRESS);
}

void Multi::resume(ctrl::Input const* controller)
{
    if( !btn_pause_ ) return; //if it's not paused at all, don't do anything

    pause_text_->set<Visible>(false);
    pause_text2_->set<Visible>(false);
    pause_t_->set<Visible>(false);
    blocker_->set<Visible>(false);

    ctrl::EventDispatcher::i().get_timer_dispatcher("game")->start();
    audio::Sound::i().pauseAll(false);
    scene_->allowPicking(true);
    if( item_ ) item_->setPickable(true);

    btn_pause_.reset(); //reset button event subscribed by this handle.

    BOOST_FOREACH(ctrl::Input const* input, ctrl::InputMgr::i().getInputs()) {
        ctrl::EventDispatcher::i().subscribe_btn_event(
            bind(&Multi::pause, this, input), shared_from_this(), &input->pause(), ctrl::BTN_PRESS);
        input->player()->subscribe_player_specific_interactions(true);
    }
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
    if( !btn_reinit_ && !btn_pause_ ) { //2011.04.09 quick fix: if these indicator is alive, stop AI's possible inputs
        player0_->cycle();
        player1_->cycle();
    }
}

