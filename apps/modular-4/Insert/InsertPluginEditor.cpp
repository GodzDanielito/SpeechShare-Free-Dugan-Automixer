#include "InsertPluginEditor.h"
#include "PresentationPolicy.h"
#include "PhonMacro.h"
#include <BinaryData.h>
#include <algorithm>
#include <cmath>

namespace {
using namespace juce;
// Slate metal, broadcast-status LEDs and neutral text. No external fonts/assets.
const Colour backdrop=Colour::fromRGB(9,13,20);
const Colour panel=Colour::fromRGB(25,35,49);
const Colour darkPanel=Colour::fromRGB(13,21,31);
const Colour border=Colour::fromRGB(61,82,102);
const Colour white=Colour::fromRGB(242,247,251);
const Colour secondary=Colour::fromRGB(155,176,192);
const Colour cyan=Colour::fromRGB(46,203,244);
const Colour green=Colour::fromRGB(54,223,148);
const Colour amber=Colour::fromRGB(255,190,85);
const Colour red=Colour::fromRGB(255,94,105);
const Colour rim=Colour::fromRGB(98,127,152);
const Colour meterWell=Colour::fromRGB(6,13,21);
const Colour groupTint[3]={green,Colour::fromRGB(92,145,250),amber};
constexpr int stripGap=5;
constexpr int minimumStripWidth=78;
constexpr int maximumStripWidth=220;
constexpr int rightContentHeight=405;

void labelStyle(Label& l,float size,bool bold=false,Colour c=white) {
    l.setFont(Font(FontOptions(size,bold?Font::bold:Font::plain)));
    l.setColour(Label::textColourId,c);
    l.setJustificationType(Justification::centredLeft);
}
void setButton(TextButton& b,bool active,Colour activeColour=cyan) {
    b.setColour(TextButton::buttonColourId,active?activeColour:Colour::fromRGB(42,55,70));
    b.setColour(TextButton::textColourOffId,active?backdrop:white);
}
void setGroupButton(TextButton& b,bool active,Colour c) {
    b.setColour(TextButton::buttonColourId,active?c:Colour::fromRGB(34,45,58));
    b.setColour(TextButton::textColourOffId,active?backdrop:white.withAlpha(.92f));
}
void configureSlider(Slider& s,Slider::SliderStyle style,Slider::TextEntryBoxPosition pos,int textW=56) {
    s.setSliderStyle(style);
    s.setTextBoxStyle(pos,false,textW,22);
    s.setColour(Slider::backgroundColourId,Colour::fromRGB(54,67,82));
    s.setColour(Slider::trackColourId,cyan);
    s.setColour(Slider::thumbColourId,white);
    s.setColour(Slider::textBoxTextColourId,white);
    s.setColour(Slider::textBoxBackgroundColourId,darkPanel);
    s.setColour(Slider::textBoxOutlineColourId,border);
}
void framed(Graphics& g,Rectangle<float> r,float radius=9.f) {
    g.setGradientFill(ColourGradient(panel.brighter(.075f),r.getX(),r.getY(),
                                     darkPanel.brighter(.04f),r.getX(),r.getBottom(),false));
    g.fillRoundedRectangle(r,radius);
    g.setColour(border.withAlpha(.9f));g.drawRoundedRectangle(r.reduced(.5f),radius,1.f);
    g.setColour(Colours::white.withAlpha(.042f));
    g.drawHorizontalLine(int(r.getY())+1,r.getX()+radius,r.getRight()-radius);
}
void statusLed(Graphics& g,float x,float y,Colour tint,bool active=true) {
    const auto glow=tint.withAlpha(active?.24f:.035f);
    g.setColour(glow);g.fillEllipse(x-3.f,y-3.f,12.f,12.f);
    g.setColour(active?tint:Colour::fromRGB(68,81,96));
    g.fillEllipse(x,y,6.f,6.f);
    g.setColour(Colours::white.withAlpha(active?.60f:.13f));
    g.fillEllipse(x+1.f,y+.7f,2.f,2.f);
}
void smallText(Graphics& g,String text,Rectangle<int> bounds,float size,Colour c=secondary,
               Justification align=Justification::centred) {
    g.setColour(c);g.setFont(Font(FontOptions(size)));
    g.drawText(text,bounds,align,false);
}
String number(float x) {return x<=-95.f?"-INF":String(speechshare_insert::canonicalDisplayDb(x),1);}
float dBToMeter(float db) {return jlimit(0.f,1.f,(db+72.f)/72.f);}
}

