#include <JuceHeader.h>
#include "StandaloneAudio.h"
#include <array>
#include <algorithm>
#include <memory>

namespace speechshare_live {
class PatchScreen final : public juce::Component, private juce::ChangeListener {
public:
    explicit PatchScreen(StandaloneAudio& core):engine(core),
      deviceSettings(core.devices(),0,maxDeviceChannels,0,maxDeviceChannels,false,false,false,false){
        setSize(940,620);
        deviceTab.setButtonText("AUDIO DRIVER / BUFFER");
        patchTab.setButtonText("PATCH MATRIX");
        reset.setButtonText("RESET 1 : 1");
        save.setButtonText("SAVE PATCH");
        enableAll.setButtonText("ENABLE ALL ASIO CHANNELS");
        addAndMakeVisible(deviceTab);addAndMakeVisible(patchTab);
        addAndMakeVisible(reset);addAndMakeVisible(save);addAndMakeVisible(enableAll);
        addAndMakeVisible(status);
        status.setColour(juce::Label::textColourId,juce::Colours::whitesmoke);
        status.setJustificationType(juce::Justification::centredLeft);
        matrixContent.addAndMakeVisible(inputTitle);matrixContent.addAndMakeVisible(outputTitle);
        inputTitle.setText("INPUT MATRIX   ASIO IN  ->  SPEECHSHARE",juce::dontSendNotification);
        outputTitle.setText("OUTPUT MATRIX   SPEECHSHARE  ->  ASIO OUT",juce::dontSendNotification);
        for(auto* lbl:{&inputTitle,&outputTitle}){lbl->setColour(juce::Label::textColourId,juce::Colours::white);lbl->setFont(juce::Font(juce::FontOptions(15.f,juce::Font::bold)));}
        deviceViewport.setViewedComponent(&deviceSettings,false);
        matrixViewport.setViewedComponent(&matrixContent,false);
        deviceViewport.setScrollBarsShown(true,false);
        matrixViewport.setScrollBarsShown(true,false);
        addAndMakeVisible(deviceViewport);addAndMakeVisible(matrixViewport);
        for(int i=0;i<maxSources;++i){
            auto& row=rows[std::size_t(i)];
            row.sourceIn.setText("CH " + juce::String(i+1).paddedLeft('0',2),juce::dontSendNotification);
            row.sourceOut.setText("CH " + juce::String(i+1).paddedLeft('0',2),juce::dontSendNotification);
            for(auto* label:{&row.sourceIn,&row.sourceOut}){
                label->setColour(juce::Label::textColourId,juce::Colours::white);
                matrixContent.addAndMakeVisible(*label);
            }
            matrixContent.addAndMakeVisible(row.in);
            matrixContent.addAndMakeVisible(row.out);
            row.in.onChange=[this,i]{
                if(!updating && !engine.isArmed())engine.patch().setInput(i,rows[std::size_t(i)].in.getSelectedId()-2);
            };
            row.out.onChange=[this,i]{
                if(!updating && !engine.isArmed()){
                    engine.patch().setOutput(i,rows[std::size_t(i)].out.getSelectedId()-2);
                    refreshMatrix();
                }
            };
        }
        engine.devices().addChangeListener(this);
        deviceTab.onClick=[this]{showTab(false);};
        patchTab.onClick=[this]{showTab(true);};
        reset.onClick=[this]{engine.patch().resetOneToOne();refreshMatrix();};
        enableAll.onClick=[this]{
            if(auto* device=engine.devices().getCurrentAudioDevice()){
                auto setup=engine.devices().getAudioDeviceSetup();
                setup.useDefaultInputChannels=false;
                setup.useDefaultOutputChannels=false;
                setup.inputChannels.clear();setup.outputChannels.clear();
                setup.inputChannels.setRange(0,std::min(maxDeviceChannels,device->getInputChannelNames().size()),true);
                setup.outputChannels.setRange(0,std::min(maxDeviceChannels,device->getOutputChannelNames().size()),true);
                const auto err=engine.devices().setAudioDeviceSetup(setup,true);
                if(err.isNotEmpty())juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,"Audio device setup",err);
            }
        };
        save.onClick=[this]{
            engine.saveConfiguration();
            if(auto* window=findParentComponentOfClass<juce::DialogWindow>())window->exitModalState(0);
        };
        showTab(true);
        refreshMatrix();
    }
    ~PatchScreen() override{engine.devices().removeChangeListener(this);}
    void paint(juce::Graphics& g) override{
        g.fillAll(juce::Colour::fromRGB(15,23,34));
        g.setColour(juce::Colour::fromRGB(58,78,98));
        g.drawRect(getLocalBounds(),1);
    }
    void resized() override{
        const int w=getWidth(),h=getHeight();
        deviceTab.setBounds(14,12,210,31);
        patchTab.setBounds(234,12,160,31);
        status.setBounds(14,46,w-28,33);
        deviceViewport.setBounds(14,83,w-28,h-140);
        matrixViewport.setBounds(14,83,w-28,h-140);
        deviceSettings.setBounds(0,0,std::max(620,w-55),std::max(550,h-125));
        reset.setBounds(15,h-45,140,31);
        enableAll.setBounds(165,h-45,250,31);
        save.setBounds(w-165,h-45,150,31);
        const int rowH=36;
        matrixContent.setSize(std::max(740,w-52),70+engine.sourceCount()*rowH);
        const int half=matrixContent.getWidth()/2;
        inputTitle.setBounds(8,12,half-12,32);
        outputTitle.setBounds(half+4,12,half-12,32);
        for(int i=0;i<maxSources;++i){
            const bool shown=i<engine.sourceCount();
            auto& r=rows[std::size_t(i)];
            for(auto* c:{static_cast<juce::Component*>(&r.sourceIn),static_cast<juce::Component*>(&r.sourceOut),
                        static_cast<juce::Component*>(&r.in),static_cast<juce::Component*>(&r.out)})c->setVisible(shown);
            if(!shown)continue;
            const int y=55+i*rowH;
            r.sourceIn.setBounds(8,y,88,28);
            r.in.setBounds(101,y,std::max(120,half-115),28);
            r.sourceOut.setBounds(half+4,y,88,28);
            r.out.setBounds(half+96,y,std::max(120,half-112),28);
        }
    }
private:
    struct Row{juce::Label sourceIn,sourceOut;juce::ComboBox in,out;};
    void changeListenerCallback(juce::ChangeBroadcaster*) override {refreshMatrix();}
    void showTab(bool patch){
        isPatch=patch;
        matrixViewport.setVisible(isPatch);
        deviceViewport.setVisible(!isPatch);
        reset.setVisible(isPatch);
        enableAll.setVisible(!isPatch);
        status.setText(isPatch?
            "MONO routing. OFF disables a source. Enable I/O ports in AUDIO DEVICE. Output is exclusive. Audio DISARMED.":
            "Select ASIO, choose the device, sample rate, enabled channels and buffer. Restart audio manually after editing.",
            juce::dontSendNotification);
        deviceTab.setColour(juce::TextButton::buttonColourId,!patch?juce::Colour::fromRGB(49,174,218):juce::Colour::fromRGB(42,59,75));
        patchTab.setColour(juce::TextButton::buttonColourId,patch?juce::Colour::fromRGB(49,174,218):juce::Colour::fromRGB(42,59,75));
    }
    void refreshMatrix(){
        updating=true;
        auto* device=engine.devices().getCurrentAudioDevice();
        const int ins=device?std::min(maxDeviceChannels,device->getInputChannelNames().size()):0;
        const int outs=device?std::min(maxDeviceChannels,device->getOutputChannelNames().size()):0;
        for(int i=0;i<engine.sourceCount();++i){
            auto& row=rows[std::size_t(i)];
            row.in.clear(juce::dontSendNotification);row.out.clear(juce::dontSendNotification);
            row.in.addItem("OFF",1);row.out.addItem("OFF",1);
            for(int c=0;c<ins;++c)row.in.addItem("IN  "+juce::String(c+1).paddedLeft('0',2),c+2);
            for(int c=0;c<outs;++c)row.out.addItem("OUT "+juce::String(c+1).paddedLeft('0',2),c+2);
            const int input=engine.patch().getInput(i),output=engine.patch().getOutput(i);
            row.in.setSelectedId(input>=0&&input<ins?input+2:1,juce::dontSendNotification);
            row.out.setSelectedId(output>=0&&output<outs?output+2:1,juce::dontSendNotification);
        }
        updating=false;
        resized();
    }
    StandaloneAudio& engine;
    juce::TextButton deviceTab,patchTab,reset,save,enableAll;
    juce::Label status,inputTitle,outputTitle;
    juce::AudioDeviceSelectorComponent deviceSettings;
    juce::Viewport deviceViewport,matrixViewport;
    juce::Component matrixContent;
    std::array<Row,maxSources> rows;
    bool isPatch=true,updating=false;
};

