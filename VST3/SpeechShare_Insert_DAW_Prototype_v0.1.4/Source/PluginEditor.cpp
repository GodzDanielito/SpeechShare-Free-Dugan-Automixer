#include "PluginEditor.h"
#include <cmath>
#include <algorithm>

namespace {
using namespace juce;
const auto bg=Colour::fromRGB(13,19,27);
const auto panel=Colour::fromRGB(25,34,46);
const auto edge=Colour::fromRGB(54,72,87);
const auto cyan=Colour::fromRGB(70,177,222);
const auto green=Colour::fromRGB(62,209,117);
const auto muted=Colour::fromRGB(154,170,183);
const auto amber=Colour::fromRGB(249,177,80);
const auto red=Colour::fromRGB(224,82,84);
Colour groupColor(int g) {return g==0?Colour::fromRGB(232,117,98):g==1?green:Colour::fromRGB(91,138,233);}
void configureSlider(Slider& s,Slider::SliderStyle style,Slider::TextEntryBoxPosition position) {
    s.setSliderStyle(style);
    s.setTextBoxStyle(position,false,72,23);
    s.setColour(Slider::trackColourId,cyan);
    s.setColour(Slider::thumbColourId,Colour::fromRGB(217,235,247));
    s.setColour(Slider::textBoxTextColourId,Colours::white);
    s.setColour(Slider::textBoxBackgroundColourId,Colour::fromRGB(17,27,37));
    s.setColour(Slider::textBoxOutlineColourId,edge);
    s.setColour(Slider::backgroundColourId,Colour::fromRGB(49,67,83));
}
void pane(Graphics& g,Rectangle<int> r) {
    g.setColour(panel);g.fillRoundedRectangle(r.toFloat(),9);
    g.setColour(edge);g.drawRoundedRectangle(r.toFloat().reduced(.5f),9,1.f);
}
void caption(Graphics& g,const String& text,Rectangle<int> r,int size=12,Colour colour=muted) {
    g.setColour(colour);g.setFont(Font(float(size)));
    g.drawText(text,r,Justification::centred,false);
}
void modeColour(TextButton& b,bool selected,Colour c) {
    b.setColour(TextButton::buttonColourId,selected?c:Colour::fromRGB(39,53,67));
    b.setColour(TextButton::textColourOffId,selected?bg:Colours::white);
}
void setChoice(AudioProcessorValueTreeState& state,const String& name,int next) {
    if(auto* p=state.getParameter(name)) {
        p->beginChangeGesture();p->setValueNotifyingHost(p->convertTo0to1(float(next)));p->endChangeGesture();
    }
}
} // namespace

