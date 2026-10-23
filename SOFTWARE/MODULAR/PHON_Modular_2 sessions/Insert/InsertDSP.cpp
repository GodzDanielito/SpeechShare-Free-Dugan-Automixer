#include "InsertDSP.h"
#include <algorithm>
#include <cmath>

namespace speechshare_insert {
namespace {
constexpr double pi=3.14159265358979323846;
double ampDb(double x) noexcept {return std::pow(10.,x*.05);}
double toDb(double x) noexcept {return std::max(-96.,20.*std::log10(std::max(x,1.e-12)));}
double coef(double rate,double ms) noexcept {return std::exp(-1./(rate*std::max(.001,ms*.001)));}
double rise(double rate,double sec) noexcept {return 1.-std::exp(-1./(rate*sec));}
}
void InsertDSP::VirtualKey::tick(double frequencyStep,double amplitude,double coeffHP,
                                  double smoothing,KeyGenerator generator) noexcept {
    constexpr double rootTwo=1.4142135623730951;
    double sample=0.;
    if(generator==KeyGenerator::Pink) {
        random^=random<<13;random^=random>>17;random^=random<<5;
        const double white=(double(random)/4294967295.)*2.-1.;
        b0=.99765*b0+.0990460*white;
        b1=.96300*b1+.2965164*white;
        b2=.57000*b2+1.0526913*white;
        sample=(b0+b1+b2+.1848*white)*.583*amplitude;
    } else {
        phase+=frequencyStep;
        if(phase>=1.)phase-=std::floor(phase);
        sample=std::sin(2.*pi*phase)*rootTwo*amplitude;
    }
    const double filtered=coeffHP>0.?coeffHP*(hpY+sample-hpX):sample;
    hpX=sample;hpY=filtered;
    // Same KEY detector envelope and score exponent as SpeechEngine v1.5.
    detectorPower=std::max(0.,detectorPower+smoothing*(filtered*filtered-detectorPower));
}
void InsertDSP::prepare(double sr) {
    rate=std::max(8000.,sr);
    maxWindow=std::max(1,int(std::ceil(rate*.51)));
    rmsRing.assign(std::size_t(maxWindow),0.f);
    reset();
}
void InsertDSP::reset() {
    std::fill(rmsRing.begin(),rmsRing.end(),0.f);
    powerSum=fastPower=noisePower=0.;
    hpX.fill(0.);hpY.fill(0.);
    rawGain=targetGain=normalization=normalizationTarget=muteEnvelope=1.;
    position=hop=0;window=1;previousSession=previousGroup=-1;
    key=VirtualKey{};keyDbMeter=-96.f;
    meter={};
}
void InsertDSP::process(float* left,float* right,int channels,int frames,const Settings& s,int slot) noexcept {
    if (!left||frames<=0||rmsRing.empty()||!(channels==1||channels==2)) return;
    const int session=std::clamp(s.session,1,maxSessions),group=std::clamp(s.group,0,2);
    const int desiredWindow=std::clamp(int(std::lround(rate*std::clamp(s.rmsMs,10.f,500.f)*.001)),1,maxWindow);
    if (desiredWindow!=window||previousSession!=session||previousGroup!=group) {
        window=desiredWindow;std::fill(rmsRing.begin(),rmsRing.end(),0.f);
        powerSum=0.;position=hop=0;rawGain=targetGain=normalization=normalizationTarget=1.;
        if(previousSession!=session || previousGroup!=group)key=VirtualKey{};
    }
    previousSession=session;previousGroup=group;
    const double hpHz=std::clamp(double(s.detectorHPF),0.,350.);
    const double hpCoeff=hpHz>0?std::exp(-2.*pi*hpHz/rate):0.;
    const double attack=coef(rate,s.attackMs),release=coef(rate,s.releaseMs);
    const double fastRise=rise(rate,.009),fastFall=rise(rate,.060);
    const double normSmoothing=rise(rate,.045);
    const double muteSmoothing=rise(rate,.003); // 3 ms click protection
    const double strength=std::clamp(double(s.strength),.5,3.);
    auto& pool=SharedPool::instance();
    const auto sessionKeys=pool.getSessionKeys(session);
    const auto keySettings=sessionKeys.groups[std::size_t(group)];
    const bool keyEnabled=sessionKeys.master && keySettings.enabled;
    const double keyAmplitude=ampDb(keySettings.levelDb);
    const double keyFrequencyStep=std::min(double(keySettings.frequencyHz),rate*.45)/rate;
    const double keySmoothing=rise(rate,std::max(.015,double(s.rmsMs)*.001));
    double keyScore=0.;
    for (int f=0;f<frames;++f) {
        // Detector-only virtual KEY, generated locally in every participant.
        // One key score is added per participant's shared denominator; unlike
        // a physical input there is no additional audio channel or shared slot.
        key.tick(keyFrequencyStep,keyAmplitude,hpCoeff,keySmoothing,keySettings.generator);
        double x0=left[f];double x1=channels==2&&right?right[f]:0.;
        double y0=hpHz>0?hpCoeff*(hpY[0]+x0-hpX[0]):x0;
        double y1=hpHz>0?hpCoeff*(hpY[1]+x1-hpX[1]):x1;
        hpX[0]=x0;hpX[1]=x1;hpY[0]=y0;hpY[1]=y1;
        const double power=(y0*y0+(channels==2?y1*y1:0.))/(channels==2?2.:1.);
        fastPower=std::max(0.,fastPower+(power>fastPower?fastRise:fastFall)*(power-fastPower));
        float& old=rmsRing[std::size_t(position)];
        powerSum=std::max(0.,powerSum+power-double(old));old=float(power);
        if (++position>=window) position=0;
        if (++hop>=64) {
            hop=0;
            keyScore=keyEnabled?std::pow(std::max(0.,key.detectorPower),strength):0.;
            const double slowPower=std::max(0.,powerSum/window);
            meter.inputDb=float(toDb(std::sqrt(slowPower)));
            double previous=noisePower;
            if (previous<=0.) previous=std::max(1.e-12,slowPower);
            const double down=1.-std::exp(-64./(rate*.35));
            const double up=1.-std::exp(-64./(rate*8.));
            noisePower=std::max(1.e-12,previous+(slowPower<previous?down:up)*(slowPower-previous));
            double score=0.;
            if (s.mode==Mode::Auto && !s.channelMute) {
                const double weighted=s.smart?
                    std::sqrt(std::max(0.,.65*slowPower+.35*fastPower-1.6*noisePower)+.001*slowPower)*ampDb(s.weightDb):
                    std::sqrt(slowPower)*ampDb(s.weightDb);
                score=std::pow(std::max(0.,weighted),2.*strength);
            }
            pool.publish(slot,session,group,s.mode,score,rawGain);
            const MixStats stats=pool.query(slot,session,group);
            // Keys compete for gain but never count as an audible mic.
            const double denominator=stats.scoreSum+keyScore;
            const double groupPowerTarget=keyScore>0. ?
                stats.scoreSum/std::max(1.e-30,denominator):1.;
            if (s.mode==Mode::Mute) targetGain=0.;
            else if (s.mode!=Mode::Auto) targetGain=1.;
            else if(denominator<1.e-27)
                targetGain=1./std::sqrt(double(std::max(1,stats.autoCount)));
            else targetGain=std::sqrt(score/std::max(1.e-30,denominator));
            // Same Classic normalization objective as the central engine, but peers' rawGain
            // values are asynchronous snapshots: NOT guaranteed sample-accurate.
            normalizationTarget=(s.mode==Mode::Auto&&!s.smart&&stats.rawGainPower>1.e-24)
                                ? std::sqrt(std::max(0.,groupPowerTarget)/stats.rawGainPower):
                                  (keyScore>0. && groupPowerTarget<=0.?0.:1.);
        }
        const double a=targetGain>rawGain?attack:release;
        rawGain=targetGain+a*(rawGain-targetGain);
        normalization+=(normalizationTarget-normalization)*normSmoothing;
        // Local mute is a separate, fast output gate. It overrides BYPASS
        // and is never treated as a participant by SharedPool.
        muteEnvelope+=((s.channelMute?0.:1.)-muteEnvelope)*muteSmoothing;
        const double baseGain=s.mode==Mode::Bypass?1.:
            std::clamp(rawGain*(s.mode==Mode::Auto?normalization:1.),0.,1.);
        const double g=baseGain*muteEnvelope;
        left[f]=float(x0*g);
        if (channels==2&&right) right[f]=float(x1*g);
        meter.gainDb=float(toDb(g));
        meter.outputDb=meter.inputDb+meter.gainDb;
    }
    keyDbMeter=float(toDb(std::sqrt(std::max(0.,key.detectorPower))));
}
}
