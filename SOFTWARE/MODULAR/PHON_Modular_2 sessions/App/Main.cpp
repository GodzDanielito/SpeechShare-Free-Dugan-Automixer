#include <JuceHeader.h>
#include "StandaloneAudio.h"
#include <array>
#include <algorithm>
#include <memory>

namespace speechshare_live {
using namespace speechshare_modular;
namespace {
constexpr int inputOff=1;
int inputId(InputRoute r) noexcept {
    switch(r.kind){
        case InputKind::AsioMono: return 100+r.index;
        case InputKind::AsioStereo: return 300+r.index;
        case InputKind::Session1ChannelMono: return 500+r.index;
        case InputKind::Session1ChannelStereo: return 600+r.index;
        case InputKind::Session1GroupMono: return 700+r.index;
        case InputKind::Session1GroupStereo: return 800+r.index;
        default:return inputOff;
    }
}
InputRoute inputRoute(int id) noexcept {
    if(id>=100&&id<100+maxPorts)return {InputKind::AsioMono,id-100};
    if(id>=300&&id<300+maxPorts)return {InputKind::AsioStereo,id-300};
    if(id>=500&&id<508)return {InputKind::Session1ChannelMono,id-500};
    if(id>=600&&id<608)return {InputKind::Session1ChannelStereo,id-600};
    if(id>=700&&id<703)return {InputKind::Session1GroupMono,id-700};
    if(id>=800&&id<803)return {InputKind::Session1GroupStereo,id-800};
    return {};
}
int outputId(OutputRoute r) noexcept {
    return r.kind==OutputKind::AsioMono?100+r.index:(r.kind==OutputKind::AsioStereo?300+r.index:1);
}
OutputRoute outputRoute(int id) noexcept {
    if(id>=100&&id<100+maxPorts)return {OutputKind::AsioMono,id-100};
    if(id>=300&&id<300+maxPorts)return {OutputKind::AsioStereo,id-300};
    return {};
}
}
class PatchScreen final:public juce::Component,private juce::ChangeListener {
public:
    explicit PatchScreen(StandaloneAudio& core):engine(core),
      deviceSettings(core.devices(),0,maxDeviceChannels,0,maxDeviceChannels,false,false,false,false){
        setSize(1020,700);
        deviceTab.setButtonText("AUDIO DRIVER / BUFFER");patchTab.setButtonText("PATCH MATRIX");
        reset.setButtonText("RESET SAFE 1:1");save.setButtonText("SAVE PATCH");
        enableAll.setButtonText("ENABLE ALL ASIO CHANNELS");
        for(auto* b:{&deviceTab,&patchTab,&reset,&save,&enableAll})addAndMakeVisible(*b);
        addAndMakeVisible(status);
        status.setColour(juce::Label::textColourId,juce::Colours::whitesmoke);
        deviceViewport.setViewedComponent(&deviceSettings,false);
        matrixViewport.setViewedComponent(&matrixContent,false);
        deviceViewport.setScrollBarsShown(true,false);matrixViewport.setScrollBarsShown(true,false);
        addAndMakeVisible(deviceViewport);addAndMakeVisible(matrixViewport);
        matrixContent.addAndMakeVisible(inputTitle);matrixContent.addAndMakeVisible(outputTitle);
        inputTitle.setText("INPUT PATCH  >  SPEECHSHARE",juce::dontSendNotification);
        outputTitle.setText("SPEECHSHARE  >  OUTPUT PATCH",juce::dontSendNotification);
        for(auto* lbl:{&inputTitle,&outputTitle}){
            lbl->setColour(juce::Label::textColourId,juce::Colours::white);
            lbl->setFont(juce::Font(juce::FontOptions(14.f,juce::Font::bold)));
        }
        for(int i=0;i<participants;++i){
            auto& row=rows[std::size_t(i)];
            row.left.setText("S"+juce::String(i/8+1)+" CH "+juce::String(i%8+1).paddedLeft('0',2),juce::dontSendNotification);
            row.right.setText(row.left.getText(),juce::dontSendNotification);
            for(auto* l:{&row.left,&row.right}){
                l->setColour(juce::Label::textColourId,juce::Colours::white);
                matrixContent.addAndMakeVisible(*l);
            }
            matrixContent.addAndMakeVisible(row.in);matrixContent.addAndMakeVisible(row.out);
            row.in.onChange=[this,i]{
                if(updating || engine.isArmed())return;
                if(!engine.patch().setInput(i,inputRoute(rows[std::size_t(i)].in.getSelectedId())))
                    message("Input not valid: Session 2 can receive only Session 1 internal taps.");
                refreshMatrix();
            };
            row.out.onChange=[this,i]{
                if(updating || engine.isArmed())return;
                if(!engine.patch().setOutput(i,outputRoute(rows[std::size_t(i)].out.getSelectedId())))
                    message("Output port is already assigned to another source/bus.");
                refreshMatrix();
            };
        }
        for(int i=0;i<sessions*groupsPerSession;++i){
            auto& row=groupRows[std::size_t(i)];
            row.left.setText("S"+juce::String(i/3+1)+" GROUP "+juce::String("ABC").substring(i%3,i%3+1),juce::dontSendNotification);
            row.left.setColour(juce::Label::textColourId,juce::Colours::white);
            matrixContent.addAndMakeVisible(row.left);matrixContent.addAndMakeVisible(row.out);
            row.out.onChange=[this,i]{
                if(updating||engine.isArmed())return;
                if(!engine.patch().setGroupOutput(i/3,i%3,outputRoute(groupRows[std::size_t(i)].out.getSelectedId())))
                    message("Group output port is already assigned.");
                refreshMatrix();
            };
        }
        engine.devices().addChangeListener(this);
        deviceTab.onClick=[this]{showTab(false);};patchTab.onClick=[this]{showTab(true);};
        reset.onClick=[this]{engine.patch().reset();message("Reset: Session 1 mono IN/OUT 1-8; Session 2 off.");refreshMatrix();};
        enableAll.onClick=[this]{
            if(auto* dev=engine.devices().getCurrentAudioDevice()){
                auto setup=engine.devices().getAudioDeviceSetup();
                setup.useDefaultInputChannels=false;setup.useDefaultOutputChannels=false;
                setup.inputChannels.clear();setup.outputChannels.clear();
                setup.inputChannels.setRange(0,std::min(maxDeviceChannels,dev->getInputChannelNames().size()),true);
                setup.outputChannels.setRange(0,std::min(maxDeviceChannels,dev->getOutputChannelNames().size()),true);
                auto err=engine.devices().setAudioDeviceSetup(setup,true);
                if(err.isNotEmpty())juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,"Device setup",err);
            }
        };
        save.onClick=[this]{
            engine.saveConfiguration();
            if(auto* w=findParentComponentOfClass<juce::DialogWindow>())w->exitModalState(0);
        };
        showTab(true);refreshMatrix();
    }
    ~PatchScreen() override {engine.devices().removeChangeListener(this);}
    void paint(juce::Graphics& g) override {g.fillAll(juce::Colour::fromRGB(15,23,34));}
    void resized() override {
        int w=getWidth(),h=getHeight();
        deviceTab.setBounds(14,12,210,31);patchTab.setBounds(234,12,165,31);
        status.setBounds(14,47,w-28,40);
        deviceViewport.setBounds(14,90,w-28,h-152);
        matrixViewport.setBounds(14,90,w-28,h-152);
        deviceSettings.setBounds(0,0,std::max(680,w-55),std::max(550,h-125));
        reset.setBounds(15,h-45,158,31);
        enableAll.setBounds(185,h-45,250,31);
        save.setBounds(w-165,h-45,150,31);
        constexpr int rowH=34;
        matrixContent.setSize(std::max(790,w-54),72+(participants+6)*rowH);
        const int half=matrixContent.getWidth()/2;
        inputTitle.setBounds(8,10,half-15,31);outputTitle.setBounds(half+5,10,half-18,31);
        for(int i=0;i<participants;++i){
            const int y=48+i*rowH;
            auto& r=rows[std::size_t(i)];
            r.left.setBounds(8,y,92,27);r.in.setBounds(103,y,half-116,27);
            r.right.setBounds(half+6,y,92,27);r.out.setBounds(half+103,y,half-118,27);
        }
        for(int i=0;i<sessions*groupsPerSession;++i){
            const int y=60+(participants+i)*rowH;
            auto& r=groupRows[std::size_t(i)];
            r.left.setBounds(half+6,y,112,27);
            r.out.setBounds(half+121,y,half-135,27);
        }
    }
private:
    struct Row {juce::Label left,right;juce::ComboBox in,out;};
    struct GroupRow {juce::Label left;juce::ComboBox out;};
    void changeListenerCallback(juce::ChangeBroadcaster*) override {refreshMatrix();}
    void message(juce::String text){status.setText(text,juce::dontSendNotification);}
    void showTab(bool patch){
        isPatch=patch;
        matrixViewport.setVisible(patch);deviceViewport.setVisible(!patch);
        reset.setVisible(patch);enableAll.setVisible(!patch);
        if(patch)message("MONO or STEREO = one automix source. Session 2 can tap S1 channels and group mixes. Audio DISARMED.");
        else message("Choose ASIO, device, sample rate, buffer and enable I/O ports. Audio must be restarted manually.");
        deviceTab.setColour(juce::TextButton::buttonColourId,!patch?juce::Colour::fromRGB(49,174,218):juce::Colour::fromRGB(42,59,75));
        patchTab.setColour(juce::TextButton::buttonColourId,patch?juce::Colour::fromRGB(49,174,218):juce::Colour::fromRGB(42,59,75));
    }
    void fillOutputs(juce::ComboBox& cb,int count){
        cb.clear(juce::dontSendNotification);cb.addItem("OFF",1);
        for(int i=0;i<count;++i)cb.addItem("ASIO OUT "+juce::String(i+1),100+i);
        for(int i=0;i<count-1;++i)cb.addItem("STEREO OUT "+juce::String(i+1)+"+"+juce::String(i+2),300+i);
    }
    void refreshMatrix(){
        updating=true;
        auto* dev=engine.devices().getCurrentAudioDevice();
        const int ins=dev?std::min(maxDeviceChannels,dev->getInputChannelNames().size()):0;
        const int outs=dev?std::min(maxDeviceChannels,dev->getOutputChannelNames().size()):0;
        for(int i=0;i<participants;++i){
            auto& r=rows[std::size_t(i)];
            r.in.clear(juce::dontSendNotification);r.in.addItem("OFF",inputOff);
            for(int x=0;x<ins;++x)r.in.addItem("ASIO IN "+juce::String(x+1)+" (MONO)",100+x);
            for(int x=0;x<ins-1;++x)
                r.in.addItem("ASIO IN "+juce::String(x+1)+"+"+juce::String(x+2)+" (STEREO)",300+x);
            if(i>=8){
                for(int x=0;x<8;++x){
                    r.in.addItem("S1 CH "+juce::String(x+1)+" MONO",500+x);
                    r.in.addItem("S1 CH "+juce::String(x+1)+" STEREO",600+x);
                }
                for(int g=0;g<3;++g){
                    r.in.addItem("S1 GROUP "+juce::String("ABC").substring(g,g+1)+" MONO",700+g);
                    r.in.addItem("S1 GROUP "+juce::String("ABC").substring(g,g+1)+" STEREO",800+g);
                }
            }
            fillOutputs(r.out,outs);
            const auto inRoute=engine.patch().input(i);
            const auto outRoute=engine.patch().output(i);
            if((inRoute.kind==InputKind::AsioMono || inRoute.kind==InputKind::AsioStereo) &&
               inRoute.index+(ModularPatch::isStereo(inRoute.kind)?1:0)>=ins)
                r.in.addItem("IN "+juce::String(inRoute.index+1)+" (PORT INACTIVE)",inputId(inRoute));
            if(outRoute.kind!=OutputKind::Off &&
               outRoute.index+(ModularPatch::isStereo(outRoute.kind)?1:0)>=outs)
                r.out.addItem("OUT "+juce::String(outRoute.index+1)+" (PORT INACTIVE)",outputId(outRoute));
            r.in.setSelectedId(inputId(inRoute),juce::dontSendNotification);
            r.out.setSelectedId(outputId(outRoute),juce::dontSendNotification);
        }
        for(int i=0;i<sessions*groupsPerSession;++i){
            auto& cb=groupRows[std::size_t(i)].out;
            fillOutputs(cb,outs);
            auto route=engine.patch().groupOutput(i/3,i%3);
            if(route.kind!=OutputKind::Off && route.index+(ModularPatch::isStereo(route.kind)?1:0)>=outs)
                cb.addItem("OUT "+juce::String(route.index+1)+" (PORT INACTIVE)",outputId(route));
            cb.setSelectedId(outputId(route),juce::dontSendNotification);
        }
        updating=false;resized();
    }
    StandaloneAudio& engine;
    juce::TextButton deviceTab,patchTab,reset,save,enableAll;
    juce::Label status,inputTitle,outputTitle;
    juce::AudioDeviceSelectorComponent deviceSettings;
    juce::Viewport deviceViewport,matrixViewport;
    juce::Component matrixContent;
    std::array<Row,participants> rows;
    std::array<GroupRow,sessions*groupsPerSession> groupRows;
    bool isPatch=true,updating=false;
};

