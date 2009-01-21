
#ifndef _CUBEAT_MODEL_SIMPLEMAP_
#define _CUBEAT_MODEL_SIMPLEMAP_

/* 2009.1 by arch.jslin
   This class is intended for AI calculations, and try to
   maxmize code-reusing between this class and the model::Map class
*/

#include "data/MapSetting.hpp"
#include "utils/vector_2d.hpp"

#include <tr1/functional>
#include <tr1/memory>
#include <set>
#include <list>
#include <vector>

namespace psc {

namespace data {
typedef std::tr1::shared_ptr<MapSetting> pMapSetting;
typedef std::tr1::weak_ptr<MapSetting>  wpMapSetting;
}

namespace model {

class SimpleCube;
typedef std::tr1::shared_ptr<SimpleCube> pSimpleCube;
typedef std::tr1::weak_ptr<SimpleCube>  wpSimpleCube;

class Chain;
typedef std::tr1::shared_ptr<Chain> pChain;
typedef std::tr1::weak_ptr<Chain>  wpChain;

class SimpleMap;

template<class T>
class OneFadingT;
typedef std::tr1::shared_ptr< OneFadingT<SimpleMap> > pOneFadingSimple;

class SimpleMap : public std::tr1::enable_shared_from_this<SimpleMap>
{
public:
    typedef std::tr1::shared_ptr<SimpleMap> pointer_type;
    typedef std::tr1::weak_ptr<SimpleMap>  wpointer_type;
    typedef pSimpleCube                        cube_type;
    typedef utils::vector_2d<pSimpleCube> container_type;
    typedef utils::vector_2d<int>         color_map_type;
    typedef std::set<pSimpleCube>         cube_list_type;
    typedef std::list<pChain>            chain_list_type;
    typedef std::vector<pSimpleCube>   preview_list_type;

    static pointer_type create(data::pMapSetting setting) {
        // map doesn't need a pool
        return pointer_type(new SimpleMap(setting))->init();
    }

    SimpleMap(data::pMapSetting setting):
        map_setting_(setting),
        garbage_left_(0), score_(0), current_sum_of_attack_(0), simp_cubes_(0, 0)
    {}
    ~SimpleMap(){}

    SimpleMap& init_cubes(color_map_type const&);
    SimpleMap& cycle() {
        update_chains();
        cycle_cubes();
        cleanup_map_and_drop_all();
        return *this;
    }

    // these are for AI calculations, chain related.
    void attach_chain_bottom_up_from(int, int, pChain);
    void push_chain(pChain chain){ chains_.push_back(chain); }
    // end

    int score() const                 { return score_; }
    int garbage_left() const          { return garbage_left_; }
    int current_sum_of_attack() const { return current_sum_of_attack_; }
    data::pMapSetting const& ms() const{ return map_setting_; }

protected:
    data::pMapSetting map_setting_;
    int garbage_left_, score_, current_sum_of_attack_;
    chain_list_type chains_;// someday make it to be std::list
    std::tr1::shared_ptr<int> warning_level_; //warning timer dummy

private:
    pointer_type init() {
        simp_cubes_.resize(map_setting_->width(), map_setting_->height());
        return shared_from_this();
    }
    void update_chains();
    void cycle_cubes();
    void cycle_a_cube(pSimpleCube, int x, int y);
    void cleanup_map_and_drop_all();
    pOneFadingSimple make_OneFading(pSimpleCube);

//  possibly needed by AI
//    pCube lookup(int x, int y){ return cubes_[x][y]; }
//    Map& setup(int x, int y, pCube cube){ cubes_[x][y] = cube; return *this; }
//    pointer_type clone() const; // NOTE: this is not a real clone...
//    static pCube clone_cube(pCube cube);

    container_type    simp_cubes_;
    preview_list_type simp_cubes_preview_;
};

typedef SimpleMap::pointer_type  pSimpleMap;
typedef SimpleMap::wpointer_type wpSimpleMap;

} //model
} //psc

#endif