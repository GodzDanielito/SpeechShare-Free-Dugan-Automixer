#include "SpeechEngine.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace speechshare {
namespace {
constexpr double pi=3.1415926535897932384626433832795;
constexpr double eps=1e-30;
constexpr double tiny=1e-24;
inline double coeff(double sr,double ms) { return std::exp(-1.0 / (sr*std::max(0.001,ms*.001))); }
inline double rise(double sr,double seconds) { return 1.0-std::exp(-1.0/(sr*seconds)); }
inline double clamp01(double v) { return std::clamp(v,0.0,1.0); }
inline double ampDb(double db) { return std::pow(10.0,db*.05); }
}
float Engine::db(double amp) { return float(std::max(-96.,20.*std::log10(std::max(1e-12,amp)))); }
double Engine::noise() { randomState ^= randomState<<13; randomState ^= randomState>>17; randomState ^= randomState<<5; return (double(randomState) / 4294967295.)*2.-1.; }

void Engine::prepare(double sampleRate) {
    rate=std::max(8000.,sampleRate);
    maxRms=std::max(1,int(std::ceil(rate*.510)));
    maxDelay=std::max(1,int(std::ceil(rate*.031)));
    rmsRing.assign(std::size_t(maxMics)*std::size_t(maxRms),0.f);
    delayRing.assign(std::size_t(maxChannels)*std::size_t(maxDelay),0.f);
    reset();
}
void Engine::reset() {
    for(auto& m:mics) m=Mic{};
    for(auto& k:keys) k=Key{};
    groupSum.fill(0);groupTarget.fill(1);groupGoalSmooth.fill(1);groupNorm.fill(1);groupCount.fill(0);
    meter=Meters{};
    std::fill(rmsRing.begin(),rmsRing.end(),0.f);
    std::fill(delayRing.begin(),delayRing.end(),0.f);
    window=1;windowPos=0;delay=0;delayPos=0;hop=0;fadeRemaining=0;noiseSamples=0;
}
int Engine::latencySamples(const Controls& c) const noexcept {
    return std::clamp(int(std::lround(rate * std::max(0.f,c.lookaheadMs)*.001)),0,maxDelay);
}
void Engine::process(float* io,int frames,int channels,const Controls& c) {
    if(!io||frames<=0||channels<=0)return;
    if(rmsRing.empty()) prepare(rate);
    const int nChannels=std::clamp(channels,0,maxChannels);
    const int count=std::max(1,std::min({maxMics,nChannels/2,c.activeMics}));
    const int newWindow=std::clamp(int(std::lround(rate*std::clamp(c.rmsMs,10.f,500.f)*.001)),1,maxRms);
    const int newDelay=latencySamples(c);
    if(newWindow!=window) {
        window=newWindow; windowPos=0;
        for(auto& m:mics) m.powerSum=0;
        std::fill(rmsRing.begin(),rmsRing.end(),0.f);
    }
    if(newDelay!=delay) { delay=newDelay;delayPos=0;std::fill(delayRing.begin(),delayRing.end(),0.f); }
    if(c.smart!=previousSmart) {
        previousSmart=c.smart; fadeRemaining=int(rate*.05);
    }
    const double strength=std::clamp(double(c.strength),.5,3.);
    const double hpHz=std::clamp(double(c.highPassHz),0.,350.);
    const double hpCoeff=hpHz>0 ? std::exp(-2.*pi*hpHz/rate) : 0.;
    const double attackCoeff=coeff(rate,c.attackMs),releaseCoeff=coeff(rate,c.releaseMs);
    const double fastRise=rise(rate,.009),fastFall=rise(rate,.060);
    const double noiseDown=1.-std::exp(-64./(rate*.35));
    const double noiseUp=1.-std::exp(-64./(rate*8.));
    const double noiseStartup=1.-std::exp(-64./(rate*.7));
    const double keyPowerAlpha=rise(rate,std::max(.015,double(c.rmsMs)*.001));
    const double refSmooth=rise(rate,.045);
    const double algorithmFade=rise(rate,.010);
    const bool activeEngine=c.engine && c.routing!=Routing::bypass;

    auto isAuto = [&](int t) {
        int g=std::clamp(c.group[t],0,2);
        return activeEngine && c.mode[t]==ChannelMode::automatic && !c.groupOverride[g] && !c.groupMute[g];
    };
    for(int frame=0;frame<frames;++frame) {
        // 1) KEY audio generators feed detector only; they NEVER enter io.
        for(int g=0;g<3;++g) {
            auto& k=keys[g];
            double sample=0;
            double rmsLevel=ampDb(c.keyDb[g]);
            if(c.keyType[g]==Generator::pink) {
                double white=noise();
                k.b0=.99765*k.b0+white*.0990460;
                k.b1=.96300*k.b1+white*.2965164;
                k.b2=.57000*k.b2+white*1.0526913;
                sample=(k.b0+k.b1+k.b2+white*.1848)*.583*rmsLevel;
            } else {
                double frequency=c.keyType[g]==Generator::sine40 ? 40. : 1000.;
                k.phase+=frequency/rate;
                if(k.phase>=1.) k.phase-=std::floor(k.phase);
                sample=std::sin(2*pi*k.phase)*std::sqrt(2.)*rmsLevel;
            }
            double y=hpHz>0 ? hpCoeff*(k.hpY+sample-k.hpX) : sample;
            k.hpX=sample;k.hpY=y;
            k.prePower+=rise(rate,.13)*(sample*sample-k.prePower);
            k.detPower+=keyPowerAlpha*(y*y-k.detPower);
            k.detPower=std::max(0.,k.detPower);
            k.score=(c.keyMaster&&c.keyEnabled[g]) ? std::pow(k.detPower,strength) : 0.;
            meter.keyPostDb[g]=db(std::sqrt(k.detPower));
        }
        // 2) HPF + sliding RMS + fast envelope on microphone inputs.
        for(int t=0;t<count;++t) {
            auto& m=mics[t];
            float l=io[frame*channels+2*t],r=io[frame*channels+2*t+1];
            m.inL=l;m.inR=r;
            double x[2]={l,r};double y[2];
            for(int s=0;s<2;++s) {
                y[s]=hpHz>0 ? hpCoeff*(m.hpY[s]+x[s]-m.hpX[s]) : x[s];
                m.hpX[s]=float(x[s]);m.hpY[s]=float(y[s]);
            }
            const double power=.5*(y[0]*y[0]+y[1]*y[1]);
            m.fastPower=std::max(0.,m.fastPower+(power>m.fastPower?fastRise:fastFall)*(power-m.fastPower));
            float& slot=rmsRing[std::size_t(t)*maxRms+windowPos];
            m.powerSum=std::max(0.,m.powerSum+power-double(slot));
            slot=float(power);
        }
        if(++windowPos>=window)windowPos=0;
        // 3) Evaluate the gain-sharing scores every 64 samples like v1.5.
        if(++hop>=64) {
            hop=0;groupSum.fill(0);groupCount.fill(0);noiseSamples+=64;
            for(int t=0;t<count;++t) {
                auto& m=mics[t];
                m.slowPower=std::max(0.,m.powerSum/window);
                double env=std::sqrt(m.slowPower);
                meter.inputDb[t]=db(env);
                double prev=m.noisePower;
                if(prev<=0)prev=std::max(1e-12,m.slowPower);
                double a=m.slowPower<prev?noiseDown:(noiseSamples<rate*.6?noiseStartup:noiseUp);
                m.noisePower=std::max(1e-12,prev+a*(m.slowPower-prev));
                if(isAuto(t)) {
                    double weighted;
                    if(c.smart) {
                        double activity=std::max(0.,.65*m.slowPower+.35*m.fastPower-1.6*m.noisePower);
                        weighted=std::sqrt(activity+.001*m.slowPower)*ampDb(c.weightDb[t]);
                    } else weighted=env*ampDb(c.weightDb[t]);
                    m.score=std::pow(std::max(0.,weighted),2.*strength);
                    int g=std::clamp(c.group[t],0,2);
                    groupSum[g]+=m.score;groupCount[g]++;
                } else m.score=0;
            }
            for(int g=0;g<3;++g) {
                if(groupCount[g]>0) {
                    groupTarget[g]=keys[g].score>0 ? groupSum[g]/std::max(eps,groupSum[g]+keys[g].score):1.;
                    if(keys[g].score<=0 && groupSum[g]<1e-27)groupTarget[g]=1;
                } else groupTarget[g]=0;
            }
            for(int t=0;t<count;++t) {
                int g=std::clamp(c.group[t],0,2); auto& m=mics[t];
                double target=1.;
                if(activeEngine && c.mode[t]!=ChannelMode::bypass) {
                    if(c.groupMute[g]) target=0;
                    else if(c.groupOverride[g]) target=c.overrideEnabled[t]?1.:0.;
                    else if(c.mode[t]==ChannelMode::mute) target=0;
                    else if(c.mode[t]==ChannelMode::manual) target=1.;
                    else if(groupSum[g]<1e-27&&keys[g].score<=0) target=1./std::sqrt(double(std::max(1,groupCount[g])));
                    else target=std::sqrt(m.score/std::max(eps,groupSum[g]+keys[g].score));
                }
                m.target=float(clamp01(target));
            }
        }
        // 4) Smooth per channel; NORMALIZE AUTO only, not manual/bypass.
        std::array<double,3> groupPower{};
        for(int t=0;t<count;++t) {
            auto& m=mics[t];double a=m.target>m.gain?attackCoeff:releaseCoeff;
            m.gain=float(m.target+a*(m.gain-m.target));
            if(isAuto(t))groupPower[std::clamp(c.group[t],0,2)]+=double(m.gain)*m.gain;
        }
        for(int g=0;g<3;++g) {
            groupGoalSmooth[g]+=(groupTarget[g]-groupGoalSmooth[g])*refSmooth;
            if(c.smart)groupNorm[g]=groupPower[g]>1?1./std::sqrt(groupPower[g]):1.;
            else groupNorm[g]=groupPower[g]>tiny ? std::sqrt(std::max(0.,groupGoalSmooth[g])/groupPower[g]):1.;
            // This is a diagnostic estimate of extra attenuation from KEY.
            meter.keyCutDb[g] = (c.keyMaster&&c.keyEnabled[g]&&groupCount[g]>0) ? db(std::sqrt(std::max(0.,groupGoalSmooth[g]))) : 0.f;
        }
        // 5) Audio path: lookahead delay and individual stems, optionally SUM.
        double totalL=0,totalR=0;
        for(int t=0;t<count;++t) {
            auto& m=mics[t]; double l=m.inL,r=m.inR;
            if(delay>0) {
                const auto base=std::size_t(t*2)*maxDelay+delayPos;
                double dl=delayRing[base],dr=delayRing[base+maxDelay];
                delayRing[base]=float(l);delayRing[base+maxDelay]=float(r);
                l=dl;r=dr;
            }
            double gain=m.gain;
            if(isAuto(t))gain*=groupNorm[std::clamp(c.group[t],0,2)];
            gain=clamp01(gain);
            if(fadeRemaining>0) {
                m.finalGain=float(m.finalGain+algorithmFade*(gain-m.finalGain));
                gain=m.finalGain;
            } else m.finalGain=float(gain);
            meter.gainDb[t]=db(gain);
            meter.outputDb[t]=meter.inputDb[t]+meter.gainDb[t];
            l*=gain;r*=gain;
            if(c.routing==Routing::sum) {
                totalL+=l;totalR+=r;
                io[frame*channels+2*t]=0;
                io[frame*channels+2*t+1]=0;
            } else {
                io[frame*channels+2*t]=float(l);
                io[frame*channels+2*t+1]=float(r);
            }
        }
        if(c.routing==Routing::sum) {io[frame*channels]=float(totalL);io[frame*channels+1]=float(totalR);}
        // If activeMics < host channels, clear unused outputs (no dry leakage).
        for(int ch=count*2;ch<channels;++ch) io[frame*channels+ch]=0;
        if(fadeRemaining>0)--fadeRemaining;
        if(delay>0 && ++delayPos>=delay)delayPos=0;
    }
}
} // namespace speechshare