class MainPanel final:public juce::Component,private juce::Timer {
public:
    MainPanel(){
        patchButton.setButtonText("PATCH / ASIO");addAndMakeVisible(patchButton);
        patchButton.onClick=[this]{
            engine.setArmed(false);updateRunButton();
            juce::DialogWindow::LaunchOptions opts;
            opts.dialogTitle="SpeechShare PHON | MODULAR PATCH";
            opts.dialogBackgroundColour=juce::Colour::fromRGB(14,22,33);
            opts.escapeKeyTriggersCloseButton=true;opts.useNativeTitleBar=true;
            opts.resizable=true;opts.componentToCentreAround=this;
            opts.content.setOwned(new PatchScreen(engine));opts.launchAsync();
        };
        addAndMakeVisible(runButton);
        runButton.onClick=[this]{
            if(!engine.isAudioReady()||!engine.devices().getCurrentAudioDevice()){
                juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,"No audio device",
                    "Select a working ASIO device under PATCH / ASIO before enabling audio.");return;
            }
            engine.setArmed(!engine.isArmed());updateRunButton();
        };
        session1.setButtonText("SESSION 1");session2.setButtonText("SESSION 2");overview.setButtonText("OVERVIEW");
        for(auto* b:{&session1,&session2,&overview})addAndMakeVisible(*b);
        session1.onClick=[this]{showSession(1);};session2.onClick=[this]{showSession(2);};
        overview.onClick=[this]{showSession(0);};
        addAndMakeVisible(deviceLabel);
        deviceLabel.setColour(juce::Label::textColourId,juce::Colour::fromRGB(175,198,216));
        deviceLabel.setFont(juce::Font(juce::FontOptions(10.f)));
        overviewInfo.setColour(juce::Label::textColourId,juce::Colours::white);
        overviewInfo.setJustificationType(juce::Justification::centredLeft);
        overviewInfo.setFont(juce::Font(juce::FontOptions(18.f)));
        addAndMakeVisible(overviewInfo);
        overviewInfo.setText("SIGNAL FLOW   /   2 SESSIONS   /   16 PARTICIPANTS MAX",juce::dontSendNotification);
        overviewSession1.setButtonText("SESSION 1  /  8 SOURCES");
        overviewSession2.setButtonText("SESSION 2  /  8 SOURCES");
        overviewSession1.setTooltip("Open Session 1 automixer");
        overviewSession2.setTooltip("Open Session 2 automixer");
        overviewSession1.onClick=[this]{showSession(1);};
        overviewSession2.onClick=[this]{showSession(2);};
        addAndMakeVisible(overviewSession1);addAndMakeVisible(overviewSession2);
        showSession(1);
        updateRunButton();setSize(1200,730);startTimerHz(2);
    }
    ~MainPanel() override {stopTimer();engine.setArmed(false);editor.reset();}
    void paint(juce::Graphics& g) override {
        g.fillAll(juce::Colour::fromRGB(10,16,24));
        if(selectedSession==0){
            const int x1=overviewSession1.getRight(),x2=overviewSession2.getX();
            const int cy=overviewSession1.getBounds().getCentreY();
            g.setColour(juce::Colour::fromRGB(63,189,226));
            g.drawArrow(juce::Line<float>(float(x1+8),float(cy),float(x2-9),float(cy)),
                        2.5f,13.f,11.f);
            g.setFont(juce::Font(juce::FontOptions(12.f)));
            g.drawFittedText("S1 CH  /  S1 GROUP A-B-C   (MONO OR STEREO)",
                juce::Rectangle<int>(x1+5,cy-52,std::max(20,x2-x1-10),35),
                juce::Justification::centred,2);
            g.setColour(juce::Colours::lightgrey);
            g.drawFittedText("PATCH hardware outputs and group mixes independently.",
                juce::Rectangle<int>(30,getHeight()-120,std::max(100,getWidth()-60),30),
                juce::Justification::centred,1);
        }
    }
    void resized() override {
        const int w=getWidth(),h=getHeight();
        patchButton.setBounds(10,9,130,30);runButton.setBounds(148,9,130,30);
        session1.setBounds(286,9,99,30);session2.setBounds(391,9,99,30);overview.setBounds(496,9,110,30);
        deviceLabel.setBounds(613,9,std::max(50,w-620),30);
        if(editor)editor->setBounds(5,47,std::max(200,w-10),std::max(190,h-52));
        overviewInfo.setBounds(55,65,std::max(200,w-105),45);
        const int cardW=std::max(170,std::min(290,(w-180)/3));
        const int middle=std::max(130,std::min(340,w-2*cardW-70));
        const int left=std::max(25,(w-(2*cardW+middle))/2);
        overviewSession1.setBounds(left,h/2-42,cardW,85);
        overviewSession2.setBounds(left+cardW+middle,h/2-42,cardW,85);
    }
