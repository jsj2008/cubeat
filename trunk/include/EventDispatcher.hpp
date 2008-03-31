#ifndef _SHOOTING_CUBES_EVENT_DISPATCHER_
#define _SHOOTING_CUBES_EVENT_DISPATCHER_

/* EventDispatcher:
   A singleton to dispatch user-defined events like on_press,
   on_release, on_blahblah.

   TODO:
   * currently none?
*/

#include "ButtonEnum.hpp"
#include "all_fwd.hpp"

#include <tr1/memory>
#include <tr1/tuple>
#include <tr1/functional>
#include <map>
#include <ctime>
#include <vector>
#include <list>

namespace irr {
namespace scene {
class ISceneNode;
}
}

namespace psc {

namespace ctrl {

class Input;
class Button;

class EventDispatcher
{
    typedef std::tr1::shared_ptr<void>                                     pvoid;
    typedef std::tr1::weak_ptr<void>                                       wpvoid;

    typedef std::tr1::function<void(int x, int y)>                         BtnCallback;
    typedef std::tr1::tuple<BtnCallback, Button const*, BSTATE, wpvoid>    BtnEvent;
    typedef std::list<BtnEvent>                                            BtnListener;
    typedef std::list< BtnListener::iterator >                             BtnEventRemoval;

    typedef std::tr1::function<void()>                                     TimerCallback;
    typedef std::tr1::tuple<TimerCallback, std::time_t, std::time_t, int, wpvoid> Timer;
    typedef std::list<Timer>                                               TimerList;
    typedef std::list< TimerList::iterator >                               TimerRemoval;

    typedef std::tr1::function<void( view::pSprite& )>                     ObjCallback;
    typedef std::tr1::tuple<ObjCallback const*, Button const*, BSTATE, view::wpSprite> ObjEvent;
    typedef std::list<ObjEvent>                                            ObjListener;
    typedef std::pair<view::wpScene const, ObjListener::iterator>          ObjEventRemovalPair;
    typedef std::map <view::wpScene, ObjListener::iterator>                ObjEventRemoval;
    typedef std::pair<view::wpScene const, ObjListener>                    SceneListenerPair;
    typedef std::map <view::wpScene, ObjListener>                          SceneListener;
    typedef std::list<SceneListener::key_type>                             SceneListenerRemoval;

    typedef std::map <Input const*, irr::scene::ISceneNode*>               PickingMap;

public:
    static EventDispatcher& i() {
        static EventDispatcher singleton;
        return singleton;
    }

    /// Object binding version
    EventDispatcher&
    subscribe_btn_event(BtnCallback const&, wpvoid const&, Button const*, BSTATE const&);
    EventDispatcher&
    subscribe_timer(TimerCallback const&, wpvoid const&, int const&, int loop = 0);
    EventDispatcher&
    subscribe_obj_event(ObjCallback const*, view::pSprite const&, Button const*, BSTATE const&);

    /// Free function version
    EventDispatcher&
    subscribe_btn_event(BtnCallback const&, Button const*, BSTATE const&);
    EventDispatcher&
    subscribe_timer(TimerCallback const&, int const&, int loop = 0);

    void dispatch();

private:
    enum { BCALLBACK = 0, BUTTON = 1, STATE = 2, BCALLEE = 3 };
    enum { TCALLBACK = 0, DURATION = 1, LASTTIME = 2, LOOP = 3, TCALLEE = 4 };
    enum { TIMER_TIME, TIMER };
    struct OE{enum{OBJ_CB, BTN, STATE, SPRITE};};
    EventDispatcher();
    void dispatch_btn();
    void dispatch_obj();
    void dispatch_timer();
    void cleanup_timer_and_init_newly_created_timer();
    void cleanup_obj_event();
    void cleanup_btn_event();
    void obj_picking(view::pScene const&);
    void obj_listening(view::pScene const&, ObjListener&);

private:
    BtnListener          btn_listeners_;
    BtnEventRemoval      btn_events_to_be_deleted_;
    TimerList            timers_, newly_created_timers_;
    TimerRemoval         timers_to_be_deleted_;
    SceneListener        scene_listeners_;
    ObjEventRemoval      obj_events_to_be_deleted_;
    SceneListenerRemoval scene_expired_;
    PickingMap           pickmap_;

    static pvoid self_; //point to &EventDispatcher::i()
};

}  //ctrl
}  //psc

#endif
