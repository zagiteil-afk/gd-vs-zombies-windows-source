#include "Game.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

using namespace gvz;
void check(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
void run(Game& g, float seconds) { for (int i=0; i<static_cast<int>(std::ceil(seconds/Tick)); ++i) g.advance(Tick); }
Game sandbox(int energy=5000) { return Game(123, Config{5,false,energy}); }
const Enemy* enemy(const Game& g, std::uint32_t id) {
    for (const auto& e:g.enemies()) if (e.id==id) return &e;
    return nullptr;
}
void economy() {
    auto g=sandbox(150);
    check(g.place(PlantKind::Shooter,0,1)==PlaceResult::Placed,"place shooter");
    check(g.energy()==50,"charge exact price");
    check(g.place(PlantKind::Generator,0,1)==PlaceResult::Occupied,"reject occupied tile");
    check(g.place(PlantKind::Ice,0,2)==PlaceResult::TooExpensive,"reject unaffordable");
    check(g.place(PlantKind::Generator,0,2)==PlaceResult::Placed,"place generator");
    check(g.energy()==0,"no overspending");
    run(g,6.2f);
    check(g.energy()==50,"sky plus generator income");
    check(g.shovel(0,2),"shovel removes plant");
    check(g.energy()==50,"shovel gives no refund");
    check(!g.plantAt(0,2),"shoveled tile empty");
    check(g.place(PlantKind::Wall,-1,0)==PlaceResult::Invalid,"reject out of bounds");
    auto rich=sandbox();
    rich.place(PlantKind::Shooter,0,0);
    check(rich.place(PlantKind::Shooter,1,0)==PlaceResult::CoolingDown,"shared card cooldown");
    run(rich,2.1f);
    check(rich.place(PlantKind::Shooter,1,0)==PlaceResult::Placed,"cooldown recharges");
}
void pause() {
    auto g=sandbox(); g.place(PlantKind::Shooter,0,0);
    auto id=g.spawnEnemy(EnemyKind::Normal,0,8.f);
    auto x=enemy(g,id)->x; auto cd=g.cooldown(PlantKind::Shooter); auto money=g.energy();
    g.setPaused(true); run(g,20.f);
    check(enemy(g,id)->x==x && g.cooldown(PlantKind::Shooter)==cd && g.energy()==money,"pause freezes simulation");
    check(g.place(PlantKind::Wall,1,1)==PlaceResult::Invalid,"cannot plant while paused");
    check(!g.shovel(0,0),"cannot shovel while paused");
    g.setPaused(false); run(g,1.f); check(enemy(g,id)->x<x,"unpause resumes");
}
void shooting() {
    auto g=sandbox(); g.place(PlantKind::Shooter,0,0);
    auto far=g.spawnEnemy(EnemyKind::Armored,0,7.f);
    auto near=g.spawnEnemy(EnemyKind::Normal,0,3.f);
    auto off=g.spawnEnemy(EnemyKind::Normal,1,3.f);
    run(g,1.f);
    check(enemy(g,near) && enemy(g,near)->hp<enemy(g,near)->maxHp,"nearest target hit first");
    check(enemy(g,far)->hp==enemy(g,far)->maxHp,"front enemy shields back enemy");
    check(enemy(g,off)->hp==enemy(g,off)->maxHp,"shot stays in lane");
    run(g,6.f);
    check(!enemy(g,near),"shooter kills enemy");
    check(g.kills()>=1,"kills counted");
}
void ice() {
    auto g=sandbox(); g.place(PlantKind::Ice,0,0);
    auto chilled=g.spawnEnemy(EnemyKind::Armored,0,3.f);
    auto plain=g.spawnEnemy(EnemyKind::Armored,1,3.f);
    run(g,2.f);
    check(enemy(g,chilled)->slow>0.f,"ice applies slow");
    check(enemy(g,chilled)->x>enemy(g,plain)->x,"slow reduces distance travelled");
}
void bomb() {
    auto g=sandbox();
    check(g.place(PlantKind::Bomb,2,4)==PlaceResult::Placed,"place bomb");
    auto center=g.spawnEnemy(EnemyKind::Armored,2,4.6f);
    auto above=g.spawnEnemy(EnemyKind::Normal,1,4.6f);
    auto below=g.spawnEnemy(EnemyKind::Normal,3,4.6f);
    auto outside=g.spawnEnemy(EnemyKind::Normal,0,4.6f);
    auto distant=g.spawnEnemy(EnemyKind::Normal,2,8.f);
    run(g,1.2f);
    check(!enemy(g,center)&&!enemy(g,above)&&!enemy(g,below),"bomb hits 3 neighboring lanes");
    check(enemy(g,outside)&&enemy(g,distant),"bomb has bounded range");
    check(!g.plantAt(2,4),"bomb consumed once");
    check(g.kills()==3,"no duplicate kill credit");
}
void wall() {
    auto g=sandbox(); g.place(PlantKind::Wall,0,2);
    auto id=g.spawnEnemy(EnemyKind::Normal,0,3.f);
    run(g,3.f);
    check(g.plantAt(0,2) && g.plantAt(0,2)->hp<950.f,"enemy bites wall");
    check(enemy(g,id)->x>2.9f,"wall stops movement");
    run(g,36.f);
    check(!g.plantAt(0,2),"wall eventually breaks");
    check(enemy(g,id) && enemy(g,id)->x<2.9f,"enemy moves after wall breaks");
}
void mowersAndLoss() {
    auto g=sandbox();
    auto first=g.spawnEnemy(EnemyKind::Normal,2,.01f);
    auto second=g.spawnEnemy(EnemyKind::Armored,2,8.f);
    auto adjacent=g.spawnEnemy(EnemyKind::Normal,3,8.f);
    run(g,1.1f);
    check(!enemy(g,first)&&!enemy(g,second),"mower clears whole lane");
    check(enemy(g,adjacent),"mower cannot hit other lane");
    check(!g.mowers()[2].available&&!g.mowers()[2].active,"mower consumed");
    g.spawnEnemy(EnemyKind::Runner,2,-.74f); run(g,.1f);
    check(g.phase()==Phase::Lost,"second breach loses");
    auto money=g.energy(); run(g,20.f); check(g.energy()==money,"end state stops simulation");
}
void frameRate() {
    Game a(19),b(19);
    for (int i=0;i<900;++i) a.advance(1.f/30.f);
    for (int i=0;i<1800;++i) b.advance(1.f/60.f);
    check(a.energy()==b.energy() && a.wave()==b.wave() && a.enemies().size()==b.enemies().size(),"fixed timestep independent of frame rate");
    for (std::size_t i=0;i<a.enemies().size();++i)
        check(std::abs(a.enemies()[i].x-b.enemies()[i].x)<.001f,"same seeded movement");
}
void victory() {
    Game g(1,Config{1,true,9999});
    // Deliberately strong test defense; verifies automatic spawning and terminal win.
    for (int row=0;row<Rows;++row) {
        check(g.place(PlantKind::Shooter,row,1)==PlaceResult::Placed,"victory setup"); run(g,2.05f);
    }
    for (int row=0;row<Rows;++row) {
        check(g.place(PlantKind::Shooter,row,2)==PlaceResult::Placed,"victory setup second column"); run(g,2.05f);
    }
    run(g,120.f);
    check(g.phase()==Phase::Won,"win after final enemy in final wave");
    check(g.wave()==1 && g.kills()==9,"all scheduled enemies spawned and defeated");
    check(g.place(PlantKind::Wall,0,8)==PlaceResult::Invalid,"no placements after victory");
}
int main() {
    int passed=0;
    auto test=[&](const char* name, auto fn) {
        fn(); ++passed; std::cout<<"PASS "<<name<<'\n';
    };
    try {
        test("economy and placement",economy); test("pause",pause); test("lane combat",shooting);
        test("freeze",ice); test("bomb area",bomb); test("wall blocking",wall);
        test("mowers and defeat",mowersAndLoss); test("frame rate consistency",frameRate); test("full wave and victory",victory);
    } catch(const std::exception& e) { std::cerr<<"FAIL "<<e.what()<<'\n'; return 1; }
    std::cout<<passed<<" tests passed\n";
}