class SpeechShareInsertEditor::BroadcastLookAndFeel final : public juce::LookAndFeel_V4 {
public:
    BroadcastLookAndFeel() {
        setColour(ComboBox::backgroundColourId,Colour::fromRGB(21,33,47));
        setColour(ComboBox::outlineColourId,border);
        setColour(ComboBox::textColourId,white);
        setColour(ComboBox::arrowColourId,cyan);
        setColour(PopupMenu::backgroundColourId,panel);
        setColour(PopupMenu::textColourId,white);
        setColour(TextEditor::backgroundColourId,Colour::fromRGB(11,20,29));
        setColour(TextEditor::textColourId,white);
        setColour(TextEditor::outlineColourId,border);
        setColour(TextEditor::focusedOutlineColourId,cyan);
        setColour(TextEditor::highlightColourId,cyan.withAlpha(.38f));
        setColour(ToggleButton::textColourId,white);
        setColour(ScrollBar::thumbColourId,Colour::fromRGB(68,91,109));
        setColour(Label::textColourId,white);
    }
    Font getTextButtonFont(TextButton& b,int buttonHeight) override {
        // Fixed-size mode legends MUST fit on a narrow 8-channel page.
        const bool channelMode=bool(b.getProperties().getWithDefault("SpeechShareModeButton", false));
        const float size=channelMode ? 10.f : float(juce::jlimit(9,12,buttonHeight/2));
        return Font(FontOptions(size, Font::bold));
    }
    Font getComboBoxFont(ComboBox&) override { return Font(FontOptions(10.f)); }
    void drawButtonBackground(Graphics& g,Button& b,const Colour& base,bool hovered,bool down) override {
        const auto r=b.getLocalBounds().toFloat().reduced(.8f);
        const bool illuminated = base==cyan || base==green || base==red || base==amber || base==groupTint[1];
        if(illuminated){
            g.setColour(base.withAlpha(hovered?.27f:.17f));
            g.fillRoundedRectangle(r.expanded(1.f),5.f);
        }
        auto top=illuminated?base.brighter(.13f):Colour::fromRGB(54,67,79);
        auto bottom=illuminated?base.darker(.25f):Colour::fromRGB(21,31,44);
        if(hovered){top=top.brighter(.12f);bottom=bottom.brighter(.12f);}
        if(down){top=top.darker(.23f);bottom=bottom.darker(.18f);}
        g.setColour(Colour::fromRGB(4,9,14));
        g.fillRoundedRectangle(r.expanded(1.f),4.f);
        g.setGradientFill(ColourGradient(top,r.getX(),r.getY(),bottom,r.getX(),r.getBottom(),false));
        g.fillRoundedRectangle(r,3.f);
        g.setColour((illuminated?base.brighter(.32f):border).withAlpha(illuminated?.9f:.72f));
        g.drawRoundedRectangle(r.reduced(.4f),3.f,1.f);
        g.setColour(Colours::white.withAlpha(illuminated?.26f:.10f));
        g.drawHorizontalLine(int(r.getY())+2,r.getX()+4.f,r.getRight()-4.f);
    }
    void drawRotarySlider(Graphics& g,int x,int y,int width,int height,float position,
                          float startAngle,float endAngle,Slider& slider) override {
        if (bool(slider.getProperties().getWithDefault("SpeechSharePHON",false))) {
            const auto bounds=Rectangle<float>(float(x),float(y),float(width),float(height));
            const float size=juce::jmax(30.f,juce::jmin(bounds.getWidth(),bounds.getHeight())-10.f);
            const float radius=size*.5f;
            const auto cx=bounds.getCentreX(),cy=bounds.getCentreY();
            const float arcRadius=radius*.87f;
            juce::Path base,active;
            base.addCentredArc(cx,cy,arcRadius,arcRadius,0.f,startAngle,endAngle,true);
            const float angle=startAngle+position*(endAngle-startAngle);
            active.addCentredArc(cx,cy,arcRadius,arcRadius,0.f,startAngle,angle,true);
            g.setColour(juce::Colour::fromRGB(45,60,75));
            g.strokePath(base,juce::PathStrokeType(7.f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
            const auto accent=juce::Colour::fromRGB(39,189,237).interpolatedWith(juce::Colour::fromRGB(249,165,74),position);
            g.setColour(accent.withAlpha(.25f));
            g.strokePath(active,juce::PathStrokeType(10.f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
            g.setColour(accent);
            g.strokePath(active,juce::PathStrokeType(5.f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
            for(int i=0;i<=20;++i){
                const float a=startAngle+float(i)/20.f*(endAngle-startAngle);
                const float rr=radius*.98f;
                const auto x1=cx+std::sin(a)*rr,y1=cy-std::cos(a)*rr;
                const auto x2=cx+std::sin(a)*(rr+(i%5==0?6.f:3.f));
                const auto y2=cy-std::cos(a)*(rr+(i%5==0?6.f:3.f));
                g.setColour(juce::Colour::fromRGB(102,131,152));
                g.drawLine(x1,y1,x2,y2,i%5==0?1.4f:1.f);
            }
            const auto dial=Rectangle<float>(cx-radius*.67f,cy-radius*.67f,radius*1.34f,radius*1.34f);
            g.setGradientFill(ColourGradient(Colour::fromRGB(114,139,155),dial.getX(),dial.getY(),
                                             Colour::fromRGB(28,41,55),dial.getRight(),dial.getBottom(),false));
            g.fillEllipse(dial);
            g.setColour(Colour::fromRGB(6,13,23));g.drawEllipse(dial,2.f);
            const auto indicatorR=radius*.49f;
            const auto ix=cx+std::sin(angle)*indicatorR,iy=cy-std::cos(angle)*indicatorR;
            g.setColour(accent);g.fillEllipse(ix-3.5f,iy-3.5f,7.f,7.f);
            g.setColour(white.withAlpha(.82f));
            g.setFont(Font(FontOptions(13.f,Font::bold)));
            g.drawText("PHON",Rectangle<int>(int(cx-35.f),int(cy-11.f),70,22),Justification::centred,false);
            return;
        }
        auto bounds=Rectangle<float>(float(x),float(y),float(width),float(height));
        const float dia=jmax(14.f,jmin(bounds.getWidth()-7.f,bounds.getHeight()-7.f));
        const float radius=dia*.5f;
        const auto cx=bounds.getCentreX(),cy=bounds.getCentreY();
        const auto rc=radius-3.f;
        Path base,active;
        base.addCentredArc(cx,cy,rc,rc,0.f,startAngle,endAngle,true);
        active.addCentredArc(cx,cy,rc,rc,0.f,startAngle,startAngle+position*(endAngle-startAngle),true);
        g.setColour(Colour::fromRGB(51,70,84));
        g.strokePath(base,PathStrokeType(3.f,PathStrokeType::curved,PathStrokeType::rounded));
        g.setColour(cyan);
        g.strokePath(active,PathStrokeType(3.3f,PathStrokeType::curved,PathStrokeType::rounded));
        const auto knob=Rectangle<float>(cx-radius*.56f,cy-radius*.56f,radius*1.12f,radius*1.12f);
        g.setGradientFill(ColourGradient(Colour::fromRGB(103,126,148),knob.getX(),knob.getY(),
                                         Colour::fromRGB(38,55,69),knob.getRight(),knob.getBottom(),false));
        g.fillEllipse(knob);
        g.setColour(rim);g.drawEllipse(knob,.9f);
        const auto angle=startAngle+position*(endAngle-startAngle);
        const auto dotX=cx+std::sin(angle)*radius*.43f;
        const auto dotY=cy-std::cos(angle)*radius*.43f;
        g.setColour(white);g.fillEllipse(dotX-1.9f,dotY-1.9f,3.8f,3.8f);
        juce::ignoreUnused(slider);
    }
    void drawLinearSlider(Graphics& g,int x,int y,int width,int height,float sliderPos,
                          float minPos,float maxPos,const Slider::SliderStyle style,Slider& slider) override {
        juce::ignoreUnused(minPos,maxPos);
        if(style!=Slider::LinearHorizontal && style!=Slider::LinearVertical) {
            LookAndFeel_V4::drawLinearSlider(g,x,y,width,height,sliderPos,minPos,maxPos,style,slider);
            return;
        }
        const bool vertical=style==Slider::LinearVertical;
        const bool weightFader=bool(slider.getProperties().getWithDefault("SpeechShareWeight",false));
        if(vertical){
            const float cx=float(x)+float(width)*.5f;
            auto groove=Rectangle<float>(cx-4.f,float(y)+4.f,8.f,float(std::max(3,height-8)));
            g.setColour(Colour::fromRGB(2,7,12));g.fillRoundedRectangle(groove.expanded(1.f),3.f);
            g.setGradientFill(ColourGradient(Colour::fromRGB(45,61,72),groove.getX(),groove.getY(),
                                             Colour::fromRGB(13,23,31),groove.getRight(),groove.getBottom(),false));
            g.fillRoundedRectangle(groove,2.f);
            g.setColour(cyan.withAlpha(.8f));
            g.drawVerticalLine(int(cx),sliderPos,groove.getBottom());
            if(weightFader){
                g.setColour(white.withAlpha(.45f));
                for(int t=0;t<=8;++t){
                    const float yy=groove.getY()+float(t)*groove.getHeight()/8.f;
                    const float extent=(t%2==0)?9.f:5.f;
                    g.drawHorizontalLine(int(yy),cx+11.f,cx+11.f+extent);
                }
            }
            const float capW=weightFader?std::clamp(float(width)*.62f,19.f,36.f):18.f;
            auto cap=Rectangle<float>(cx-capW*.5f,sliderPos-6.f,capW,12.f);
            g.setColour(Colour::fromRGB(1,7,13));g.fillRoundedRectangle(cap.expanded(1.5f),3.f);
            g.setGradientFill(ColourGradient(Colour::fromRGB(238,244,248),cap.getX(),cap.getY(),
                                             Colour::fromRGB(104,128,148),cap.getX(),cap.getBottom(),false));
            g.fillRoundedRectangle(cap,2.5f);
            g.setColour(Colour::fromRGB(46,67,85));g.drawRoundedRectangle(cap,2.5f,.8f);
            g.setColour(cyan.darker(.23f));g.drawHorizontalLine(int(sliderPos),cap.getX()+3.f,cap.getRight()-3.f);
            g.setColour(Colours::white.withAlpha(.65f));g.drawHorizontalLine(int(cap.getY()+2.f),cap.getX()+3.f,cap.getRight()-3.f);
        } else {
            auto track=Rectangle<float>(float(x)+6.f,float(y)+float(height)*.5f-2.f,float(std::max(2,width-12)),4.f);
            g.setColour(Colour::fromRGB(3,10,17));g.fillRoundedRectangle(track.expanded(1.f),2.f);
            g.setColour(Colour::fromRGB(46,66,82));g.fillRoundedRectangle(track,2.f);
            auto lit=track;lit.setRight(jlimit(track.getX(),track.getRight(),sliderPos));
            if(lit.getWidth()>0.f){g.setColour(cyan);g.fillRoundedRectangle(lit,2.f);}
            g.setColour(cyan.withAlpha(.20f));g.fillEllipse(sliderPos-8.f,float(y)+float(height)*.5f-8.f,16.f,16.f);
            g.setColour(white);g.fillEllipse(sliderPos-4.5f,float(y)+float(height)*.5f-4.5f,9.f,9.f);
            g.setColour(border);g.drawEllipse(sliderPos-4.5f,float(y)+float(height)*.5f-4.5f,9.f,9.f,.8f);
        }
    }

};

// Compact controls remain visible at the default editor size. Viewports are
// only a fallback when the host window is resized below the recommended footprint.
class SpeechShareInsertEditor::RightPanel final : public juce::Component {
public:
    void paint(juce::Graphics& g) override {
        using namespace juce;
        const float w=float(getWidth()), h=float(getHeight());
        const float sy=std::max(1.f,h/415.f);
        g.setGradientFill(ColourGradient(Colour::fromRGB(24,35,48),0.f,0.f,Colour::fromRGB(10,18,28),0.f,h,false));
        g.fillAll();
        auto card=[&](float y,float bh,Colour accent){
            auto r=Rectangle<float>(6.f,y,w-12.f,bh);
            g.setColour(Colour::fromRGB(3,9,17));g.fillRoundedRectangle(r.expanded(1.f),9.f);
            g.setGradientFill(ColourGradient(Colour::fromRGB(34,46,59),r.getX(),r.getY(),
                                             Colour::fromRGB(14,24,35),r.getX(),r.getBottom(),false));
            g.fillRoundedRectangle(r,8.f);
            g.setColour(border.withAlpha(.80f));g.drawRoundedRectangle(r.reduced(.5f),8.f,1.f);
            g.setColour(accent.withAlpha(.90f));g.fillRoundedRectangle(r.getX()+1.f,r.getY()+6.f,4.f,r.getHeight()-12.f,2.f);
            g.setColour(white.withAlpha(.07f));
            g.drawHorizontalLine(int(r.getY()+2.f),r.getX()+12.f,r.getRight()-10.f);
        };
        card(45.f*sy,164.f*sy,groupTint[selectedGroup]);
        card(219.f*sy,std::max(30.f,h-225.f*sy),cyan);
    }
    void setActiveGroup(int group){ selectedGroup=std::clamp(group,0,2);repaint(); }
private:
    int selectedGroup=0;
};

class SpeechShareInsertEditor::ChannelStrip final : public juce::Component {
public:
    ChannelStrip() {
        name.setMultiLine(false); name.setReturnKeyStartsNewLine(false);
        name.setTextToShowWhenEmpty("Source",secondary);name.setSelectAllWhenFocused(true);
        name.setInputRestrictions(36);
        name.onReturnKey=[this]{name.moveKeyboardFocusToSibling(false);commitName();};
        name.onFocusLost=[this]{commitName();};
        addAndMakeVisible(name);
        for(auto* b:{&mute,&solo,&modeManual,&modeAuto,&modeBypass,&extraMode})
            addAndMakeVisible(*b);
        mute.setButtonText("MUTE");solo.setButtonText("SOLO");
        modeManual.setButtonText("MAN");modeAuto.setButtonText("AUTO");
        modeBypass.setButtonText("BYP");extraMode.setButtonText("LEGACY");
        for(auto* b:{&modeManual,&modeAuto,&modeBypass,&extraMode})
            b->getProperties().set("SpeechShareModeButton",true);
        mute.setTooltip("Mute channel and exclude from gain sharing");
        solo.setTooltip("Solo within this SpeechShare session only");
        modeManual.setTooltip("Manual gain, bypass automix decision");
        modeAuto.setTooltip("Join automatic gain sharing");
        modeBypass.setTooltip("Bypass this source's automix processing");
        extraMode.setTooltip("Legacy MUTE mode; separate from the dedicated channel MUTE button");
        mute.onClick=[this]{if(valid())pool().requestMute(view.slot,view.generation,!view.localMute);};
        solo.onClick=[this]{if(valid())pool().requestSolo(view.slot,view.generation,!view.solo);};
        modeManual.onClick=[this]{chooseMode(speechshare_insert::Mode::Manual);};
        modeAuto.onClick=[this]{chooseMode(speechshare_insert::Mode::Auto);};
        modeBypass.onClick=[this]{chooseMode(speechshare_insert::Mode::Bypass);};
        extraMode.onClick=[this]{chooseMode(speechshare_insert::Mode::Mute);};
        weight.setRange(-24.,24.,.1);weight.setValue(0.,dontSendNotification);
        configureSlider(weight,Slider::LinearVertical,Slider::TextBoxBelow,52);
        weight.getProperties().set("SpeechShareWeight",true);
        weight.setTooltip("Dugan weight / sensitivity in dB; double-click to reset to 0");
        weight.setDoubleClickReturnValue(true,0.);
        weight.onValueChange=[this]{if(valid())pool().requestWeight(view.slot,view.generation,float(weight.getValue()));};
        addAndMakeVisible(weight);
        compactMode.addItem("MAN",1);compactMode.addItem("AUTO",2);
        compactMode.addItem("MUTE",3);compactMode.addItem("BYPASS",4);
        compactMode.setTooltip("MODE: Manual / Auto / Legacy Mute / Bypass");
        compactMode.onChange=[this]{if(valid())pool().requestMode(view.slot,view.generation,compactMode.getSelectedId()-1);};
        addAndMakeVisible(compactMode);
        for(int i=0;i<3;++i){
            auto& b=groupButtons[std::size_t(i)];
            b.setButtonText(i==0?"A":(i==1?"B":"C"));
            b.setTooltip("Assign source to group "+String(i==0?"A":(i==1?"B":"C")));
            b.onClick=[this,i]{if(valid())pool().requestGroup(view.slot,view.generation,i);};
            addAndMakeVisible(b);
        }
        setSize(150,480);
    }
    void update(const speechshare_insert::ParticipantView& input,bool own) {
        const bool changed=view.slot!=input.slot || view.generation!=input.generation;
        view=input; isOwn=own;
        if(changed || (!name.hasKeyboardFocus(true) && !name.isMouseButtonDown())) {
            auto user=pool().getDisplayName(view.slot,view.generation);
            if(user.empty())user="SRC "+String(view.slot+1).paddedLeft('0',2).toStdString();
            const auto text=String::fromUTF8(user.c_str());
            if(name.getText()!=text)name.setText(text,false);
        }
        if(!weight.isMouseButtonDown() && !weight.hasKeyboardFocus(true) &&
           std::abs(weight.getValue()-view.weightDb)>.05)
            weight.setValue(view.weightDb,dontSendNotification);
        setButton(mute,view.localMute,red);
        setButton(solo,view.solo,amber);
        setButton(modeManual,view.mode==int(speechshare_insert::Mode::Manual),amber);
        setButton(modeAuto,view.mode==int(speechshare_insert::Mode::Auto),green);
        setButton(modeBypass,view.mode==int(speechshare_insert::Mode::Bypass),cyan);
        setButton(extraMode,view.mode==int(speechshare_insert::Mode::Mute),red);
        if(compactMode.getSelectedId()!=view.mode+1)
            compactMode.setSelectedId(view.mode+1,dontSendNotification);
        for(int i=0;i<3;++i)setGroupButton(groupButtons[std::size_t(i)],view.group==i,groupTint[i]);
        repaint();
    }
    void paint(Graphics& g) override {
        const int w=getWidth(),h=getHeight();
        const auto tint=groupTint[std::clamp(view.group,0,2)];
        const auto body=getLocalBounds().toFloat().reduced(.75f);
        g.setColour(Colour::fromRGB(4,9,16));g.fillRoundedRectangle(body,8.f);
        g.setGradientFill(ColourGradient(Colour::fromRGB(34,43,54).interpolatedWith(tint,.095f),0.f,0.f,
                                         Colour::fromRGB(10,17,25).interpolatedWith(tint,.16f),0.f,float(h),false));
        g.fillRoundedRectangle(body.reduced(1.0f),7.f);
        const int sectionY=h-133;
        auto colored=Rectangle<float>(3.f,float(sectionY),float(w-6),float(129));
        g.setGradientFill(ColourGradient(tint.withAlpha(.40f),0.f,float(sectionY),
                                         tint.withAlpha(.21f),0.f,float(h),false));
        g.fillRoundedRectangle(colored,6.f);
        g.setColour(tint.withAlpha(.8f));g.drawHorizontalLine(sectionY+1,5.f,float(w-5));
        g.setColour(tint.withAlpha(.18f));g.fillRoundedRectangle(4.f,4.f,float(w-8),9.f,3.f);
        g.setColour(tint.withAlpha(.95f));g.fillRoundedRectangle(6.f,6.f,float(w-12),3.5f,1.5f);
        const auto outline=view.localMute?red:(view.solo?amber:tint);
        g.setColour(outline.withAlpha(.8f));g.drawRoundedRectangle(body.reduced(.3f),8.f,1.1f);
        g.setColour(white.withAlpha(.05f));g.drawHorizontalLine(3,9.f,float(w-9));
        if(isOwn)statusLed(g,7.f,13.f,cyan);
        if(view.localMute)statusLed(g,float(w)-17.f,13.f,red);
        else if(view.solo)statusLed(g,float(w)-17.f,13.f,amber);
        else if(!view.processing)statusLed(g,float(w)-17.f,13.f,secondary,false);
        smallText(g,String(view.slot+1).paddedLeft('0',2),{19,11,w-40,11},9.f,secondary,Justification::centredRight);

        const int mt=meterTop(),mh=meterHeight(h);
        const auto areas=meterColumns(w);
        for(int j=0;j<2;++j){
            const int cx=areas[std::size_t(j)];
            const int mw=std::clamp(w/11,10,22);
            auto recess=Rectangle<float>(float(cx-mw/2-4),float(mt-5),float(mw+8),float(mh+10));
            g.setGradientFill(ColourGradient(Colour::fromRGB(6,16,27),recess.getX(),recess.getY(),
                                            Colour::fromRGB(13,29,43),recess.getRight(),recess.getBottom(),false));
            g.fillRoundedRectangle(recess,3.f);
            g.setColour(border.withAlpha(.8f));g.drawRoundedRectangle(recess.reduced(.5f),3.f,.85f);
            const int segments=std::max(12,mh/6);
            const float step=float(mh-3)/float(segments);
            const float level=j==0?view.inputDb:view.gainDb;
            for(int k=0;k<segments;++k){
                const float db=-72.f + 72.f*float(k)/float(std::max(1,segments-1));
                const float y=float(mt+mh-4)-step*float(k);
                const Colour col=j==1?cyan:(db>=-7.f?red:(db>=-19.f?amber:green));
                const bool lit=level>=db;
                // At silence the graduation remains legible instead of becoming black.
                g.setColour(lit?col:col.withAlpha(j==1?.24f:.21f));
                g.fillRoundedRectangle(float(cx-mw/2),y,float(mw),std::max(2.f,step-1.f),.85f);
                if(lit){
                    g.setColour(col.withAlpha(.19f));
                    g.fillRoundedRectangle(float(cx-mw/2)-1.5f,y-1.f,float(mw)+3.f,std::max(2.f,step+1.f),1.f);
                    g.setColour(col);g.fillRoundedRectangle(float(cx-mw/2),y,float(mw),std::max(2.f,step-1.f),.85f);
                    g.setColour(white.withAlpha(.18f));g.drawHorizontalLine(int(y),float(cx-mw/2),float(cx+mw/2));
                }
            }
            const int colLeft=(j==0)?0:w/3;
            smallText(g,j==0?"INPUT":"AUTO GAIN",{colLeft,mt-27,w/3,16},w>=125?10.f:8.2f,white);
            smallText(g,number(level),{colLeft,mt+mh+5,w/3,15},10.f,j==1?cyan:white);
        }
        smallText(g,"WEIGHT",{2*w/3,mt-27,w/3,16},w>=125?10.f:8.2f,white);
        const int meterEnd=mt+mh;
        // Meter and weight scales; labels are reduced in 8-channel compact mode.
        const bool showNumbers=w>=138;
        for(int t=0;t<=6;++t){
            const int db=-12*t;
            const int y=mt+(mh*t)/6;
            g.setColour(white.withAlpha(t%2==0?.50f:.25f));
            for(int meter=0;meter<2;++meter){
                const int cx=areas[std::size_t(meter)];
                const int mw=std::clamp(w/11,10,22);
                g.drawHorizontalLine(y,float(cx-mw/2-9),float(cx-mw/2-5));
            }
            if(showNumbers && t%2==0){
                const int cx=areas[0];
                smallText(g,String(db),{std::max(1,cx-35),y-7,23,14},8.4f,secondary,Justification::centredRight);
            }
        }
        const int wx=areas[2];
        for(int t=0;t<=4;++t){
            const int y=mt+(mh*t)/4;
            g.setColour(white.withAlpha(t==2?.65f:.36f));
            g.drawHorizontalLine(y,float(wx+14),float(wx+21));
            if(showNumbers){
                const int weightDb=24-12*t;
                smallText(g,String(weightDb>0?"+":"")+String(weightDb),
                          {wx+21,y-7,std::max(14,w-wx-23),14},8.2f,secondary,Justification::centredLeft);
            }
        }
        g.setColour(tint.withAlpha(.24f));
        g.drawHorizontalLine(sectionY+1,8.f,float(w-8));
        smallText(g,"GROUP",{0,h-125,w,15},11.f,white);
        smallText(g,"MODE",{0,h-70,w,14},10.f,white);
    }
    void resized() override {
        const int w=getWidth(),h=getHeight();
        const int pad=std::max(5,w/40);
        name.setBounds(pad,21,w-2*pad,23);
        const int bW=std::max(24,(w-2*pad-4)/2);
        mute.setBounds(pad,47,bW,25);
        solo.setBounds(pad+bW+4,47,std::max(24,w-pad-(pad+bW+4)),25);
        const int top=meterTop(),height=meterHeight(h);
        auto cols=meterColumns(w);
        const int weightWidth=std::clamp(w/3,28,70);
        weight.setBounds(cols[2]-weightWidth/2,top-6,weightWidth,height+24);
        const int groupW=(w-2*pad-6)/3;
        for(int i=0;i<3;++i){
            const int x=pad+i*(groupW+3);
            groupButtons[std::size_t(i)].setBounds(x,h-107,i==2?std::max(18,w-pad-x):groupW,30);
        }
        const bool compact=w<166;
        compactMode.setVisible(compact);
        for(auto* b:{&modeManual,&modeAuto,&modeBypass,&extraMode})b->setVisible(!compact);
        compactMode.setBounds(pad,h-52,w-2*pad,29);
        const int modeW=(w-2*pad-9)/4;
        modeManual.setBounds(pad,h-52,modeW,29);
        modeAuto.setBounds(pad+modeW+3,h-52,modeW,29);
        modeBypass.setBounds(pad+(modeW+3)*2,h-52,modeW,29);
        const int lastX=pad+(modeW+3)*3;
        extraMode.setBounds(lastX,h-52,std::max(29,w-pad-lastX),29);
    }
private:
    static speechshare_insert::SharedPool& pool(){return speechshare_insert::SharedPool::instance();}
    bool valid()const{return view.slot>=0&&view.generation!=0;}
    void chooseMode(speechshare_insert::Mode m){if(valid())pool().requestMode(view.slot,view.generation,int(m));}
    static int meterTop(){return 111;}
    static int meterHeight(int h){return std::max(85,h-260);}
    static std::array<int,3> meterColumns(int w){return {w/6,w/2,5*w/6};}
    void commitName(){if(valid())pool().requestDisplayName(view.slot,view.generation,name.getText().trim().toStdString());}
    speechshare_insert::ParticipantView view;
    bool isOwn=false;
    TextEditor name;
    TextButton mute,solo,modeManual,modeAuto,modeBypass,extraMode;
    Slider weight;
    ComboBox compactMode;
    std::array<TextButton,3> groupButtons;
};

SpeechShareInsertEditor::SpeechShareInsertEditor(SpeechShareInsertProcessor& p)
    :AudioProcessorEditor(p),processor(p),skin(std::make_unique<BroadcastLookAndFeel>()) {
    setOpaque(true);setLookAndFeel(skin.get());
    setResizable(true,true);setResizeLimits(760,425,3000,1800);
    // Real Backstage signature supplied by the project owner, cropped to its
    // non-transparent bounds and embedded into the VST3 at build time.
    const auto logo = juce::ImageFileFormat::loadFrom(
        BinaryData::backstage_logo_png, BinaryData::backstage_logo_pngSize);
    backstageLogo.setImage(logo, juce::RectanglePlacement::centred);
    backstageLogo.setInterceptsMouseClicks(false, false);
    addAndMakeVisible(backstageLogo);
#if defined(SPEECHSHARE_STANDALONE_APP)
    heading.setText("SPEECHSHARE  /  LIVE",dontSendNotification);
#else
    heading.setText("SPEECHSHARE  /  INSERT",dontSendNotification);
#endif
    labelStyle(heading,12.f,true);addAndMakeVisible(heading);
    labelStyle(statusLabel,10.f,false,secondary);addAndMakeVisible(statusLabel);
    labelStyle(sessionInfo,10.f,false,secondary);addAndMakeVisible(sessionInfo);
    labelStyle(keyHeading,12.f,true);keyHeading.setText("VIRTUAL KEYS / GROUP SELECT",dontSendNotification);addAndMakeVisible(keyHeading);
    labelStyle(stripHeading,11.f,true);stripHeading.setText("CHANNEL STRIPS",dontSendNotification);addAndMakeVisible(stripHeading);
    labelStyle(footnote,9.f,false,secondary);
#if defined(SPEECHSHARE_STANDALONE_APP)
#if defined(SPEECHSHARE_PHON_MODULAR)
    footnote.setText("LIVE PHON MODULAR 4 v0.3.0  |  4 SESSIONS x 8 CH  |  Forward-only internal PATCH",dontSendNotification);
#elif defined(SPEECHSHARE_PHON_STANDALONE)
    footnote.setText("LIVE PHON v0.1.0  |  Standalone ASIO  |  PHON adjusts shared detector response",dontSendNotification);
#else
    footnote.setText("LIVE v0.1.0  /  ASIO PATCH  |  One mono participant per source  |  Lookahead unavailable",dontSendNotification);
#endif
#else
    footnote.setText("INSERT v0.1.3  /  BROADCAST CONSOLE UI  |  Solo affects this session only  |  Lookahead unavailable",dontSendNotification);
#endif
    addAndMakeVisible(footnote);
    sessionSelector.addItemList({"Session 1","Session 2","Session 3","Session 4","Session 5","Session 6","Session 7","Session 8"},1);
    sessionSelector.setSelectedId(processor.currentSession(),dontSendNotification);
    sessionSelector.onChange=[this]{processor.setOwnSession(sessionSelector.getSelectedId());bank=0;resized();refreshParticipants();};
#ifdef SPEECHSHARE_PHON_MODULAR
    sessionSelector.setEnabled(false); // graph owns session IDs; navigate via host toolbar
#endif
    addAndMakeVisible(sessionSelector);
    classicButton.onClick=[this]{setDetector(false);};smartButton.onClick=[this]{setDetector(true);};
    for(auto* b:{&classicButton,&smartButton,&smartHelp,&presetCopy,&presetPaste,&masterMute,&masterBypass})addAndMakeVisible(*b);
    presetCopy.onClick=[this]{exportPreset();};presetPaste.onClick=[this]{importPreset();};
    presetCopy.setTooltip("Copy this session's settings to clipboard in SpeechShare preset format");
    masterMute.setTooltip("Mute all participants in this session");
    masterBypass.setTooltip("Bypass automixing for the entire session");
    classicButton.setTooltip("Classic gain sharing across all participants in this session");
    smartButton.setTooltip("Experimental Smart detector across all participants in this session");
    smartHelp.setTooltip("What is the difference between Classic and Smart?");
    smartHelp.onClick=[this]{ juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon, "Classic vs Smart", "CLASSIC\nTraditional gain sharing based on the measured channel energy.\n\nSMART\nEnhanced detector with extra analysis to better reject persistent noise and react differently to speech changes."); };
    presetPaste.setTooltip("Paste compatible SpeechShare clipboard preset onto the currently open session");
    masterMute.onClick=[this]{auto s=processor.currentControls();s.muted=!s.muted;processor.setSessionControls(s);};
    masterBypass.onClick=[this]{auto s=processor.currentControls();s.bypass=!s.bypass;processor.setSessionControls(s);};
    masterGain.setRange(-36.,12.,.1);masterGain.setValue(0.,dontSendNotification);
    configureSlider(masterGain,Slider::LinearHorizontal,Slider::TextBoxRight,46);
    masterGain.setTextBoxStyle(Slider::TextBoxRight,false,46,20);
    masterGain.setDoubleClickReturnValue(true,0.);
    masterGain.onValueChange=[this]{if(refreshing)return;auto s=processor.currentControls();s.gainDb=float(masterGain.getValue());processor.setSessionControls(s);};
    addAndMakeVisible(masterGain);
    masterGainLabel.setText("MASTER dB",dontSendNotification);labelStyle(masterGainLabel,9.f,false,secondary);masterGainLabel.setJustificationType(Justification::centred);addAndMakeVisible(masterGainLabel);
    bankPrev.onClick=[this]{changeBank(-1);};bankNext.onClick=[this]{changeBank(1);};
    view8.onClick=[this]{setViewSize(8);};
    for(auto* b:{&bankPrev,&bankNext,&view8})addAndMakeVisible(*b);
    rightContent=std::make_unique<RightPanel>();
    rightViewport.setViewedComponent(rightContent.get(),false);
    rightViewport.setScrollBarsShown(true,false);
    rightViewport.setScrollBarThickness(9);
    addAndMakeVisible(rightViewport);
    // The right panel owns the positioning of key and detector controls.
    rightContent->addAndMakeVisible(keyHeading);
    keyMaster.onClick=[this]{processor.setKeyMaster(keyMaster.getToggleState());};
    rightContent->addAndMakeVisible(keyMaster);
    for(int g=0;g<3;++g) {
        auto& tab=keyTabs[std::size_t(g)];
        tab.setButtonText(g==0?"A":(g==1?"B":"C"));
        tab.onClick=[this,g]{ selectedKeyGroup=g; updateKeyTabs(); resized(); refreshKeys(); };
        tab.setTooltip(g==0?"Show Virtual Key group A":(g==1?"Show Virtual Key group B":"Show Virtual Key group C"));
        rightContent->addAndMakeVisible(tab);
    }
    for(int g=0;g<3;++g) {
        auto& l=keyLabels[std::size_t(g)];l.setText((g==0?"KEY A":(g==1?"KEY B":"KEY C")),dontSendNotification);
        labelStyle(l,11.f,true,groupTint[g]);rightContent->addAndMakeVisible(l);
        auto& on=keyEnable[std::size_t(g)];on.setButtonText("ON");on.onClick=[this,g]{pushKey(g);};rightContent->addAndMakeVisible(on);
        auto& t=keyType[std::size_t(g)];t.addItem("Pink",1);t.addItem("Sine",2);t.onChange=[this,g]{pushKey(g);};rightContent->addAndMakeVisible(t);
        auto& f=keyFrequency[std::size_t(g)];f.setRange(10.,20000.,1.);f.setValue(40.,dontSendNotification);
        configureSlider(f,Slider::LinearHorizontal,Slider::TextBoxRight,56);
        f.setTextBoxStyle(Slider::TextBoxRight,false,56,19);f.setTextValueSuffix(" Hz");
        f.onValueChange=[this,g]{if(!refreshing)pushKey(g);};rightContent->addAndMakeVisible(f);
        auto& db=keyLevel[std::size_t(g)];db.setRange(-96.,0.,.1);db.setValue(-56.,dontSendNotification);
        configureSlider(db,Slider::LinearHorizontal,Slider::TextBoxRight,56);
        db.setTextBoxStyle(Slider::TextBoxRight,false,56,19);db.setTextValueSuffix(" dB");
        db.onValueChange=[this,g]{if(!refreshing)pushKey(g);};rightContent->addAndMakeVisible(db);
    }
#if defined(SPEECHSHARE_PHON_STANDALONE)
    advancedHeading.setText("PHON  /  RESPONSE",dontSendNotification);
#else
    advancedHeading.setText("DETECTOR / TIMING",dontSendNotification);
#endif
    labelStyle(advancedHeading,11.f,true,white);rightContent->addAndMakeVisible(advancedHeading);
    advancedState.setText("SESSION-WIDE",dontSendNotification);
    labelStyle(advancedState,8.f,false,secondary);rightContent->addAndMakeVisible(advancedState);
    const char* names[5]={"RMS WINDOW","STRENGTH","ATTACK","RELEASE","DETECTOR HPF"};
    const double limits[5][4]={{10.,500.,1.,100.},{.5,3.,.01,1.},{1.,120.,1.,12.},
                               {10.,1500.,1.,160.},{0.,350.,1.,100.}};
    for(int j=0;j<5;++j){
        auto& label=advancedLabels[std::size_t(j)];
        label.setText(names[j],dontSendNotification);
        labelStyle(label,9.f,false,secondary);rightContent->addAndMakeVisible(label);
        auto& control=advancedSliders[std::size_t(j)];
        control.setRange(limits[j][0],limits[j][1],limits[j][2]);
        control.setValue(limits[j][3],dontSendNotification);
        configureSlider(control,Slider::LinearHorizontal,Slider::TextBoxRight,56);
        control.setTextBoxStyle(Slider::TextBoxRight,false,56,18);
        if(j!=1)control.setTextValueSuffix(j==4?" Hz":" ms");
        control.onValueChange=[this,j]{if(!refreshing)pushAdvanced(j);};
#if !defined(SPEECHSHARE_PHON_STANDALONE)
        rightContent->addAndMakeVisible(control);
#endif
    }
#if defined(SPEECHSHARE_PHON_STANDALONE)
    phonDial.setName("PHON");
    phonDial.setRange(0.,100.,1.);
    phonDial.setValue(50.,dontSendNotification);
    phonDial.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    phonDial.setTextBoxStyle(juce::Slider::NoTextBox,false,0,0);
    phonDial.setDoubleClickReturnValue(true,50.);
    phonDial.getProperties().set("SpeechSharePHON",true);
    phonDial.setTooltip("PHON: left softer, centre original settings, right more aggressive. Double-click: neutral.");
    phonDial.onValueChange=[this]{if(!refreshing)applyPhon();};
    rightContent->addAndMakeVisible(phonDial);
    phonTitle.setText("ONE KNOB / FULL RESPONSE",dontSendNotification);
    labelStyle(phonTitle,10.f,true,cyan);phonTitle.setJustificationType(Justification::centred);
    rightContent->addAndMakeVisible(phonTitle);
    phonHint.setText("MORBIDO     /     AGGRESSIVO",dontSendNotification);
    labelStyle(phonHint,9.f,false,secondary);phonHint.setJustificationType(Justification::centred);
    rightContent->addAndMakeVisible(phonHint);
    phonValue.setText("50  /  NEUTRO",dontSendNotification);
    labelStyle(phonValue,12.f,true,white);phonValue.setJustificationType(Justification::centred);
    rightContent->addAndMakeVisible(phonValue);
#else
    lookaheadNotice.setText("LOOKAHEAD: 0 ms  (not implemented)",dontSendNotification);
    labelStyle(lookaheadNotice,9.f,false,secondary);
    lookaheadNotice.setTooltip("Lookahead requires real audio delay and host PDC: it is not implemented in Insert DSP.");
    rightContent->addAndMakeVisible(lookaheadNotice);
#endif
    stripViewport.setScrollBarsShown(true,true);
    stripViewport.setScrollBarThickness(9);
    stripViewport.setViewedComponent(&stripContent,false);
    addAndMakeVisible(stripViewport);
    for(auto& strip:strips){strip=std::make_unique<ChannelStrip>();stripContent.addAndMakeVisible(*strip);strip->setVisible(false);}
    // JUCE can request resized() from setResizable/setResizeLimits before
    // the right viewport and its component have been initialized.
    editorReady=true;
    // Fit the initial GUI inside the work area on compact/high-DPI laptops.
    int initialW=1020,initialH=535;
    if(const auto* display=Desktop::getInstance().getDisplays().getPrimaryDisplay()){
        initialW=std::clamp(static_cast<int>(display->userBounds.getWidth())-55,760,1020);
        initialH=std::clamp(static_cast<int>(display->userBounds.getHeight())-75,425,535);
    }
    setSize(initialW,initialH);
    resized();
    updateKeyTabs();
    refreshKeys();refreshSessionControls();refreshParticipants();refreshAdvanced();
    startTimerHz(16); // GUI only. DSP does not allocate, lock, or touch these controls.
}
SpeechShareInsertEditor::~SpeechShareInsertEditor(){
    stopTimer();
    stripViewport.setViewedComponent(nullptr,false);
    rightViewport.setViewedComponent(nullptr,false);
    setLookAndFeel(nullptr);
}
void SpeechShareInsertEditor::paint(Graphics& g) {
    const int w=getWidth(),h=getHeight();
    g.setGradientFill(ColourGradient(Colour::fromRGB(16,23,33),0.f,0.f,backdrop,0.f,float(h),false));
    g.fillAll();
    const int top=std::clamp(int(float(h)*93.f/535.f),93,132);
    g.setGradientFill(ColourGradient(Colour::fromRGB(37,50,65),0.f,0.f,
                                     Colour::fromRGB(14,23,34),0.f,float(top),false));
    g.fillRect(0,0,w,top);
    g.setColour(white.withAlpha(.085f));g.drawHorizontalLine(1,0.f,float(w));
    g.setColour(cyan);g.fillRect(0,top-3,w,2);
    g.setColour(cyan.withAlpha(.12f));g.fillRect(0,top-1,w,3);
    // Subtle divider between the signed logo and product name.
    g.setColour(rim.withAlpha(.60f));
    const int dx=int(164.f*std::clamp(float(w)/1020.f,1.f,1.36f));
    g.drawVerticalLine(dx,9.f,float(top)*.50f);
    const int side=std::clamp(int(w*0.27f),240,430);
    const int rightX=w-side-9;
    framed(g,Rectangle<float>(8.f,float(top),float(rightX-15),float(std::max(100,h-top-22))));
    framed(g,Rectangle<float>(float(rightX),float(top),float(side),float(std::max(100,h-top-22))));
    g.setColour(border.withAlpha(.7f));g.drawHorizontalLine(h-19,12.f,float(w-12));
    // Slim status bar carries an unobtrusive LIVE/READY indicator.
    statusLed(g,float(w-20),float(h-12),green);
}
void SpeechShareInsertEditor::resized() {
    if(!editorReady || rightContent==nullptr)return;
    const int w=getWidth(),h=getHeight();
    const int top=std::clamp(int(float(h)*93.f/535.f),93,132);
    const float hs=std::clamp(float(top)/93.f,1.f,1.42f);
    const float xscale=std::clamp(float(w)/1020.f,1.f,1.36f);
    const auto xx=[xscale](int v){return int(std::round(float(v)*xscale));};
    const auto yy=[hs](int v){return int(std::round(float(v)*hs));};
    backstageLogo.setBounds(xx(10),0,xx(149),yy(53));
    heading.setFont(juce::Font(juce::FontOptions(12.f*hs,juce::Font::bold)));
    heading.setBounds(xx(176),yy(12),xx(238),yy(24));
    sessionInfo.setBounds(xx(424),yy(9),std::max(84,w-xx(591)),yy(19));
    statusLabel.setBounds(xx(424),yy(31),std::max(84,w-xx(591)),yy(15));
    sessionSelector.setBounds(w-xx(145),yy(12),xx(131),yy(25));
    presetCopy.setBounds(xx(13),yy(56),xx(80),yy(24));
    presetPaste.setBounds(xx(96),yy(56),xx(80),yy(24));
    classicButton.setBounds(xx(190),yy(56),xx(72),yy(24));
    smartButton.setBounds(xx(266),yy(56),xx(68),yy(24));
    smartHelp.setBounds(xx(338),yy(56),xx(24),yy(24));
    masterGainLabel.setBounds(xx(370),yy(61),xx(58),yy(15));
    const int sliderWidth=std::clamp(w-xx(678),xx(92),xx(170));
    masterGain.setBounds(xx(428),yy(54),sliderWidth,yy(29));
    masterBypass.setBounds(w-xx(239),yy(56),xx(111),yy(24));
    masterMute.setBounds(w-xx(124),yy(56),xx(111),yy(24));

    const int side=std::clamp(int(w*0.27f),240,430);
    const int rightX=w-side-9;
    stripHeading.setBounds(18,top+6,170,20);
    bankPrev.setBounds(181,top+6,24,20);
    bankNext.setBounds(208,top+6,24,20);
    view8.setBounds(237,top+6,52,20);

    stripViewport.setBounds(17,top+29,std::max(80,rightX-30),std::max(50,h-top-58));
    const int available=stripViewport.getWidth()-10;
    displayedPerPage=std::max(1,std::min(bankSize,8));
    const int maxBank=visibleTotal>0?(visibleTotal-1)/displayedPerPage:0;
    bank=std::clamp(bank,0,maxBank);
    const int first=bank*displayedPerPage;
    const int shownCount=std::max(1,std::min(displayedPerPage, std::max(0, visibleTotal-first)));
    stripColumnWidth=std::max(minimumStripWidth,(available-(shownCount-1)*stripGap)/shownCount);
    // Fill the channel viewport at normal sizes; scroll only if the user
    // manually forces a narrow window where eight strips cannot fit.
    const int contentHeight=std::max(345,stripViewport.getHeight()-2);
    const int desiredWidth=shownCount*stripColumnWidth+(shownCount-1)*stripGap+9;
    stripContent.setSize(std::max(stripViewport.getWidth()-2,desiredWidth),contentHeight);
    stripViewport.setViewPosition(0,0);
    for(int i=0;i<speechshare_insert::maxInstances;++i){
        const bool shown=i>=first && i<first+shownCount && i<visibleTotal;
        strips[std::size_t(i)]->setVisible(shown);
        if(shown)strips[std::size_t(i)]->setBounds(4+(i-first)*(stripColumnWidth+stripGap),1,
                                                stripColumnWidth,contentHeight-3);
    }
    bankPrev.setEnabled(bank>0);
    bankNext.setEnabled((bank+1)*displayedPerPage<visibleTotal);
    const int from=visibleTotal==0?0:first+1;
    const int to=std::min(visibleTotal,first+shownCount);
    stripHeading.setText("CHANNELS  "+String(from)+"-"+String(to)+" / "+String(visibleTotal),dontSendNotification);

    rightViewport.setBounds(rightX+4,top+3,side-8,std::max(50,h-top-29));
    const int rw=std::max(204,rightViewport.getWidth()-11);
    rightContent->setSize(rw,std::max(rightContentHeight,rightViewport.getHeight()));
    const float panelScale=std::max(1.f,float(rightContent->getHeight())/415.f);
    const auto sy=[panelScale](int val){return int(std::round(float(val)*panelScale));};
    keyHeading.setFont(juce::Font(juce::FontOptions(12.f*std::min(panelScale,1.5f),juce::Font::bold)));
    advancedHeading.setFont(juce::Font(juce::FontOptions(11.f*std::min(panelScale,1.5f),juce::Font::bold)));
    keyHeading.setBounds(13,sy(5),rw-24,sy(22));
    keyMaster.setBounds(13,sy(27),rw-24,sy(18));
    const int tabY=sy(50), tabW=std::max(44,(rw-40)/3), tabGap=5;
    for(int i=0;i<3;++i){
        keyTabs[std::size_t(i)].setBounds(13 + i * (tabW + tabGap),tabY,tabW,sy(24));
    }
    const int g = selectedKeyGroup;
    for(int i=0;i<3;++i){
        const bool active = i == g;
        keyLabels[std::size_t(i)].setVisible(active);
        keyEnable[std::size_t(i)].setVisible(active);
        keyType[std::size_t(i)].setVisible(active);
        keyFrequency[std::size_t(i)].setVisible(active);
        keyLevel[std::size_t(i)].setVisible(active);
        if(active){
            const int y=sy(88);
            keyLabels[std::size_t(i)].setFont(juce::Font(juce::FontOptions(11.f*std::min(panelScale,1.5f),juce::Font::bold)));
            keyLabels[std::size_t(i)].setBounds(18,y,96,sy(18));
            keyEnable[std::size_t(i)].setBounds(rw-61,y-1,52,sy(18));
            keyType[std::size_t(i)].setBounds(16,y+sy(28),84,sy(22));
            keyFrequency[std::size_t(i)].setBounds(104,y+sy(27),std::max(90,rw-115),sy(24));
            keyLevel[std::size_t(i)].setBounds(74,y+sy(58),std::max(90,rw-85),sy(24));
        }
    }
#if defined(SPEECHSHARE_PHON_STANDALONE)
    advancedHeading.setBounds(15,sy(229),rw-30,sy(20));
    advancedState.setBounds(13,sy(249),rw-26,sy(15));
    phonTitle.setBounds(13,sy(264),rw-26,sy(16));
    const int dialSide=std::min(rw-28,sy(101));
    phonDial.setBounds((rw-dialSide)/2,sy(270),dialSide,dialSide);
    phonValue.setBounds(20,sy(367),rw-40,sy(18));
    phonHint.setBounds(13,sy(385),rw-26,sy(17));
#else
    advancedHeading.setBounds(13,sy(227),140,sy(19));
    advancedState.setBounds(149,sy(227),rw-160,sy(18));
    for(int j=0;j<5;++j){
        const int y=sy(251+j*26);
        advancedLabels[std::size_t(j)].setFont(juce::Font(juce::FontOptions(9.f*std::min(panelScale,1.45f))));
        advancedLabels[std::size_t(j)].setBounds(15,y,98,sy(19));
        advancedSliders[std::size_t(j)].setBounds(108,y,std::max(88,rw-118),sy(20));
    }
    lookaheadNotice.setBounds(15,sy(389),rw-20,sy(16));
#endif
    footnote.setBounds(13,h-19,w-26,16);
}
void SpeechShareInsertEditor::timerCallback(){refreshParticipants();refreshSessionControls();refreshKeys();refreshAdvanced();}
void SpeechShareInsertEditor::updateKeyTabs(){
    static const char* titles[3] = {"GROUP A","GROUP B","GROUP C"};
    for(int i=0;i<3;++i){
        setGroupButton(keyTabs[std::size_t(i)], i == selectedKeyGroup, groupTint[i]);
        keyTabs[std::size_t(i)].setColour(juce::TextButton::textColourOffId, i == selectedKeyGroup ? backdrop : white);
    }
    keyHeading.setText("VIRTUAL KEYS / " + juce::String(titles[selectedKeyGroup]), juce::dontSendNotification);
    if(rightContent!=nullptr)rightContent->setActiveGroup(selectedKeyGroup);
}
void SpeechShareInsertEditor::pushKey(int g){
    if(refreshing)return;
    auto keys=processor.currentKeys();auto setting=keys.groups[std::size_t(g)];
    setting.enabled=keyEnable[std::size_t(g)].getToggleState();
    setting.generator=keyType[std::size_t(g)].getSelectedId()==1?
        speechshare_insert::KeyGenerator::Pink:speechshare_insert::KeyGenerator::Sine;
    setting.frequencyHz=float(keyFrequency[std::size_t(g)].getValue());
    setting.levelDb=float(keyLevel[std::size_t(g)].getValue());
    processor.setKeyGroup(g,setting);
}
void SpeechShareInsertEditor::pushAdvanced(int parameter){
    if(refreshing||parameter<0||parameter>=5)return;
    auto& pool=speechshare_insert::SharedPool::instance();
    std::array<speechshare_insert::ParticipantView,speechshare_insert::maxInstances> views{};
    const int n=pool.sessionViews(processor.currentSession(),views.data(),int(views.size()));
    const float value=float(advancedSliders[std::size_t(parameter)].getValue());
    for(int i=0;i<n;++i){
        const auto& v=views[std::size_t(i)];
        pool.requestAdvanced(v.slot,v.generation,parameter,value);
    }
}
#if defined(SPEECHSHARE_PHON_STANDALONE)
void SpeechShareInsertEditor::applyPhon(){
    if(refreshing)return;
    phonLastEditMs=juce::Time::getMillisecondCounter();
    const float p=float(phonDial.getValue());
    const auto values=speechshare_phon::advancedValues(p);
    auto& pool=speechshare_insert::SharedPool::instance();
    std::array<speechshare_insert::ParticipantView,speechshare_insert::maxInstances> views{};
    const int n=pool.sessionViews(processor.currentSession(),views.data(),int(views.size()));
    for(int i=0;i<n;++i){
        const auto& v=views[std::size_t(i)];
        // RMS & HPF are deliberately unchanged while the dial moves:
        // changing RMS in flight resets the detector window and may cause transients.
        for(int j=1;j<=3;++j)pool.requestAdvanced(v.slot,v.generation,j,values[std::size_t(j)]);
    }
    phonValue.setText(juce::String(int(p))+"  /  "+(p<45.f?"MORBIDO":(p>55.f?"AGGRESSIVO":"NEUTRO")),dontSendNotification);
}
#endif
void SpeechShareInsertEditor::refreshAdvanced(){
    auto& pool=speechshare_insert::SharedPool::instance();
    std::array<speechshare_insert::ParticipantView,speechshare_insert::maxInstances> views{};
    const int n=pool.sessionViews(processor.currentSession(),views.data(),int(views.size()));
    if(n<=0){advancedState.setText("NO SOURCES",juce::dontSendNotification);return;}
#if defined(SPEECHSHARE_PHON_STANDALONE)
    bool mixed=false;
    const float strength=views[0].advanced[1];
    for(int i=1;i<n;++i)
        if(std::abs(views[std::size_t(i)].advanced[1]-strength)>.02f)mixed=true;
    advancedState.setText(mixed?"MIXED CHANNEL VALUES":"SESSION-WIDE",juce::dontSendNotification);
    const auto elapsed=juce::Time::getMillisecondCounter()-phonLastEditMs;
    if(elapsed>600 && !phonDial.isMouseButtonDown() && !phonDial.hasKeyboardFocus(true)){
        const int p=int(std::lround(speechshare_phon::positionForStrength(strength)));
        if(int(phonDial.getValue())!=p){
            refreshing=true;
            phonDial.setValue(p,dontSendNotification);
            refreshing=false;
        }
        phonValue.setText(juce::String(p)+"  /  "+(p<45?"MORBIDO":(p>55?"AGGRESSIVO":"NEUTRO")),dontSendNotification);
    }
#else
    bool mixed=false;
    refreshing=true;
    for(int j=0;j<5;++j){
        float ref=views[0].advanced[std::size_t(j)];
        for(int i=1;i<n;++i)
            if(std::abs(views[std::size_t(i)].advanced[std::size_t(j)]-ref)>.015f)mixed=true;
        auto& slider=advancedSliders[std::size_t(j)];
        if(!slider.isMouseButtonDown()&&!slider.hasKeyboardFocus(true) &&
           std::abs(slider.getValue()-double(ref))>.015)
            slider.setValue(ref,juce::dontSendNotification);
    }
    advancedState.setText(mixed?"MIXED - edit to apply to all":"SESSION-WIDE - ALL SOURCES",juce::dontSendNotification);
    refreshing=false;
#endif
}
void SpeechShareInsertEditor::refreshKeys(){
    const auto data=processor.currentKeys();
    refreshing=true;
    keyMaster.setToggleState(data.master,dontSendNotification);
    updateKeyTabs();
    for(int g=0;g<3;++g){
        const auto& k=data.groups[std::size_t(g)];
        keyEnable[std::size_t(g)].setToggleState(k.enabled,dontSendNotification);
        keyType[std::size_t(g)].setSelectedId(k.generator==speechshare_insert::KeyGenerator::Pink?1:2,dontSendNotification);
        if(!keyFrequency[std::size_t(g)].isMouseButtonDown() &&
           !keyFrequency[std::size_t(g)].hasKeyboardFocus(true))
            keyFrequency[std::size_t(g)].setValue(k.frequencyHz,dontSendNotification);
        if(!keyLevel[std::size_t(g)].isMouseButtonDown() &&
           !keyLevel[std::size_t(g)].hasKeyboardFocus(true))
            keyLevel[std::size_t(g)].setValue(k.levelDb,dontSendNotification);
        keyFrequency[std::size_t(g)].setEnabled(k.generator==speechshare_insert::KeyGenerator::Sine);
    }
    refreshing=false;
}
void SpeechShareInsertEditor::refreshSessionControls(){
    auto control=processor.currentControls();
    setButton(masterMute,control.muted,red);
    setButton(masterBypass,control.bypass,amber);
    refreshing=true;
    if(!masterGain.isMouseButtonDown()&&!masterGain.hasKeyboardFocus(true))
        masterGain.setValue(control.gainDb,dontSendNotification);
    refreshing=false;
}
void SpeechShareInsertEditor::updateSessionControls(){
    auto s=processor.currentControls();s.gainDb=float(masterGain.getValue());processor.setSessionControls(s);
}
void SpeechShareInsertEditor::setDetector(bool smart){
    std::array<speechshare_insert::ParticipantView,speechshare_insert::maxInstances> views{};
    auto& pool=speechshare_insert::SharedPool::instance();
    const auto n=pool.sessionViews(processor.currentSession(),views.data(),int(views.size()));
    for(int i=0;i<n;++i)pool.requestSmart(views[std::size_t(i)].slot,views[std::size_t(i)].generation,smart);
}
void SpeechShareInsertEditor::changeBank(int diff){
    const int maxBank=visibleTotal>0?(visibleTotal-1)/displayedPerPage:0;
    bank=std::clamp(bank+diff,0,maxBank);
    resized();
    refreshParticipants();
}
void SpeechShareInsertEditor::setViewSize(int count){
    juce::ignoreUnused(count);
    bankSize=8;
    bank=0;
    setButton(view8,true,cyan);
    resized();
    refreshParticipants();
}
void SpeechShareInsertEditor::refreshParticipants(){
    if(sessionSelector.getSelectedId()!=processor.currentSession())
        sessionSelector.setSelectedId(processor.currentSession(),dontSendNotification);
    std::array<speechshare_insert::ParticipantView,speechshare_insert::maxInstances> views{};
    auto& pool=speechshare_insert::SharedPool::instance();
    const int n=pool.sessionViews(processor.currentSession(),views.data(),int(views.size()));
    int nSmart=0,nProcessing=0,nMute=0;
    for(int i=0;i<n;++i){
        auto& v=views[std::size_t(i)];
        if(v.smart)++nSmart;if(v.processing)++nProcessing;if(v.localMute)++nMute;
        strips[std::size_t(i)]->update(v,v.slot==processor.instanceSlot());
    }
    for(int i=n;i<speechshare_insert::maxInstances;++i)strips[std::size_t(i)]->setVisible(false);
    if(visibleTotal!=n){visibleTotal=n;resized();}
    setButton(classicButton,nSmart==0,green);setButton(smartButton,n>0&&nSmart==n,cyan);
    setButton(view8,true,cyan);
    const auto status=nSmart==0?"CLASSIC":(nSmart==n?"SMART":"MIXED");
    sessionInfo.setText("SESSION "+String(processor.currentSession())+"    /    "+String(n)+" SOURCES    /    "+
        String(nProcessing)+" PROCESSING    /    "+String(nMute)+" MUTED    /    "+status,dontSendNotification);
}
void SpeechShareInsertEditor::exportPreset(){
    auto& pool=speechshare_insert::SharedPool::instance();
    juce::var root(new juce::DynamicObject());
    auto* obj=root.getDynamicObject();
    obj->setProperty("format","SpeechShareInsertPreset");obj->setProperty("version",1);
    const auto ctrl=processor.currentControls();
    obj->setProperty("bypass",ctrl.bypass);obj->setProperty("mute",ctrl.muted);
    obj->setProperty("gain",ctrl.gainDb);
    const auto key=processor.currentKeys();
    obj->setProperty("keyMaster",key.master);
    juce::Array<juce::var> groups;
    for(const auto& k:key.groups){
        juce::var item(new juce::DynamicObject());auto* o=item.getDynamicObject();
        o->setProperty("on",k.enabled);o->setProperty("type",int(k.generator));
        o->setProperty("hz",k.frequencyHz);o->setProperty("db",k.levelDb);groups.add(item);
    }
    obj->setProperty("keys",juce::var(groups));
    std::array<speechshare_insert::ParticipantView,speechshare_insert::maxInstances> views{};
    const int n=pool.sessionViews(processor.currentSession(),views.data(),int(views.size()));
    juce::Array<juce::var> channels;
    for(int i=0;i<n;++i){
        const auto& v=views[std::size_t(i)];
        juce::var item(new juce::DynamicObject());auto* o=item.getDynamicObject();
        o->setProperty("name",juce::String::fromUTF8(pool.getDisplayName(v.slot,v.generation).c_str()));
        o->setProperty("mute",v.localMute);o->setProperty("solo",v.solo);
        o->setProperty("mode",v.mode);o->setProperty("group",v.group);
        o->setProperty("weight",v.weightDb);o->setProperty("level",v.levelDb);
        o->setProperty("smart",v.smart);
        const char* names[5]={"rms","strength","attack","release","hpf"};
        for(int j=0;j<5;++j)o->setProperty(names[j],v.advanced[std::size_t(j)]);
        channels.add(item);
    }
    obj->setProperty("sources",juce::var(channels));
    juce::SystemClipboard::copyTextToClipboard(juce::JSON::toString(root,true));
    statusLabel.setText("PRESET COPIED",dontSendNotification);
}
void SpeechShareInsertEditor::importPreset(){
    auto root=juce::JSON::parse(juce::SystemClipboard::getTextFromClipboard());
    if(!root.isObject() || root.getProperty("format",juce::var()).toString()!="SpeechShareInsertPreset"){
        statusLabel.setText("INVALID PRESET",dontSendNotification);return;
    }
    const auto ctrl=processor.currentControls();
    speechshare_insert::SessionControls next=ctrl;
    next.bypass=bool(root.getProperty("bypass",ctrl.bypass));
    next.muted=bool(root.getProperty("mute",ctrl.muted));
    next.gainDb=float(double(root.getProperty("gain",ctrl.gainDb)));
    processor.setSessionControls(next);
    processor.setKeyMaster(bool(root.getProperty("keyMaster",false)));
    auto keys=root.getProperty("keys",juce::var());
    if(auto* a=keys.getArray())for(int i=0;i<std::min(3,a->size());++i){
        const auto k=(*a)[i];if(!k.isObject())continue;
        speechshare_insert::KeyGroupSettings setting;
        setting.enabled=bool(k.getProperty("on",true));
        setting.generator=int(k.getProperty("type",1))==0?speechshare_insert::KeyGenerator::Pink:speechshare_insert::KeyGenerator::Sine;
        setting.frequencyHz=float(double(k.getProperty("hz",40.)));
        setting.levelDb=float(double(k.getProperty("db",-56.)));
        processor.setKeyGroup(i,setting);
    }
    std::array<speechshare_insert::ParticipantView,speechshare_insert::maxInstances> views{};
    auto& pool=speechshare_insert::SharedPool::instance();
    const int n=pool.sessionViews(processor.currentSession(),views.data(),int(views.size()));
    auto sources=root.getProperty("sources",juce::var());
    if(auto* a=sources.getArray())for(int i=0;i<std::min(n,a->size());++i){
        const auto v=views[std::size_t(i)];const auto ch=(*a)[i];if(!ch.isObject())continue;
        pool.requestMode(v.slot,v.generation,std::clamp(int(ch.getProperty("mode",1)),0,3));
        pool.requestGroup(v.slot,v.generation,std::clamp(int(ch.getProperty("group",0)),0,2));
        pool.requestMute(v.slot,v.generation,bool(ch.getProperty("mute",false)));
        pool.requestSolo(v.slot,v.generation,bool(ch.getProperty("solo",false)));
        pool.requestWeight(v.slot,v.generation,float(double(ch.getProperty("weight",0.))));
        pool.requestLevel(v.slot,v.generation,float(double(ch.getProperty("level",0.))));
        pool.requestSmart(v.slot,v.generation,bool(ch.getProperty("smart",false)));
        const char* names[5]={"rms","strength","attack","release","hpf"};
        for(int j=0;j<5;++j){
            // Previous preset JSON did not contain these parameters.
            if(ch.hasProperty(names[j]))
                pool.requestAdvanced(v.slot,v.generation,j,float(double(ch.getProperty(names[j],0.))));
        }
        pool.requestDisplayName(v.slot,v.generation,ch.getProperty("name",juce::String()).toString().toStdString());
    }
    statusLabel.setText("PRESET APPLIED",dontSendNotification);
}