private:
    void showSession(int session){
        selectedSession=session;
        editor.reset();
        if(session>0){
            if(auto* p=engine.primary(session)){
                editor.reset(p->createEditor());addAndMakeVisible(*editor);
            }
        }
        overviewInfo.setVisible(session==0);
        overviewSession1.setVisible(session==0);
        overviewSession2.setVisible(session==0);
        repaint();
        session1.setColour(juce::TextButton::buttonColourId,session==1?juce::Colours::seagreen:juce::Colours::darkslategrey);
        session2.setColour(juce::TextButton::buttonColourId,session==2?juce::Colours::royalblue:juce::Colours::darkslategrey);
        overview.setColour(juce::TextButton::buttonColourId,session==0?juce::Colours::darkcyan:juce::Colours::darkslategrey);
        if(getWidth()>0&&getHeight()>0)resized();
    }
    void updateRunButton(){
        bool run=engine.isArmed();runButton.setButtonText(run?"STOP AUDIO":"RUN AUDIO");
        runButton.setColour(juce::TextButton::buttonColourId,
          run?juce::Colour::fromRGB(210,65,77):juce::Colour::fromRGB(51,180,128));
    }
    void timerCallback() override {deviceLabel.setText(engine.describeDevice(),juce::dontSendNotification);}
    StandaloneAudio engine;
    std::unique_ptr<juce::AudioProcessorEditor> editor;
    juce::Label deviceLabel,overviewInfo;
    juce::TextButton patchButton,runButton,session1,session2,overview,overviewSession1,overviewSession2;
    int selectedSession=1;
};

