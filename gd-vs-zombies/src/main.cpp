#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include "GardenLayer.hpp"

using namespace geode::prelude;

class $modify(GVZMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;
        auto badge = ButtonSprite::create("GD vs Z", 70, true, "bigFont.fnt", "GJ_button_01.png", 24.f, .45f);
        auto item = CCMenuItemSpriteExtra::create(badge, this, menu_selector(GVZMenuLayer::onGarden));
        item->setID("garden-button"_spr);
        auto menu = CCMenu::create();
        menu->setID("garden-menu"_spr);
        auto size = CCDirector::sharedDirector()->getWinSize();
        menu->setPosition({size.width - 54.f, size.height - 24.f});
        menu->addChild(item);
        addChild(menu, 100);
        return true;
    }
    void onGarden(CCObject*) {
        CCDirector::sharedDirector()->pushScene(GardenLayer::scene());
    }
};
