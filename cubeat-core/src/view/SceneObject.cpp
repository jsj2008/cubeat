
#include "view/SceneObject.hpp"
#include "view/Scene.hpp"
#include "IrrDevice.hpp"

#include <sstream>

using namespace irr;
using namespace core;
using namespace scene;
using namespace video;

using namespace psc;
using namespace view;
using std::tr1::static_pointer_cast;

SceneObject* SceneObject::clone() const
{
    SceneObject* obj = new SceneObject(*this);
    return obj;
}

pSceneObject SceneObject::init(pObject const& parent)
{
    setupSceneAndManager(parent);

    std::ostringstream oss;
    oss << "rc/model/" << name_ << ".x";
    IMesh* mesh = smgr_->getMesh( oss.str().c_str() )->getMesh(0);
    body_ = smgr_->addMeshSceneNode( mesh, parent->body() );

    pSceneObject self = static_pointer_cast<SceneObject>( shared_from_this() );
    scene()->addPickMapping( body_, self );
    return self;
}