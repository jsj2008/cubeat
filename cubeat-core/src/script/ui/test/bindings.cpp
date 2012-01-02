#include "view/Scene.hpp"
#include "view/Sprite.hpp"
#include "Accessors.hpp"
#include "test/test_ui.hpp"

using namespace psc;
using namespace view;
using namespace accessor;

extern "C" {
#include "script/ui/test/bindings.h"
}

pScene* TestUI_get_ui_scene(TestUI* self) {
    pScene* s = new pScene;
    *s = self->getUIScene();
    return s;
}

pSprite* Sprite_create(char const* name, pScene* s, int w, int h, bool center) {
    pSprite* sp = new pSprite;
    *sp = Sprite::create(name, *s, w, h, center);
    return sp;
}

void Sprite_set_pos(pSprite* self, double x, double y) {
    (*self)->set<Pos2D>(vec2(x, y));
}

void Scene__gc(pScene* self) {
    delete self;
}

void Sprite__gc(pSprite* self) {
    delete self;
}