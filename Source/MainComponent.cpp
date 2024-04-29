#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent()
    : state(STOPPED) {
    addAndMakeVisible(&openButton);
    openButton.setButtonText("Open...");
    openButton.onClick = [this] {openButtonClicked();};

    addAndMakeVisible(&playButton);
    playButton.setButtonText("Play");
    playButton.onClick = [this] {playButtonClicked();};
    playButton.setColour(juce::TextButton::buttonColourId, juce::Colours::green);
    playButton.setEnabled(false);

    addAndMakeVisible(&stopButton);
    stopButton.setButtonText("Stop");
    stopButton.onClick = [this] {stopButtonClicked();};
    stopButton.setColour(juce::TextButton::buttonColourId, juce::Colours::red);
    stopButton.setEnabled(false);

    addAndMakeVisible(&loopingToggle);
    loopingToggle.setButtonText("Loop");
    loopingToggle.onClick = [this] {loopButtonChanged();};

    addAndMakeVisible(&currentPositionLabel);
    currentPositionLabel.setText("Stopped", juce::dontSendNotification);

    // Make sure you set the size of the component after
    // you add any child components.
    setSize (300, 200);

    formatManager.registerBasicFormats();  
    transportSource.addChangeListener(this);

    // Specify the number of input and output channels that we want to open
    setAudioChannels(0, 2);
    startTimer(20);
}

MainComponent::~MainComponent() {
    // This shuts down the audio device and clears the audio source.
    shutdownAudio();
}

//==============================================================================
void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate) {
    transportSource.prepareToPlay(samplesPerBlockExpected, sampleRate);
}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) {
    if (readerSource.get() == nullptr) {
        bufferToFill.clearActiveBufferRegion();
        return;
    }

    transportSource.getNextAudioBlock(bufferToFill);
}

void MainComponent::releaseResources() {
    transportSource.releaseResources();
}

//==============================================================================
void MainComponent::paint (juce::Graphics& g) {
    // fill black
    g.fillAll(juce::Colours::black);

}

void MainComponent::resized() {
    openButton.setBounds(10, 10, getWidth() - 20, 20);
    playButton.setBounds(10, 40, getWidth() - 20, 20);
    stopButton.setBounds(10, 70, getWidth() - 20, 20);
    loopingToggle.setBounds(10, 100, getWidth() - 20, 20);
    currentPositionLabel.setBounds(10, 130, getWidth() - 20, 20);
}

void MainComponent::changeListenerCallback(juce::ChangeBroadcaster* source) {
    if (source == &transportSource) {
        if (transportSource.isPlaying()) {
            changeState(PLAYING);
        } else if ((state == STOPPING) || (state == PLAYING)) {
            changeState(STOPPED);
        } else if (state == PAUSING) {
            changeState(PAUSED);
        }
    }
}

void MainComponent::timerCallback() {
    if (transportSource.isPlaying()) {
        juce::RelativeTime position(transportSource.getCurrentPosition());

        auto minutes = ((int)position.inMinutes()) % 60;
        auto seconds = ((int)position.inSeconds()) % 60;
        auto millis = ((int)position.inMilliseconds()) % 1000;

        auto positionString = juce::String::formatted("%02d:%02d:%03d", minutes, seconds, millis);

        currentPositionLabel.setText(positionString, juce::dontSendNotification);
    } else {
        currentPositionLabel.setText("Stopped", juce::dontSendNotification);
    }
}

void MainComponent::updateLoopState(bool shouldLoop) {
    if (readerSource.get() != nullptr) {
        readerSource->setLooping(shouldLoop);
    }
}

void MainComponent::changeState(TransportState newState) {
    if (state != newState) {
        state = newState;
        switch (state) {
        case MainComponent::STOPPED: 
            playButton.setButtonText("Play");
            stopButton.setButtonText("Stop");
            stopButton.setEnabled(false);
            transportSource.setPosition(0.0);
            break;

        case MainComponent::STARTING: 
            transportSource.start();
            break;

        case MainComponent::PLAYING:  
            playButton.setButtonText("Pause");
            stopButton.setButtonText("Stop");
            stopButton.setEnabled(true);
            break;

        case MainComponent::PAUSING: 
            transportSource.stop();
            break;

        case MainComponent::PAUSED:
            playButton.setButtonText("Resume");
            stopButton.setButtonText("Return to Zero");
            break;

        case MainComponent::STOPPING: 
            transportSource.stop();
            break;
        }
    }
}

void MainComponent::openButtonClicked() {
    chooser = std::make_unique<juce::FileChooser>("Select a Wave file to play...", juce::File{}, "*.wav;*.aif;*.aiff");
    auto chooserFlags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;

    chooser->launchAsync(chooserFlags, [this](const juce::FileChooser& fc) {
        auto file = fc.getResult();

        if (file != juce::File{}) {
            auto* reader = formatManager.createReaderFor(file);

            if (reader != nullptr) {
                auto newSource = std::make_unique<juce::AudioFormatReaderSource>(reader, true);
                transportSource.setSource(newSource.get(), 0, nullptr, reader->sampleRate);
                playButton.setEnabled(true);
                readerSource.reset(newSource.release());
            }
        }
    });
}

void MainComponent::playButtonClicked() {
    if ((state == STOPPED) || (state == PAUSED)) {
        updateLoopState(loopingToggle.getToggleState());
        changeState(STARTING);
    } else if (state == PLAYING) {
        changeState(PAUSING);
    }
}

void MainComponent::stopButtonClicked() {
    if (state == PAUSED) {
        changeState(STOPPED);
    } else {
        changeState(STOPPING);
    }
}

void MainComponent::loopButtonChanged() {
    updateLoopState(loopingToggle.getToggleState());
}
