#include "../Source/SpeechEngine.h"
#include <vector>
#include <cmath>
#include <iostream>
#include <cstdlib>
#include <cassert>
using namespace speechshare;
static void require(bool p,const char* why) { if(!p){std::cerr<<"FAIL "<<why<<"\n";std::exit(1);} }
static std::vector<float> constant(int frames,int pairs,float first,float second=0) {
    std::vector<float> a(std::size_t(frames)*pairs*2);
    for(int f=0;f<frames;f++){a[std::size_t(f)*pairs*2]=first;a[std::size_t(f)*pairs*2+1]=first;
      if(pairs>1){a[std::size_t(f)*pairs*2+2]=second;a[std::size_t(f)*pairs*2+3]=second;}}
    return a;
}
int main(){
    constexpr int sr=48000;
    Controls c;c.rmsMs=20;c.attackMs=1;c.releaseMs=1;c.highPassHz=0;c.keyMaster=false;
    Engine e;e.prepare(sr);
    {auto a=constant(sr,1,.1f); e.process(a.data(),sr,2,c);require(std::abs(a.back()-.1)<.003,"one mic unity classic");}
    {e.reset(); c.smart=false;auto a=constant(sr,2,.1f,.1f);e.process(a.data(),sr,4,c);
     require(std::abs(a[a.size()-1]-.0707107)<.005,"two equal classic sharing");}
    {e.reset(); c.smart=false; c.group[1]=1;auto a=constant(sr,2,.1f,.1f);e.process(a.data(),sr,4,c);
     require(std::abs(a[a.size()-1]-.1)<.005,"two groups independent");}
    {e.reset(); c.group[1]=0;c.keyMaster=true; c.keyDb[0]=-30;c.keyType[0]=Generator::sine1000;
      auto a=constant(sr,2,.01f,.01f);e.process(a.data(),sr,4,c);require(std::abs(a.back())<.007,"reference competes");}
    {e.reset(); c.keyMaster=false;c.routing=Routing::sum;c.smart=true;auto a=constant(sr,2,.1f,.2f);e.process(a.data(),sr,4,c);
     require(a[a.size()-1]==0,"sum clears other pairs");require(std::abs(a[a.size()-4])>.01,"sum produces output");}
    {e.reset();c.routing=Routing::multi;c.engine=false;auto a=constant(sr,2,.125f,.25f);e.process(a.data(),sr,4,c);
     require(std::abs(a.back()-.25f)<.001,"engine off dry output");}
    {e.reset();c.engine=true;c.smart=true;c.keyMaster=false;auto a=constant(sr,2,0,0);e.process(a.data(),sr,4,c);
     for(float v:a)require(std::isfinite(v),"digital silence finite");}
    {e.reset();c.smart=false;c.highPassHz=100;c.keyMaster=true;c.keyDb[0]=-56;
     c.keyType[0]=Generator::sine40; auto a=constant(sr,1,.0001f);e.process(a.data(),sr,2,c);
     float d40=e.meters().keyPostDb[0];e.reset();c.keyType[0]=Generator::sine1000;a=constant(sr,1,.0001f);e.process(a.data(),sr,2,c);
     float d1000=e.meters().keyPostDb[0];require(d1000>d40+6,"40 Hz HPF attenuated more than 1000 Hz");
     std::cout<<"KEY HPF: 40 Hz="<<d40<<" dBFS, 1000 Hz="<<d1000<<" dBFS\n";}
    {e.reset();c.keyMaster=false;c.lookaheadMs=10;
     auto a=constant(sr/2,1,.05f);e.process(a.data(),sr/2,2,c);
     require(std::abs(a[0])<.001,"lookahead delays sample");require(e.latencySamples(c)==480,"delay reported");}
    // Stress: 32 microphones, noisy program, parameter switches, grouping,
    // key generators and short windows must remain finite.
    { e.prepare(sr); speechshare::Controls stress;
      stress.activeMics=32; stress.rmsMs=10; stress.strength=2.7f;
      stress.keyMaster=true; stress.keyEnabled={true,true,true};
      stress.keyType={Generator::pink,Generator::sine40,Generator::sine1000};
      for(int i=0;i<32;++i) stress.group[i]=i%3;
      std::vector<float> a(4096*64);
      for(std::size_t i=0;i<a.size();++i)a[i]=.01f*float(std::sin(double(i)*.07));
      e.process(a.data(),4096,64,stress);
      stress.smart=true; stress.routing=Routing::sum;stress.lookaheadMs=15;
      e.process(a.data(),4096,64,stress);
      for(float v:a)require(std::isfinite(v),"stress 64 channels finite");
    }
    std::cout<<"All independent core tests passed.\n";
}
