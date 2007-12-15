#ifndef _SHOOTING_CUBES_SETTERS_
#define _SHOOTING_CUBES_SETTERS_

#include "ISceneNode.h"

namespace irr {
namespace scene {

namespace accessor {

    template <class T>
    struct Accessor {
        typedef T value_type;
    };

    struct Pos3D : Accessor<core::vector3df>{
        static void set(ISceneNode* node, value_type const& val ) {
            node->setPosition(val);
        }
        static void get(ISceneNode const* node, value_type& out) {
            out = node->getPosition();
        }
    };

    struct Pos2D : Accessor<core::vector2df>{
        static void set(ISceneNode* node, value_type const& val ) {
            f32 z = node->getPosition().Z;
            node->setPosition(core::vector3df(val.X, val.Y, z));
        }
        static void get(ISceneNode const* node, value_type& out) {
            core::vector3df pos;
            Pos3D::get(node, pos);
            out = value_type(pos.X, pos.Y);
        }
    };

    struct Rotation : Accessor<core::vector3df>{
        static void set(ISceneNode* node, value_type const& val ) {
            node->setRotation(val);
        }
        static void get(ISceneNode const* node, value_type& out) {
            out = node->getRotation();
        }
    };

    struct Scale : Accessor<core::vector3df>{
        static void set(ISceneNode* node, value_type const& val ) {
            node->setScale(val);
        }
        static void get(ISceneNode const* node, value_type& out) {
            out = node->getScale();
        }
    };

    struct RGB : Accessor<s32>{
        static void set(ISceneNode* node, value_type const& val ) {
            if( !node->getMaterialCount() ) return;
            node->getMaterial(0).DiffuseColor.setRed(val%256);
            node->getMaterial(0).DiffuseColor.setGreen(val%256);
            node->getMaterial(0).DiffuseColor.setBlue(val%256);
        }
        static void get(ISceneNode* node, value_type& out) {
            if( !node->getMaterialCount() ) return;
            out = node->getMaterial(0).DiffuseColor.getAverage();
        }
    };

    struct Red : Accessor<s32>{
        static void set(ISceneNode* node, value_type const& val ) {
            if( !node->getMaterialCount() ) return;
            node->getMaterial(0).DiffuseColor.setRed(val);
        }
        static void get(ISceneNode* node, value_type& out) {
            if( !node->getMaterialCount() ) return;
            out = node->getMaterial(0).DiffuseColor.getRed();
        }
    };

    struct Green : Accessor<s32>{
        static void set(ISceneNode* node, value_type const& val ) {
            if( !node->getMaterialCount() ) return;
            node->getMaterial(0).DiffuseColor.setGreen(val);
        }
        static void get(ISceneNode* node, value_type& out) {
            if( !node->getMaterialCount() ) return;
            out = node->getMaterial(0).DiffuseColor.getGreen();
        }
    };

    struct Blue : Accessor<s32>{
        static void set(ISceneNode* node, value_type const& val ) {
            if( !node->getMaterialCount() ) return;
            node->getMaterial(0).DiffuseColor.setBlue(val);
        }
        static void get(ISceneNode* node, value_type& out) {
            if( !node->getMaterialCount() ) return;
            out = node->getMaterial(0).DiffuseColor.getBlue();
        }
    };

    struct Alpha : Accessor<s32>{
        static void set(ISceneNode* node, value_type const& val ) {
            if( !node->getMaterialCount() ) return;
            node->getMaterial(0).DiffuseColor.setAlpha(val);
        }
        static void get(ISceneNode* node, value_type& out) {
            if( !node->getMaterialCount() ) return;
            out = node->getMaterial(0).DiffuseColor.getAlpha();
        }
    };

    struct Frame : Accessor<f32>{
        static void set(ISceneNode* node, value_type const& val ) {
            if( node->getType() != ESNT_ANIMATED_MESH ) return;
            static_cast<IAnimatedMeshSceneNode*>(node)->setCurrentFrame( val );
        }
        static void get(ISceneNode const* node, value_type& out) {
            if( node->getType() != ESNT_ANIMATED_MESH ) return;
            out = static_cast<IAnimatedMeshSceneNode const*>(node)->getFrameNr();
        }
    };
}   //accessor
}   //scene
}   //irr

#endif 