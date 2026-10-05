#include "GardenLayer.hpp"
#include <algorithm>
#include <cmath>
#include <ctime>
#include <string>
#include <unordered_set>

using namespace geode::prelude;
namespace {
constexpr float Width = 640.f, Height = 360.f;
constexpr float BX = 136.f, BY = 50.f, CW = 50.f, CH = 42.f;
const std::array<const char*, 5> Names = {"CUBE", "ORB", "WALL", "FROST", "BOMB"};
const std::array<const char*, 5> Hints = {
    "CUBE: shoots down its lane. Select a card, then an empty tile.",
    "ORB: generates 25 energy every 9 seconds. Protect it!",
    "WALL: absorbs bites. Put it in front of your shooters.",
    "FROST: slows enemies with ice shots. Costs 150 energy.",
    "BOMB: explodes after 1 second; hits nearby enemies in 3 lanes."
};
ccColor4F color(float r, float g, float b, float a = 1.f) { return {r,g,b,a}; }
CCPoint point(float x, int row) { return {BX + x * CW, BY + (4 - row + .5f) * CH}; }
void rect(CCDrawNode* d, float x, float y, float w, float h, ccColor4F fill,
          ccColor4F border = {0,0,0,0}, float stroke = 0.f) {
    CCPoint v[] = {{x,y}, {x+w,y}, {x+w,y+h}, {x,y+h}};
    d->drawPolygon(v, 4, fill, stroke, border);
}
CCLabelBMFont* label(CCNode* parent, const char* text, float x, float y, float scale,
                     ccColor3B tint = {230,240,255}) {
    auto l = CCLabelBMFont::create(text, "bigFont.fnt");
    l->setPosition({x,y}); l->setScale(scale); l->setColor(tint); parent->addChild(l);
    return l;
}
void fitText(CCLabelBMFont* l, const std::string& text, float scale, float maxWidth) {
    l->setString(text.c_str());
    l->setScale(scale);
    if (l->getContentSize().width * scale > maxWidth)
        l->setScale(maxWidth / l->getContentSize().width);
}
SimplePlayer* icon(IconType type, int frame, ccColor3B primary, ccColor3B secondary, float scale) {
    auto p = SimplePlayer::create(1);
    p->updatePlayerFrame(frame, type);
    p->setColor(primary); p->setSecondColor(secondary); p->setScale(scale);
    return p;
}
SimplePlayer* plantIcon(gvz::PlantKind kind, float scale) {
    switch (kind) {
        case gvz::PlantKind::Generator: return icon(IconType::Ball, 1, {255,208,45}, {255,249,190}, scale);
        case gvz::PlantKind::Wall: return icon(IconType::Cube, 2, {125,149,201}, {222,238,255}, scale);
        case gvz::PlantKind::Ice: return icon(IconType::Ship, 1, {55,214,255}, {227,255,255}, scale);
        case gvz::PlantKind::Bomb: return icon(IconType::Ball, 2, {255,91,92}, {255,202,55}, scale);
        default: return icon(IconType::Cube, 1, {93,255,139}, {223,255,236}, scale);
    }
}
CCMenuItemSpriteExtra* button(CCMenu* menu, const char* text, float x, float y, float w,
                             CCObject* target, SEL_MenuHandler callback, CCLabelBMFont** out = nullptr) {
    auto n = CCNode::create(); n->setContentSize({w,28});
    auto d = CCDrawNode::create(); n->addChild(d);
    rect(d, 0, 0, w, 28, color(.09f,.16f,.27f), color(.23f,.41f,.57f), 1.f);
    auto l = label(n, text, w/2, 14, .37f);
    fitText(l, text, .37f, w - 9);
    if (out) *out = l;
    auto b = CCMenuItemSpriteExtra::create(n, target, callback);
    b->setPosition({x,y}); menu->addChild(b); return b;
}
}
GardenLayer* GardenLayer::create() {
    auto ret = new GardenLayer;
    if (ret->init()) { ret->autorelease(); return ret; }
    delete ret; return nullptr;
}
CCScene* GardenLayer::scene() {
    auto s = CCScene::create();
    if (auto layer = create()) s->addChild(layer);
    return s;
}
bool GardenLayer::init() {
    if (!CCLayer::init()) return false;
    m_game = gvz::Game(static_cast<std::uint32_t>(std::time(nullptr)));
    setKeypadEnabled(true); setKeyboardEnabled(true);
    auto win = CCDirector::sharedDirector()->getWinSize();
    addChild(CCLayerColor::create({8,14,29,255}));
    float scale = std::min(win.width / Width, win.height / Height);
    m_canvas = CCNode::create(); m_canvas->setContentSize({Width,Height});
    m_canvas->setScale(scale);
    m_canvas->setPosition({(win.width - Width*scale)/2, (win.height - Height*scale)/2});
    addChild(m_canvas);

    auto background = CCDrawNode::create(); m_canvas->addChild(background);
    rect(background, 12, 12, 616, 336, color(.035f,.055f,.105f), color(.13f,.23f,.37f), 1.f);
    rect(background, BX-3, BY-3, CW*9+6, CH*5+6, color(.04f,.13f,.16f), color(.24f,.6f,.63f), 1.f);
    rect(background, 18, 50, 78, 261, color(.045f,.09f,.15f));
    for (int row=0; row<gvz::Rows; ++row) {
        for (int col=0; col<gvz::Cols; ++col) {
            bool odd = (row+col)%2 != 0;
            rect(background, BX+col*CW+1, BY+(4-row)*CH+1, CW-2, CH-2,
                 odd ? color(.07f,.20f,.22f) : color(.06f,.17f,.20f));
        }
    }
    auto title = label(m_canvas, "GD vs ZOMBIES", 164, 331, .65f, {131,255,169});
    fitText(title, "GD vs ZOMBIES", .65f, 280);
    m_waveLabel = label(m_canvas, "GET READY", 452, 331, .4f);
    label(m_canvas, "ENERGY", 57, 297, .3f, {255,217,104});
    m_energyLabel = label(m_canvas, "275", 57, 275, .62f, {255,222,80});
    label(m_canvas, "1-5", 57, 240, .38f);
    label(m_canvas, "CARDS", 57, 222, .25f);

    auto menu = CCMenu::create(); menu->setPosition({0,0}); m_canvas->addChild(menu, 5);
    button(menu, "X", 607, 330, 24, this, menu_selector(GardenLayer::onExit));
    button(menu, "PAUSE", 57, 187, 68, this, menu_selector(GardenLayer::onPause), &m_pauseLabel);
    button(menu, "SHOVEL", 57, 150, 68, this, menu_selector(GardenLayer::onShovel), &m_shovelLabel);
    button(menu, "RETRY", 57, 113, 68, this, menu_selector(GardenLayer::onRestart));
    button(menu, "HELP", 57, 76, 68, this, menu_selector(GardenLayer::onHelp));
    m_selectionDraw = CCDrawNode::create(); m_canvas->addChild(m_selectionDraw, 4);
    for (int i=0; i<5; ++i) {
        auto n = CCNode::create(); n->setContentSize({82,44});
        auto d = CCDrawNode::create(); n->addChild(d);
        rect(d, 0, 0, 82, 44, color(.08f,.13f,.22f), color(.2f,.35f,.48f), .6f);
        auto p = plantIcon(static_cast<gvz::PlantKind>(i), .55f);
        p->setPosition({16,22}); n->addChild(p);
        label(n, Names[i], 54, 31, .26f);
        m_costLabels[i] = label(n, "0", 54, 12, .34f, {255,226,113});
        auto b = CCMenuItemSpriteExtra::create(n, this, menu_selector(GardenLayer::onCard));
        b->setTag(i); b->setPosition({BX+41+i*90,289}); menu->addChild(b);
    }
    // Tile hit targets share the same scaled canvas as all visuals.
    auto cells = CCMenu::create(); cells->setPosition({0,0}); m_canvas->addChild(cells, 1);
    for (int row=0; row<gvz::Rows; ++row) for (int col=0; col<gvz::Cols; ++col) {
        auto n = CCNode::create(); n->setContentSize({CW-2,CH-2});
        auto item = CCMenuItemSpriteExtra::create(n, this, menu_selector(GardenLayer::onCell));
        item->setPosition(point(col+.5f,row)); item->setTag(row*gvz::Cols+col); cells->addChild(item);
    }
    m_actors = CCNode::create(); m_canvas->addChild(m_actors, 2);
    m_draw = CCDrawNode::create(); m_canvas->addChild(m_draw, 3);
    for (int row=0; row<gvz::Rows; ++row) {
        auto p = icon(IconType::Wave, 1, {136,255,188}, {236,255,245}, .65f);
        m_mowers[row] = p; m_canvas->addChild(p, 2);
    }
    m_hint = label(m_canvas, Hints[0], 320, 28, .28f);

    m_overlay = CCNode::create(); m_canvas->addChild(m_overlay, 6);
    auto dim = CCDrawNode::create(); m_overlay->addChild(dim);
    rect(dim, BX, BY, CW*9, CH*5, color(.01f,.03f,.07f,.88f));
    m_status = label(m_overlay, "PAUSED", 361, 170, .85f, {160,255,200});
    m_statusDetail = label(m_overlay, "Press SPACE or PAUSE to continue", 361, 137, .3f);
    m_overlay->setVisible(false);
    refreshHUD(); syncActors(); scheduleUpdate(); return true;
}
void GardenLayer::feedback(const char* message) {
    m_messageTime = 2.4f; fitText(m_hint, message, .3f, 593);
}
void GardenLayer::onCell(CCObject* sender) {
    int tag = static_cast<CCNode*>(sender)->getTag();
    if (m_shovel) {
        if (!m_game.shovel(tag/gvz::Cols, tag%gvz::Cols)) feedback("No plant to remove here.");
    } else {
        switch (m_game.place(m_selected, tag/gvz::Cols, tag%gvz::Cols)) {
            case gvz::PlaceResult::TooExpensive: feedback("Not enough energy. Build ORBs to earn more."); break;
            case gvz::PlaceResult::CoolingDown: feedback("That card is recharging."); break;
            case gvz::PlaceResult::Occupied: feedback("Tile occupied. Press S for the shovel."); break;
            case gvz::PlaceResult::Invalid: break;
            case gvz::PlaceResult::Placed: break;
        }
    }
    refreshHUD(); syncActors();
}
void GardenLayer::onCard(CCObject* sender) {
    m_selected = static_cast<gvz::PlantKind>(static_cast<CCNode*>(sender)->getTag());
    m_shovel = false; m_messageTime = 0.f; refreshHUD();
}
void GardenLayer::onShovel(CCObject*) { m_shovel = !m_shovel; m_messageTime = 0.f; refreshHUD(); }
void GardenLayer::onPause(CCObject*) {
    if (m_game.phase() == gvz::Phase::Playing) m_game.setPaused(!m_game.paused());
    refreshHUD();
}
void GardenLayer::onRestart(CCObject*) {
    m_game = gvz::Game(static_cast<std::uint32_t>(std::time(nullptr)));
    m_sprites.clear(); m_actors->removeAllChildrenWithCleanup(true);
    m_shovel = false; m_recorded = false; m_messageTime = 0.f; m_time = 0.f;
    refreshHUD(); syncActors();
}
void GardenLayer::onExit(CCObject*) { CCDirector::sharedDirector()->popScene(); }
void GardenLayer::onHelp(CCObject*) {
    if (m_game.phase() == gvz::Phase::Playing) m_game.setPaused(true);
    refreshHUD();
    FLAlertLayer::create("GD vs Zombies",
        "Survive <cg>5 waves</c>! Pick a card and click an empty tile.\n"
        "<cy>ORB</c>: +25 energy / 9s. Energy is collected automatically.\n"
        "<cg>CUBE</c>: shoots. <cb>WALL</c>: blocks. <cl>FROST</c>: slows.\n"
        "<cr>BOMB</c>: explodes in 3 lanes. Each lane has one rescue wave.\n"
        "<cy>1-5</c>: cards, <cy>S</c>: shovel, <cy>Space/Esc</c>: pause.\n"
        "Press <cg>RESUME</c> after closing this help.", "OK")->show();
}
void GardenLayer::keyBackClicked() { onPause(nullptr); }
void GardenLayer::keyDown(enumKeyCodes key, double timestamp) {
    int value = static_cast<int>(key);
    if (value >= static_cast<int>(KEY_One) && value <= static_cast<int>(KEY_Five)) {
        m_selected = static_cast<gvz::PlantKind>(value-static_cast<int>(KEY_One));
        m_shovel = false; m_messageTime = 0.f; refreshHUD(); return;
    }
    if (key == KEY_S) { onShovel(nullptr); return; }
    if (key == KEY_Space || key == KEY_Escape) { onPause(nullptr); return; }
    CCLayer::keyDown(key, timestamp);
}
void GardenLayer::update(float dt) {
    if (!m_game.paused() && m_game.phase() == gvz::Phase::Playing) m_time += std::min(dt,.25f);
    m_messageTime = std::max(0.f, m_messageTime - dt);
    m_game.advance(dt); syncActors(); refreshHUD();
    if (m_game.phase() == gvz::Phase::Won && !m_recorded) {
        m_recorded = true;
        auto mod = Mod::get();
        mod->setSavedValue<int>("wins", mod->getSavedValue<int>("wins", 0) + 1);
    }
}
void GardenLayer::refreshHUD() {
    fitText(m_energyLabel, std::to_string(m_game.energy()), .62f, 71);
    std::string wave = "WAVE " + std::to_string(m_game.wave()) + " / " + std::to_string(m_game.totalWaves());
    if (m_game.enemies().empty() && m_game.nextWaveIn() > 0.f && m_game.phase() == gvz::Phase::Playing) {
        wave = (m_game.wave() == 0 ? "START IN " : "NEXT IN ") + std::to_string(static_cast<int>(std::ceil(m_game.nextWaveIn()))) + "s";
    }
    fitText(m_waveLabel, wave, .4f, 240);
    m_selectionDraw->clear();
    for (int i=0; i<5; ++i) {
        auto kind = static_cast<gvz::PlantKind>(i);
        float cd = m_game.cooldown(kind);
        auto text = cd > 0.f ? std::to_string(static_cast<int>(std::ceil(cd))) + "s" : std::to_string(gvz::spec(kind).cost);
        fitText(m_costLabels[i], text, .34f, 43);
        m_costLabels[i]->setColor(cd > 0.f || m_game.energy() < gvz::spec(kind).cost ? ccColor3B{145,156,178} : ccColor3B{255,226,113});
        if (!m_shovel && i == static_cast<int>(m_selected))
            rect(m_selectionDraw, BX+i*90-2, 265, 86, 48, color(0,0,0,0), color(.43f,1.f,.65f), 1.3f);
    }
    if (m_messageTime <= 0.f) fitText(m_hint, m_shovel ? "SHOVEL: click a plant to remove it. No energy refund. S to cancel." : Hints[static_cast<int>(m_selected)], .28f, 593);
    fitText(m_pauseLabel, m_game.paused() ? "RESUME" : "PAUSE", .37f, 59);
    m_shovelLabel->setColor(m_shovel ? ccColor3B{255,215,84} : ccColor3B{230,240,255});
    auto phase = m_game.phase();
    m_overlay->setVisible(m_game.paused() || phase != gvz::Phase::Playing);
    if (phase == gvz::Phase::Won) {
        fitText(m_status, "LAWN DEFENDED!", .85f, 404);
        fitText(m_statusDetail, "All 5 waves cleared! RETRY for another run.", .3f, 404);
    } else if (phase == gvz::Phase::Lost) {
        fitText(m_status, "THEY GOT THROUGH", .75f, 404);
        fitText(m_statusDetail, "Build ORBs early. Use every lane. Press RETRY.", .3f, 404);
    } else {
        fitText(m_status, "PAUSED", .85f, 404);
        fitText(m_statusDetail, "Press SPACE or RESUME to continue", .3f, 404);
    }
}
void GardenLayer::syncActors() {
    std::unordered_set<std::uint32_t> seen;
    m_draw->clear();
    auto getSprite = [&](std::uint32_t id, auto createSprite) {
        seen.insert(id);
        auto found = m_sprites.find(id);
        if (found != m_sprites.end()) return found->second;
        auto p = createSprite(); m_actors->addChild(p); m_sprites.emplace(id,p); return p;
    };
    auto health = [&](CCPoint pos, float hp, float maxHP) {
        if (hp >= maxHP) return;
        rect(m_draw, pos.x-14, pos.y-17, 28, 3, color(.03f,.05f,.08f));
        rect(m_draw, pos.x-14, pos.y-17, 28*std::clamp(hp/maxHP,0.f,1.f), 3, color(.5f,1.f,.55f));
    };
    for (const auto& p : m_game.plants()) {
        auto s = getSprite(p.id, [&] { return plantIcon(p.kind, .83f); });
        auto pos = point(p.col+.5f, p.row); s->setPosition(pos);
        s->setScale(.83f + .025f * std::sin(m_time*3.f + p.id));
        if (p.kind == gvz::PlantKind::Bomb) s->setRotation(m_time*180.f);
        if (p.kind == gvz::PlantKind::Generator) s->setRotation(m_time*22.f);
        health(pos, p.hp, p.maxHp);
    }
    for (const auto& e : m_game.enemies()) {
        auto s = getSprite(e.id, [&] {
            auto p = icon(IconType::Cube, e.kind == gvz::EnemyKind::Armored ? 4 : 1,
                          {247,96,130}, {100,14,64}, e.kind == gvz::EnemyKind::Armored ? 1.f : .85f);
            p->setFlipX(true); return p;
        });
        auto pos = point(e.x,e.row); s->setPosition({pos.x,pos.y+1.5f*std::sin(m_time*7.f+e.id)});
        s->setRotation(4.f*std::sin(m_time*5.f+e.id));
        ccColor3B tint = e.kind == gvz::EnemyKind::Armored ? ccColor3B{180,151,225} :
                        e.kind == gvz::EnemyKind::Runner ? ccColor3B{255,164,78} : ccColor3B{247,96,130};
        s->setColor(e.slow > 0.f ? ccColor3B{83,204,255} : tint);
        health(pos,e.hp,e.maxHp);
    }
    for (const auto& shot : m_game.shots()) {
        auto s = getSprite(shot.id, [&] { return icon(IconType::Ball, 1,
            shot.ice ? ccColor3B{66,210,255} : ccColor3B{102,255,141}, {245,255,251}, .23f); });
        s->setPosition(point(shot.x, shot.row)); s->setRotation(m_time*260.f);
    }
    for (auto it=m_sprites.begin(); it!=m_sprites.end();) {
        if (!seen.count(it->first)) { it->second->removeFromParentAndCleanup(true); it=m_sprites.erase(it); }
        else ++it;
    }
    for (int row=0; row<gvz::Rows; ++row) {
        const auto& mower = m_game.mowers()[row]; auto s = m_mowers[row];
        s->setVisible(mower.available || mower.active); s->setPosition(point(mower.x,row));
        if (mower.active) {
            auto p = point(mower.x,row);
            m_draw->drawSegment({p.x-24,p.y}, {p.x-6,p.y}, 3.f, color(.3f,1.f,.65f,.6f));
        }
    }
    for (const auto& fx : m_game.effects()) {
        auto p = point(fx.x,fx.row);
        float alpha = std::clamp(fx.remaining / (fx.blast ? .55f : .3f),0.f,1.f);
        m_draw->drawDot(p, fx.blast ? 20.f + (1.f-alpha)*65.f : 5.f + (1.f-alpha)*14.f,
                       fx.blast ? color(1.f,.55f,.2f,alpha*.38f) : color(.55f,1.f,.75f,alpha*.45f));
    }
}
