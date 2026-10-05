#include "Game.hpp"
#include <iostream>
#include <algorithm>
using namespace gvz;
int main(){
 int wins=0;
 for(int seed=1;seed<=20;++seed){
  Game g(seed);int ticks=0;
  for(;ticks<60*600&&g.phase()==Phase::Playing;++ticks){
   if(ticks%6==0){
    // Defend lanes as threats arrive; invest remaining energy in the economy.
    for(const auto& e:g.enemies())
     if(!g.plantAt(e.row,1))g.place(PlantKind::Shooter,e.row,1);
    for(int row=0;row<Rows;++row)if(!g.plantAt(row,0))g.place(PlantKind::Generator,row,0);
    for(int row=0;row<Rows;++row)if(!g.plantAt(row,1))g.place(PlantKind::Shooter,row,1);
    for(int row=0;row<Rows;++row)if(!g.plantAt(row,2))g.place(PlantKind::Shooter,row,2);
    for(int row=0;row<Rows;++row)if(!g.plantAt(row,5))g.place(PlantKind::Wall,row,5);
    for(int row=0;row<Rows;++row)if(!g.plantAt(row,3))g.place(PlantKind::Ice,row,3);
   }
   g.advance(Tick);
  }
  bool won=g.phase()==Phase::Won;wins+=won;
  std::cout<<"seed="<<seed<<" result="<<(won?"WIN":"FAIL")<<" wave="<<g.wave()<<" kills="<<g.kills()<<" time="<<ticks/60<<"s\n";
 }
 std::cout<<wins<<"/20 wins\n";
 return wins==20?0:1;
}
