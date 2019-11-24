/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#include "MainComponent.h"

StringSynthesiser *strynth;

//==============================================================================
MainComponent::MainComponent()
{
    // Make sure you set the size of the component after
    // you add any child components.
	createStringComponents();
    setSize (800, 600);

    // Some platforms require permissions to open input channels so request that here
    if (RuntimePermissions::isRequired (RuntimePermissions::recordAudio)
        && ! RuntimePermissions::isGranted (RuntimePermissions::recordAudio))
    {
        RuntimePermissions::request (RuntimePermissions::recordAudio,
                                     [&] (bool granted) { if (granted)  setAudioChannels (2, 2); });
    }
    else
    {
        // Specify the number of input and output channels that we want to open
        setAudioChannels (2, 2);
    }
}

MainComponent::~MainComponent()
{
    // This shuts down the audio device and clears the audio source.
    shutdownAudio();
}

//==============================================================================
void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    // This function will be called when the audio device is started, or when
    // its settings (i.e. sample rate, block size, etc) are changed.

    // You can use this function to initialise any resources you might need,
    // but be careful - it will be called on the audio thread, not the GUI thread.

    // For more details, see the help for AudioProcessor::prepareToPlay()

	//currentSampleRate = sampleRate;
	//strynth = new StringSynthesiser(sampleRate, 200.0f);
	generateStringSynths(sampleRate);

}

void MainComponent::getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill)
{
    // Your audio-processing code goes here!

    // For more details, see the help for AudioProcessor::getNextAudioBlock()

	bufferToFill.clearActiveBufferRegion();

	for (auto channel = 0; channel < bufferToFill.buffer->getNumChannels(); ++channel)
	{
		auto* channelData = bufferToFill.buffer->getWritePointer(channel, bufferToFill.startSample);

		if (channel == 0)
		{

				//strynth->generateAndAddData(channelData, bufferToFill.numSamples);
			for (auto synth : stringSynths)
				synth->generateAndAddData(channelData, bufferToFill.numSamples);
		}
		else
		{
			memcpy(channelData,
				bufferToFill.buffer->getReadPointer(0),
				((size_t)bufferToFill.numSamples) * sizeof(float));
		}
	}
}

void MainComponent::releaseResources()
{
    // This will be called when the audio device stops, or when it is being
    // restarted due to a setting change.

    // For more details, see the help for AudioProcessor::releaseResources()
	stringSynths.clear();
}

//==============================================================================
void MainComponent::paint (Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));

    // You can add your drawing code here!
}

void MainComponent::resized()
{
    // This is called when the MainContentComponent is resized.
    // If you add any child components, this is where you should
    // update their positions.

	auto xPos = 20;
	auto yPos = 20;
	auto yDistance = 50;

	for (auto stringLine : stringLines)
	{
		stringLine->setTopLeftPosition(xPos, yPos);
		yPos += yDistance;
		addAndMakeVisible(stringLine);
	}
}