SpeechShareEditor::MicStrip::MicStrip(SpeechShareProcessor& p):processor(p) {
    addAndMakeVisible(weight);configureSlider(weight,juce::Slider::LinearVertical,juce::Slider::TextBoxBelow);
    weight.setDoubleClickReturnValue(true,0.);
    group.addItemList({"A","B","C"},1);addAndMakeVisible(group);
    addAndMakeVisible(overrideButton);
    for(auto* b:{&man,&autoButton,&mute,&bypass})addAndMakeVisible(*b);
    man.onClick=[this]{mode(0);};autoButton.onClick=[this]{mode(1);};
    mute.onClick=[this]{mode(2);};bypass.onClick=[this]{mode(3);};
    setIndex(0);
}
void SpeechShareEditor::MicStrip::mode(int next) {
    setChoice(processor.state,SpeechShareProcessor::mid(index,"mode"),next);
    refresh();
}
void SpeechShareEditor::MicStrip::setIndex(int i) {
    index=i;
    weightAttach.reset();groupAttach.reset();overrideAttach.reset();
    weightAttach=std::make_unique<SliderAttachment>(processor.state,SpeechShareProcessor::mid(i,"weight"),weight);
    groupAttach=std::make_unique<ComboAttachment>(processor.state,SpeechShareProcessor::mid(i,"group"),group);
    overrideAttach=std::make_unique<ButtonAttachment>(processor.state,SpeechShareProcessor::mid(i,"override"),overrideButton);
    refresh();repaint();
}
void SpeechShareEditor::MicStrip::refresh() {
    auto* p=processor.state.getRawParameterValue(SpeechShareProcessor::mid(index,"mode"));
    const int modeIndex=p?int(p->load()):1;
    modeColour(man,modeIndex==0,amber);modeColour(autoButton,modeIndex==1,green);
    modeColour(mute,modeIndex==2,red);modeColour(bypass,modeIndex==3,amber);
    repaint();
}
void SpeechShareEditor::MicStrip::resized() {
    const auto w=getWidth(),h=getHeight();
    const int grip=std::max(80,h-226);
    weight.setBounds(10,220,w-20,grip-220);
    group.setBounds(10,h-209,w-20,27);
    overrideButton.setBounds(10,h-176,w-20,23);
    bypass.setBounds(10,185,w-20,27);
    man.setBounds(10,h-144,w-20,26);
    autoButton.setBounds(10,h-112,w-20,26);
    mute.setBounds(10,h-80,w-20,26);
}
void SpeechShareEditor::MicStrip::paint(juce::Graphics& g) {
    pane(g,getLocalBounds().reduced(1));
    const auto w=getWidth();
    caption(g,"MIC "+juce::String(index+1).paddedLeft('0',2),{6,8,w-12,25},14,Colours::white);
    const int kind=int(processor.state.getRawParameterValue("meter")->load());
    const auto v=processor.micMeter(index,kind);
    const auto input=processor.micMeter(index,1);
    caption(g,kind==0?"GAIN dB":kind==1?"INPUT dB":"OUTPUT dB",{5,38,w-10,19},10,muted);
    const int bx=18, by=62,bw=w-36,bh=105;
    g.setColour(bg);g.fillRoundedRectangle(float(bx),float(by),float(bw),float(bh),4.f);
    const double norm=kind==0?std::clamp((double(v)+60)/60.,0.,1.):std::clamp((double(v)+72)/72.,0.,1.);
    auto bar=(int)std::lround(norm*(bh-9));
    g.setColour(kind==0?green:cyan);
    g.fillRoundedRectangle(float(bx+6),float(by+bh-5-bar),float(bw-12),float(bar),2.f);
    g.setColour(Colours::white);g.setFont(juce::Font(12.f));
    g.drawText(juce::String(v,1)+" dB",juce::Rectangle<int>{6,166,w-12,18},juce::Justification::centred);
    caption(g,"WEIGHT",{4,211,w-8,14},10);
    caption(g,"GROUP",{5,getHeight()-231,w-10,17},10);
    caption(g,"SOURCE / OVERRIDE",{5,getHeight()-155,w-10,12},9);
    // Thin activity LED based on unprocessed signal
    g.setColour(input>-52?green:edge);g.fillEllipse(float(w-16),13.f,6.f,6.f);
}

SpeechShareEditor::KeyStrip::KeyStrip(SpeechShareProcessor& p,int group):processor(p),groupNumber(group) {
    type.addItemList({"Pink noise","Sine 40 Hz","Sine 1 kHz"},1);
    addAndMakeVisible(type);
    addAndMakeVisible(level);
    configureSlider(level,juce::Slider::LinearVertical,juce::Slider::TextBoxBelow);
    addAndMakeVisible(enabled);
    typeAttach=std::make_unique<ComboAttachment>(processor.state,SpeechShareProcessor::kid(group,"type"),type);
    levelAttach=std::make_unique<SliderAttachment>(processor.state,SpeechShareProcessor::kid(group,"db"),level);
    enabledAttach=std::make_unique<ButtonAttachment>(processor.state,SpeechShareProcessor::kid(group,"enabled"),enabled);
}
void SpeechShareEditor::KeyStrip::resized() {
    int w=getWidth(),h=getHeight();
    type.setBounds(10,92,w-20,30);
    enabled.setBounds(12,135,w-24,30);
    level.setBounds(10,185,w-20,std::max(85,h-300));
}
void SpeechShareEditor::KeyStrip::paint(juce::Graphics& g) {
    pane(g,getLocalBounds().reduced(1));
    g.setColour(groupColor(groupNumber));g.fillRoundedRectangle(10.f,10.f,float(getWidth()-20),5.f,2.f);
    caption(g,"KEY "+juce::String::charToString('A'+groupNumber),{6,24,getWidth()-12,32},17,Colours::white);
    caption(g,"VIRTUAL DETECTOR",{3,58,getWidth()-6,22},11);
    caption(g,"LEVEL (RMS dBFS)",{7,171,getWidth()-14,14},10);
    auto value=processor.keyMeter(groupNumber);
    auto cut=processor.keyCut(groupNumber);
    const int y=getHeight()-102;
    g.setColour(bg);g.fillRoundedRectangle(11.f,float(y),float(getWidth()-22),84.f,5.f);
    caption(g,"AFTER HPF",{12,y+5,getWidth()-24,18},11);
    caption(g,juce::String(value,1)+" dBFS",{12,y+22,getWidth()-24,25},15,cyan);
    caption(g,"REF CUT " + juce::String(cut,1)+" dB",{12,y+52,getWidth()-24,23},12,groupColor(groupNumber));
}

