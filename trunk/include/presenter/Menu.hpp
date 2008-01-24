
#ifndef _SHOOTING_CUBES_PRESENTER_MENU_
#define _SHOOTING_CUBES_PRESENTER_MENU_

/* Currently not used. */

#include "presenter/Object.hpp"

#include "utils/ObjectPool.hpp"
#include <boost/tr1/memory.hpp>

namespace psc {

namespace presenter {

class Menu : public Object, public std::tr1::enable_shared_from_this<Menu>
{
public:
    typedef std::tr1::shared_ptr< Menu > pointer_type;
    typedef std::tr1::weak_ptr< Menu > wpointer_type;
    static pointer_type create() {
        pointer_type p = psc::ObjectPool< Menu >::create();
        p->init();
        return p;
    }

    virtual void cycle();

protected:
    void init();

protected:

};

typedef Menu::pointer_type pMenu;
typedef Menu::wpointer_type wpMenu;

} //presenter
} //psc


#endif // _SHOOTING_CUBES_PRESENTER_MENU_
