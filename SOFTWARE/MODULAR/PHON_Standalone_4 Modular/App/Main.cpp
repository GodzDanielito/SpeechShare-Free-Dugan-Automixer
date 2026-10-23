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
    if(id>=500&&id<500+participants)return {InputKind::InternalChannelMono,id-500};
    if(id>=600&&id<600+participants)return {InputKind::InternalChannelStereo,id-600};
    if(id>=700&&id<700+sessions*groupsPerSession)return {InputKind::InternalGroupMono,id-700};
    if(id>=800&&id<800+sessions*groupsPerSession)return {InputKind::InternalGroupStereo,id-800};
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
                    message("Invalid source: internal taps must come from an EARLIER session.");
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
        reset.onClick=[this]{
            if(engine.isArmed())return;
            engine.patch().reset();
            message("Reset: S1 mono I/O 1-8, S2-S4 disabled. Audio must be rearmed manually.");
            refreshMatrix();
        };
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
        matrixContent.setSize(std::max(790,w-54),72+(participants+sessions*groupsPerSession)*rowH);
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
        if(patch)message("MONO/STEREO = one source. Any session can tap channels or group mixes of EARLIER sessions. Audio DISARMED.");
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
            // Only already-processed sessions are selectable: DAG, no feedback.
            const int destinationSession=i/participantsPerSession;
            for(int s=0;s<destinationSession;++s){
                for(int x=0;x<participantsPerSession;++x){
                    const int src=s*participantsPerSession+x;
                    const auto label="S"+juce::String(s+1)+" CH "+juce::String(x+1);
                    r.in.addItem(label+" MONO",500+src);
                    r.in.addItem(label+" STEREO",600+src);
                }
                for(int g=0;g<groupsPerSession;++g){
                    const int idx=s*groupsPerSession+g;
                    const auto label="S"+juce::String(s+1)+" GROUP "+juce::String("ABC").substring(g,g+1);
                    r.in.addItem(label+" MONO",700+idx);
                    r.in.addItem(label+" STEREO",800+idx);
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
            opts.dialogTitle="SpeechShare PHON 4 | MODULAR PATCH";
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
        for(int i=0;i<sessions;++i){
            sessionButtons[std::size_t(i)].setButtonText("SESSION "+juce::String(i+1));
            sessionButtons[std::size_t(i)].onClick=[this,i]{showSession(i+1);};
            addAndMakeVisible(sessionButtons[std::size_t(i)]);
            overviewButtons[std::size_t(i)].setButtonText("SESSION "+juce::String(i+1)+"  /  8 SOURCES");
            overviewButtons[std::size_t(i)].setTooltip("Open automixer for Session "+juce::String(i+1));
            overviewButtons[std::size_t(i)].onClick=[this,i]{showSession(i+1);};
            addAndMakeVisible(overviewButtons[std::size_t(i)]);
        }
        overview.setButtonText("OVERVIEW");addAndMakeVisible(overview);
        overview.onClick=[this]{showSession(0);};
        addAndMakeVisible(deviceLabel);
        deviceLabel.setColour(juce::Label::textColourId,juce::Colour::fromRGB(175,198,216));
        deviceLabel.setFont(juce::Font(juce::FontOptions(10.f)));
        overviewInfo.setColour(juce::Label::textColourId,juce::Colours::white);
        overviewInfo.setJustificationType(juce::Justification::centredLeft);
        overviewInfo.setFont(juce::Font(juce::FontOptions(18.f)));
        addAndMakeVisible(overviewInfo);
        overviewInfo.setText("SIGNAL FLOW  /  4 SESSIONS  /  32 PARTICIPANTS MAX",juce::dontSendNotification);
        showSession(1);
        updateRunButton();setSize(1200,730);startTimerHz(2);
    }
    ~MainPanel() override {stopTimer();engine.setArmed(false);editor.reset();}
    void paint(juce::Graphics& g) override {
        g.fillAll(juce::Colour::fromRGB(10,16,24));
        if(selectedSession==0){
            g.setFont(juce::Font(juce::FontOptions(13.f)));
            for(int i=0;i<sessions-1;++i){
                const auto& first=overviewButtons[std::size_t(i)];
                const auto& next=overviewButtons[std::size_t(i+1)];
                const int y=first.getBounds().getCentreY();
                g.setColour(juce::Colour::fromRGB(96,126,146));
                g.drawArrow(juce::Line<float>(float(first.getRight()+5),float(y),float(next.getX()-6),float(y)),
                            2.f,9.f,8.f);
            }
            g.setColour(juce::Colour::fromRGB(177,197,207));
            g.drawFittedText("FORWARD ROUTING AVAILABLE: earlier session -> later session | Individual channels / Groups A-B-C | MONO / STEREO",
                juce::Rectangle<int>(20,getHeight()/2+90,std::max(100,getWidth()-40),45),
                juce::Justification::centred,2);
            g.drawFittedText("Overview arrows indicate permitted direction, not active patch connections. Configure routing in PATCH / ASIO.",
                juce::Rectangle<int>(25,getHeight()-115,std::max(100,getWidth()-50),36),
                juce::Justification::centred,2);
        }
    }
    void resized() override {
        const int w=getWidth(),h=getHeight();
        patchButton.setBounds(10,9,124,30);runButton.setBounds(141,9,118,30);
        constexpr int firstTab=268,tabW=91,tabGap=5;
        for(int i=0;i<sessions;++i)
            sessionButtons[std::size_t(i)].setBounds(firstTab+i*(tabW+tabGap),9,tabW,30);
        const int overviewX=firstTab+sessions*(tabW+tabGap);
        overview.setBounds(overviewX,9,104,30);
        deviceLabel.setBounds(overviewX+111,9,std::max(50,w-overviewX-117),30);
        if(editor)editor->setBounds(5,47,std::max(200,w-10),std::max(190,h-52));
        overviewInfo.setBounds(38,65,std::max(200,w-76),45);
        const int pad=std::max(26,w/26);
        const int cardW=std::max(132,std::min(235,(w-2*pad-3*16)/sessions));
        const int gap=std::max(16,(w-2*pad-sessions*cardW)/(sessions-1));
        const int span=sessions*cardW+(sessions-1)*gap;
        const int left=std::max(15,(w-span)/2);
        for(int i=0;i<sessions;++i)
            overviewButtons[std::size_t(i)].setBounds(left+i*(cardW+gap),h/2-42,cardW,85);
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
        for(int i=0;i<sessions;++i){
            auto& btn=overviewButtons[std::size_t(i)];
            btn.setVisible(session==0);
            sessionButtons[std::size_t(i)].setColour(juce::TextButton::buttonColourId,
                session==i+1?juce::Colour::fromRGB(47,160,126):juce::Colours::darkslategrey);
        }
        overview.setColour(juce::TextButton::buttonColourId,
            session==0?juce::Colours::darkcyan:juce::Colours::darkslategrey);
        if(getWidth()>0&&getHeight()>0)resized();
        repaint();
    }
    void updateRunButton(){
        const bool run=engine.isArmed();runButton.setButtonText(run?"STOP AUDIO":"RUN AUDIO");
        runButton.setColour(juce::TextButton::buttonColourId,
          run?juce::Colour::fromRGB(210,65,77):juce::Colour::fromRGB(51,180,128));
    }
    void timerCallback() override {deviceLabel.setText(engine.describeDevice(),juce::dontSendNotification);}
    StandaloneAudio engine;
    std::unique_ptr<juce::AudioProcessorEditor> editor;
    juce::Label deviceLabel,overviewInfo;
    juce::TextButton patchButton,runButton,overview;
    std::array<juce::TextButton,sessions> sessionButtons,overviewButtons;
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
    const juce::String getApplicationName() override {return "SpeechShare PHON Modular 4";}
    const juce::String getApplicationVersion() override {return "0.3.0-prototype";}
    void initialise(const juce::String&) override {mainWindow.reset(new MainWindow(getApplicationName()));}
    void shutdown() override {mainWindow.reset();}
};
} // namespace speechshare_live
START_JUCE_APPLICATION(speechshare_live::StandaloneApplication)