SpeechShareEditor::MasterPane::MasterPane(SpeechShareProcessor& p):processor(p) {
    for(auto* b:{&engine,&keyMaster,&smart})addAndMakeVisible(*b);
    engineAttach=std::make_unique<ButtonAttachment>(processor.state,"engine",engine);
    keyAttach=std::make_unique<ButtonAttachment>(processor.state,"key_master",keyMaster);
    smartAttach=std::make_unique<ButtonAttachment>(processor.state,"smart",smart);
    addAndMakeVisible(meterButton);
    meterButton.onClick=[this]{
        int next=(int(processor.state.getRawParameterValue("meter")->load())+1)%3;
        setChoice(processor.state,"meter",next);refreshed();
    };
    routing.addItemList({"Multitrack","Summed stereo","Bypass"},1);
    addAndMakeVisible(routing);
    routingAttach=std::make_unique<ComboAttachment>(processor.state,"routing",routing);
    addAndMakeVisible(micCount);configureSlider(micCount,juce::Slider::LinearHorizontal,juce::Slider::TextBoxRight);
    countAttach=std::make_unique<SliderAttachment>(processor.state,"mic_count",micCount);
    const char* ids[]={"rms","strength","attack","release","lookahead","hpf"};
    const char* names[]={"RMS (ms)","Strength","Attack (ms)","Release (ms)","Lookahead (ms)","HPF (Hz)"};
    for(int i=0;i<6;++i) {
        advancedLabels[i].setText(names[i],juce::dontSendNotification);
        advancedLabels[i].setFont(juce::Font(12.f));
        advancedLabels[i].setColour(juce::Label::textColourId,muted);
        addAndMakeVisible(advancedLabels[i]);
        configureSlider(advanced[i],juce::Slider::LinearHorizontal,juce::Slider::TextBoxRight);
        addAndMakeVisible(advanced[i]);
        advancedAttach[i]=std::make_unique<SliderAttachment>(processor.state,ids[i],advanced[i]);
    }
    for(int i=0;i<3;++i) {
        groupOverride[i].setButtonText("OVERRIDE");groupMute[i].setButtonText("MUTE");
        addAndMakeVisible(groupOverride[i]);addAndMakeVisible(groupMute[i]);
        overrideAttach[i]=std::make_unique<ButtonAttachment>(processor.state,SpeechShareProcessor::gid(i,"override"),groupOverride[i]);
        muteAttach[i]=std::make_unique<ButtonAttachment>(processor.state,SpeechShareProcessor::gid(i,"mute"),groupMute[i]);
    }
    refreshed();
}
void SpeechShareEditor::MasterPane::refreshed() {
    const auto selected=int(processor.state.getRawParameterValue("meter")->load());
    meterButton.setButtonText("METER: "+juce::String(selected==0?"GAIN":selected==1?"INPUT":"OUTPUT"));
    repaint();
}
void SpeechShareEditor::MasterPane::resized() {
    const int w=getWidth(),half=(w-34)/2;
    engine.setBounds(12,42,half,31);keyMaster.setBounds(20+half,42,half,31);
    smart.setBounds(12,80,half,31);meterButton.setBounds(20+half,80,half,31);
    for(int i=0;i<6;++i) {
        int y=151+i*42;
        advancedLabels[i].setBounds(12,y,103,30);
        advanced[i].setBounds(109,y+1,w-122,28);
    }
    routing.setBounds(12,422,w-24,30);
    micCount.setBounds(109,466,w-122,32);
    for(int g=0;g<3;++g) {
        const int y=540+g*36;
        groupOverride[g].setBounds(78,y,130,28);
        groupMute[g].setBounds(210,y,w-222,28);
    }
}
void SpeechShareEditor::MasterPane::paint(juce::Graphics& g) {
    pane(g,getLocalBounds().reduced(1));
    caption(g,"MASTER / CONTROL",{5,8,getWidth()-10,27},18,Colours::white);
    caption(g,"DETECTOR & DYNAMICS",{11,119,getWidth()-22,25},12,cyan);
    caption(g,"OUTPUT MODE",{11,398,getWidth()-22,19},11,muted);
    caption(g,"ACTIVE MICROPHONES",{11,463,100,34},11);
    caption(g,"GROUP LOGIC",{11,506,getWidth()-22,25},12,cyan);
    for(int i=0;i<3;++i)caption(g,"GROUP "+juce::String::charToString('A'+i),{9,546+i*36,66,20},11,groupColor(i));
}