class StandaloneApplication final:public juce::JUCEApplication {
private:
    class MainWindow final:public juce::DocumentWindow {
    public:
        explicit MainWindow(const juce::String& title):
          juce::DocumentWindow(title,juce::Colour::fromRGB(11,17,25),juce::DocumentWindow::allButtons){
            setUsingNativeTitleBar(true);setContentOwned(new MainPanel(),true);setResizable(true,true);
            int w=1200,h=750;
            if(const auto* display=juce::Desktop::getInstance().getDisplays().getPrimaryDisplay()){
                w=std::min(w,std::max(820,int(display->userBounds.getWidth())-40));
                h=std::min(h,std::max(550,int(display->userBounds.getHeight())-40));
            }
            centreWithSize(w,h);setVisible(true);
        }
        void closeButtonPressed() override {juce::JUCEApplication::getInstance()->systemRequestedQuit();}
    };
    std::unique_ptr<MainWindow> mainWindow;
public:
    const juce::String getApplicationName() override {return "SpeechShare PHON Modular";}
    const juce::String getApplicationVersion() override {return "0.2.0-prototype";}
    void initialise(const juce::String&) override {mainWindow.reset(new MainWindow(getApplicationName()));}
    void shutdown() override {mainWindow.reset();}
};
} // namespace speechshare_live
START_JUCE_APPLICATION(speechshare_live::StandaloneApplication)