class MainPanel final : public juce::Component, private juce::Timer {
public:
    MainPanel(){
        addAndMakeVisible(patchButton);
        patchButton.setButtonText("PATCH / ASIO");
        patchButton.onClick=[this]{
            engine.setArmed(false);
            updateRunButton();
            juce::DialogWindow::LaunchOptions options;
            options.dialogTitle="SpeechShare | PATCH & AUDIO DRIVER";
            options.dialogBackgroundColour=juce::Colour::fromRGB(14,22,33);
            options.escapeKeyTriggersCloseButton=true;
            options.useNativeTitleBar=true;
            options.resizable=true;
            options.componentToCentreAround=this;
            options.content.setOwned(new PatchScreen(engine));
            options.launchAsync();
        };
        addAndMakeVisible(runButton);
        runButton.onClick=[this]{
            if(!engine.isAudioReady() || engine.devices().getCurrentAudioDevice()==nullptr){
                juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,"No audio device",
                    "Choose a valid ASIO audio device in PATCH / ASIO before enabling audio.");
                return;
            }
            engine.setArmed(!engine.isArmed());updateRunButton();
        };
        addAndMakeVisible(countSelect);
        for(auto n:{1,2,4,8,12,16,24,32})countSelect.addItem(juce::String(n)+" CH",n);
        countSelect.setSelectedId(engine.sourceCount(),juce::dontSendNotification);
        countSelect.onChange=[this]{
            auto count=countSelect.getSelectedId();
            if(count<1 || count>maxSources || count==engine.sourceCount())return;
            editor.reset();engine.setSourceCount(count);attachEditor();
            updateRunButton();resized();
        };
        addAndMakeVisible(deviceLabel);
        deviceLabel.setColour(juce::Label::textColourId,juce::Colour::fromRGB(175,198,216));
        deviceLabel.setFont(juce::Font(juce::FontOptions(11.f)));
        attachEditor();updateRunButton();
        setSize(1120,690);
        startTimerHz(2);
    }
    ~MainPanel() override{stopTimer();engine.setArmed(false);editor.reset();}
    void paint(juce::Graphics& g) override {g.fillAll(juce::Colour::fromRGB(10,16,24));}
    void resized() override{
        patchButton.setBounds(12,10,145,31);
        runButton.setBounds(166,10,140,31);
        countSelect.setBounds(316,10,99,31);
        deviceLabel.setBounds(425,10,std::max(80,getWidth()-437),31);
        if(editor)editor->setBounds(5,51,std::max(100,getWidth()-10),std::max(200,getHeight()-57));
    }