SpeechShareEditor::SpeechShareEditor(SpeechShareProcessor& p)
    : juce::AudioProcessorEditor(p),processor(p),master(p) {
    setLookAndFeel(&lookAndFeel);
    lookAndFeel.setColour(juce::TextButton::buttonColourId,juce::Colour::fromRGB(40,57,72));
    lookAndFeel.setColour(juce::ToggleButton::textColourId,juce::Colours::white);
    lookAndFeel.setColour(juce::ComboBox::backgroundColourId,juce::Colour::fromRGB(24,40,56));
    lookAndFeel.setColour(juce::ComboBox::textColourId,juce::Colours::white);
    lookAndFeel.setColour(juce::Label::textColourId,juce::Colours::white);
    lookAndFeel.setColour(juce::TextButton::textColourOffId,juce::Colours::white);
    header.setText("SPEECHSHARE",juce::dontSendNotification);
    header.setFont(juce::Font(25.f,juce::Font::bold));
    header.setColour(juce::Label::textColourId,juce::Colours::white);addAndMakeVisible(header);
    subtitle.setText("SMART AUTOMIXER  /  32 MIC  /  THREE VIRTUAL KEY GROUPS",juce::dontSendNotification);
    subtitle.setColour(juce::Label::textColourId,muted);addAndMakeVisible(subtitle);
    for(int i=0;i<4;++i) {
        pages[i].setButtonText("MIC "+juce::String(i*8+1)+"–"+juce::String(i*8+8));
        pages[i].onClick=[this,i]{changePage(i);}; addAndMakeVisible(pages[i]);
    }
    for(auto& strip:mics){strip=std::make_unique<MicStrip>(processor);addAndMakeVisible(*strip);}
    for(int g=0;g<3;++g){keys[g]=std::make_unique<KeyStrip>(processor,g);addAndMakeVisible(*keys[g]);}
    addAndMakeVisible(master);
    changePage(0);
    setResizable(true,true);
    setResizeLimits(1500,740,2800,1400);
    setSize(1770,790);
    startTimerHz(25);
}
SpeechShareEditor::~SpeechShareEditor() {stopTimer();setLookAndFeel(nullptr);}
void SpeechShareEditor::changePage(int newPage) {
    page=std::clamp(newPage,0,3);
    for(int i=0;i<8;++i)mics[i]->setIndex(page*8+i);
    for(int i=0;i<4;++i)modeColour(pages[i],i==page,cyan);
}
void SpeechShareEditor::timerCallback() {
    for(auto& strip:mics)strip->refresh();
    for(auto& strip:keys)strip->repaint();
    master.refreshed();
}
void SpeechShareEditor::paint(juce::Graphics& g) {
    g.fillAll(bg);
    g.setColour(panel);g.fillRect(0,0,getWidth(),52);
    g.setColour(cyan);g.fillRect(0,49,getWidth(),3);
    g.setColour(edge);g.drawLine(0.f,93.f,float(getWidth()),93.f,1.f);
}
void SpeechShareEditor::resized() {
    int w=getWidth(),h=getHeight();
    header.setBounds(16,7,250,38);
    subtitle.setBounds(270,12,w-300,31);
    for(int i=0;i<4;++i)pages[i].setBounds(15+i*125,59,119,29);
    const int margin=12, gap=7;
    const int bodyY=101,bodyH=h-bodyY-12;
    const int masterW=355,keyW=118;
    const int available=w-2*margin-masterW-3*keyW-(8+3)*gap-14;
    const int micW=std::max(70,available/8);
    int x=margin;
    for(auto& strip:mics){strip->setBounds(x,bodyY,micW,bodyH);x+=micW+gap;}
    x+=7;
    for(auto& strip:keys){strip->setBounds(x,bodyY,keyW,bodyH);x+=keyW+gap;}
    master.setBounds(x+2,bodyY,w-x-margin-2,bodyH);
}
juce::AudioProcessorEditor* SpeechShareProcessor::createEditor() {return new SpeechShareEditor(*this);}
