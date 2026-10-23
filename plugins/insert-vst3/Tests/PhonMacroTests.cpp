#include "../Insert/PhonMacro.h"
#include <cassert>
#include <cmath>
#include <iostream>
int main(){
 using namespace speechshare_phon;
 auto eq=[](float a,float b){return std::abs(a-b)<.001f;};
 assert(eq(mapping(0).strength,.60f));
 assert(eq(mapping(50).strength,1.f));
 assert(eq(mapping(100).strength,2.20f));
 assert(eq(mapping(50).attackMs,12.f));
 assert(eq(mapping(50).releaseMs,160.f));
 assert(eq(mapping(50).rmsMs,100.f));
 assert(eq(mapping(50).hpfHz,100.f));
 for(int x=1;x<=100;++x){
  const auto p=mapping(float(x)),m=mapping(float(x-1));
  assert(p.strength>=m.strength);
  assert(eq(p.rmsMs,100.f));
  assert(p.attackMs<=m.attackMs);
  assert(p.releaseMs<=m.releaseMs);
  assert(p.strength>=.5f&&p.strength<=3.f);
  assert(p.rmsMs>=10.f&&p.rmsMs<=500.f);
  assert(p.attackMs>=1.f&&p.attackMs<=120.f);
  assert(p.releaseMs>=10.f&&p.releaseMs<=1500.f);
  assert(eq(positionForStrength(p.strength),float(x)));
 }
 assert(eq(mapping(-500).strength,mapping(0).strength));
 assert(eq(mapping(500).strength,mapping(100).strength));
 assert(eq(positionForStrength(NAN),50));
 std::cout<<"PHON macro tests OK\n";
}
