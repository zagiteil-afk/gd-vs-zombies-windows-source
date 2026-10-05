#pragma once
#include <Geode/Geode.hpp>
#include "Game.hpp"
#include <array>
#include <unordered_map>

class GardenLayer : public cocos2d::CCLayer {
public:
    static GardenLayer* create();
    static cocos2d::CCScene* scene();
    bool init() override;
    void update(float dt) override;
    void keyDown(cocos2d::enumKeyCodes key, double timestamp) override;
    void keyBackClicked() override;
private:
    void onCell(cocos2d::CCObject* sender);
    void onCard(cocos2d::CCObject* sender);
    void onShovel(cocos2d::CCObject*);
    void onPause(cocos2d::CCObject*);
    void onRestart(cocos2d::CCObject*);
    void onExit(cocos2d::CCObject*);
    void onHelp(cocos2d::CCObject*);
    void syncActors();
    void refreshHUD();
    void feedback(const char* message);
    gvz::Game m_game;
    gvz::PlantKind m_selected = gvz::PlantKind::Shooter;
    bool m_shovel = false;
    bool m_recorded = false;
    float m_time = 0.f, m_messageTime = 0.f;
    cocos2d::CCNode* m_canvas = nullptr;
    cocos2d::CCNode* m_actors = nullptr;
    cocos2d::CCDrawNode* m_draw = nullptr;
    cocos2d::CCDrawNode* m_selectionDraw = nullptr;
    cocos2d::CCLabelBMFont* m_energyLabel = nullptr;
    cocos2d::CCLabelBMFont* m_waveLabel = nullptr;
    cocos2d::CCLabelBMFont* m_hint = nullptr;
    cocos2d::CCLabelBMFont* m_pauseLabel = nullptr;
    cocos2d::CCLabelBMFont* m_shovelLabel = nullptr;
    cocos2d::CCNode* m_overlay = nullptr;
    cocos2d::CCLabelBMFont* m_status = nullptr;
    cocos2d::CCLabelBMFont* m_statusDetail = nullptr;
    std::array<cocos2d::CCLabelBMFont*, 5> m_costLabels{};
    std::array<SimplePlayer*, gvz::Rows> m_mowers{};
    std::unordered_map<std::uint32_t, SimplePlayer*> m_sprites;
};
