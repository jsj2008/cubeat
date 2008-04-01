
#include "IrrDevice.hpp"
#include "view/Sprite.hpp"
#include "view/Scene.hpp"
#include "EventDispatcher.hpp"
#include "Button.hpp"
#include "Accessors.hpp"
#include "EasingEquations.hpp"

#include <sstream>
#include <boost/foreach.hpp>

using std::tr1::function;

using namespace irr;
using namespace core;
using namespace scene;
using namespace video;

using namespace psc;
using namespace view;
using namespace easing;
using namespace accessor;
using std::tr1::static_pointer_cast;

Sprite::Sprite(std::string const& name, bool const& center)
    :Object(name),
     center_(center), center_aligned_plane_(0),
     upperleft_aligned_plane_(0), thismesh_(0)
{
    upperleft_aligned_plane_ =
        IrrDevice::i().d()->getSceneManager()->getMesh( "rc/model/plane_orig.x" );
    //we don't specify different smgr_ for different scene here,
    //because smgr_ is not yet defined at this stage, however the resource
    //will be shared across different SceneManagers.
}

pSprite Sprite::init(pObject const& parent, int const& w, int const& h)
{
    if( name_.size() < 1 ) {
        Object::init(parent);
        return std::tr1::static_pointer_cast<Sprite>(shared_from_this());
    }
    setupSceneAndManager(parent);

    std::ostringstream oss;
    oss << "rc/texture/" << name_ << ".png";

    video::IVideoDriver* driver = smgr_->getVideoDriver();

    size_.Width  = w;
    size_.Height = h;

    SMaterial mat;
    mat.setFlag(video::EMF_LIGHTING, true);
    mat.setFlag(video::EMF_ZWRITE_ENABLE, false);
    mat.setTexture(0, driver->getTexture(oss.str().c_str()));

    mat.MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL;
    mat.MaterialTypeParam = 0.01f;

    mat.DiffuseColor.set(255, 255, 255, 255);

    setupMeshBase(parent);
    body_->getMaterial(0) = mat;

    pSprite self = static_pointer_cast<Sprite>( shared_from_this() );
    scene()->addPickMapping( body_, self );
    return self;
}

void Sprite::setupMeshBase(pObject const& parent)
{
    IMeshManipulator* mani = smgr_->getMeshManipulator();
    thismesh_ = mani->createMeshCopy( upperleft_aligned_plane_ );
    mani->scaleMesh( thismesh_, vector3df(size_.Width / 100, size_.Height / 100, 1) );

    if( center_ ) {
        matrix4 mat; mat.setTranslation( vector3df(-size_.Width/2, size_.Height/2, 0) );
        mani->transformMesh( thismesh_, mat );
    }

    body_ = smgr_->addMeshSceneNode( thismesh_, parent->body(), -1, vector3df(0,0,0) );
    body_->setAutomaticCulling(EAC_OFF);
    body_->setName( name_.c_str() );
}

void Sprite::adjust_texcoord_for_hand_made_texture(int const& w, int const& h)
{
    //texture ratio adjustment because the texture is sized at power of 2.
    //have to do this for hand made texture like Movie Texture or Text Texture.
    float x = 1, y = 1;
    while( x < w ){x*=2;} float tex_coord_ratio_x = w / x;
    while( y < h ){y*=2;} float tex_coord_ratio_y = h / y;

    S3DVertex* ptr = static_cast<S3DVertex*>(thismesh_->getMeshBuffer(0)->getVertices());
    ptr[0].TCoords.Y = tex_coord_ratio_y;
    ptr[1].TCoords.X = tex_coord_ratio_x; ptr[1].TCoords.Y = tex_coord_ratio_y;
    ptr[3].TCoords.X = tex_coord_ratio_x;
}

Sprite& Sprite::setDepth(float d)
{
    vector2df pos2d = get<Pos2D>();
    set<Pos3D>(vector3df(pos2d.X, -pos2d.Y, d));
    return *this;
}

Sprite& Sprite::setCenterAligned(bool const& center)
{
    if( center_ == center ) return *this;
    center_ = center;
    matrix4 mat;
    if( center_ )
        mat.setTranslation( vector3df(-size_.Width/2, size_.Height/2, 0) );
    else
        mat.setTranslation( vector3df(+size_.Width/2, -size_.Height/2, 0) );
    IMeshManipulator* mani = smgr_->getMeshManipulator();
    mani->transformMesh( thismesh_, mat );
    return *this;
}

Sprite& Sprite::moveTo(int x, int y)
{
    //body_->setPosition(vector3df(x-320.f+51.f, -y+240.f-21.f, body_->getPosition().Z));
    set<Pos2D>(vector2df(x, y));
    return *this;
}

Sprite& Sprite::moveTween(int x, int y, int duration, function<void()> cb, int delay)
{
    vector2df newpos = vector2df(x, y);
    tween<Linear, Pos2D>(newpos, duration, 0, cb, delay);
    return *this;
}

Sprite* Sprite::clone() const
{
    Sprite* obj = new Sprite(*this);
    return obj;
}


/// Maybe this feature should move to another class and use multiple inheritance.

ctrl::CallbackDelegate& Sprite::onPress(ctrl::Button const* btn) {
    return onButtonEvent(btn, ctrl::BTN_PRESS);
}

ctrl::CallbackDelegate& Sprite::onRelease(ctrl::Button const* btn) {
    return onButtonEvent(btn, ctrl::BTN_RELEASE);
}

ctrl::CallbackDelegate& Sprite::onDown(ctrl::Button const* btn) {
    return onButtonEvent(btn, ctrl::BTN_DOWN);
}

ctrl::CallbackDelegate& Sprite::onUp(ctrl::Button const* btn) {
    return onButtonEvent(btn, ctrl::BTN_UP);
}

ctrl::CallbackDelegate&
Sprite::onButtonEvent(ctrl::Button const* btn, ctrl::BSTATE const& state)
{
    BOOST_FOREACH(ctrl::CallbackDelegate& cd, delegates_)
        if( btn == cd.subscribed_btn() && state == cd.subscribed_state() )
            return cd;

    ctrl::CallbackDelegate cd(static_pointer_cast<Sprite>(shared_from_this()), btn, state);
    delegates_.push_back( cd );
    return delegates_.back();
}

//ctrl::CallbackDelegate& Sprite::onFocus(ctrl::Crosshair const* cursor)
//{
//    ctrl::CallbackDelegate cd(shared_from_this(), cursor);
//    delegates_.push_back( cd );
//    return delegates_.back();
//}
//
//ctrl::CallbackDelegate& Sprite::onLeaveFocus(ctrl::Crosshair const* cursor)
//{
//    ctrl::CallbackDelegate cd(shared_from_this(), cursor);
//    delegates_.push_back( cd );
//    return delegates_.back();
//}

/// End of Maybe

Sprite::~Sprite()
{
    if( thismesh_ )
        thismesh_->drop();
}
