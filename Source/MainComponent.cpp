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

    // Make sure you set the size of the component after
    // you add any child components.
    setSize (300, 200);

    formatManager.registerBasicFormats();   // [1]
    transportSource.addChangeListener(this);// [2]

    // Specify the number of input and output channels that we want to open
    setAudioChannels(0, 2);
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

void MainComponent::changeState(TransportState newState) {
    if (state != newState) {
        state = newState;
        switch (state) {
        case MainComponent::STOPPED:  //[3]
            playButton.setButtonText("Play");
            stopButton.setButtonText("Stop");
            stopButton.setEnabled(false);
            transportSource.setPosition(0.0);
            break;

        case MainComponent::STARTING: //[4]
            transportSource.start();
            break;

        case MainComponent::PLAYING:  //[5]
            playButton.setButtonText("Pause");
            stopButton.setButtonText("Stop");
            stopButton.setEnabled(true);
            break;

        case MainComponent::PAUSING: //[6]
            transportSource.stop();
            break;

        case MainComponent::PAUSED:  //[5]
            playButton.setButtonText("Resume");
            stopButton.setButtonText("Return to Zero");
            break;

        case MainComponent::STOPPING: //[6]
            transportSource.stop();
            break;
        }
    }
}

void MainComponent::openButtonClicked() {
    chooser = std::make_unique<juce::FileChooser>("Select a Wave file to play...", juce::File{}, "*.wav");
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