private:
    void attachEditor(){
        if(auto* p=engine.primary()){
            editor.reset(p->createEditor());
            addAndMakeVisible(*editor);
            if(getWidth()>0&&getHeight()>0)resized();
        }
    }
    void updateRunButton(){
        const bool run=engine.isArmed();
        runButton.setButtonText(run?"STOP AUDIO":"RUN AUDIO");
        runButton.setColour(juce::TextButton::buttonColourId,
             run?juce::Colour::fromRGB(210,65,77):juce::Colour::fromRGB(51,180,128));
    }
    void timerCallback() override{deviceLabel.setText(engine.describeDevice(),juce::dontSendNotification);}
    StandaloneAudio engine;
    std::unique_ptr<juce::AudioProcessorEditor> editor;
    juce::TextButton patchButton,runButton;
    juce::ComboBox countSelect;
    juce::Label deviceLabel;
};

class StandaloneApplication final : public juce::JUCEApplication {
private:
    // Define MainWindow completely *before* creating its unique_ptr.
    // This also avoids an MSVC template-instantiation issue reported in VS 2026.
    class MainWindow final : public juce::DocumentWindow {
    public:
        explicit MainWindow(const juce::String& title)
            : juce::DocumentWindow(title,
                                   juce::Colour::fromRGB(11, 17, 25),
                                   juce::DocumentWindow::allButtons) {
            setUsingNativeTitleBar(true);
            setContentOwned(new MainPanel(), true);
            setResizable(true, true);

            int width = 1120;
            int height = 720;
            if (const auto* display = juce::Desktop::getInstance()
                                          .getDisplays().getPrimaryDisplay()) {
                // JUCE Display::userBounds may have floating-point coordinates.
                // Keep the std::min / std::max operands consistently typed.
                const int availableWidth  = static_cast<int>(display->userBounds.getWidth())  - 50;
                const int availableHeight = static_cast<int>(display->userBounds.getHeight()) - 50;
                width  = std::min(width,  std::max(780, availableWidth));
                height = std::min(height, std::max(530, availableHeight));
            }
            centreWithSize(width, height);
            setVisible(true);
        }

        void closeButtonPressed() override {
            juce::JUCEApplication::getInstance()->systemRequestedQuit();
        }
    };

    std::unique_ptr<MainWindow> mainWindow;

public:
    const juce::String getApplicationName() override {
        return "SpeechShare Standalone PHON";
    }

    const juce::String getApplicationVersion() override {
        return "0.1.0-phon-preview";
    }

    void initialise(const juce::String&) override {
        mainWindow.reset(new MainWindow(getApplicationName()));
    }

    void shutdown() override {
        mainWindow.reset();
    }
};
}
START_JUCE_APPLICATION(speechshare_live::StandaloneApplication)
