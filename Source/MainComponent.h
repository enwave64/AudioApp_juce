/*
  ==============================================================================

    MainComponent.h

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"


//https://docs.juce.com/master/tutorial_wavetable_synth.html

//uses std::sin calcullations for 
class SineOscillator
{
	public:
		SineOscillator() {}

		//calculate the angle delta via 2pi * (frequency / samplerate)
		void setFrequency(float frequency, float sampleRate);

		//called by getNextAudioBlock() on every sample in the buffer to get sample value from oscillator.
		//Here we calculate sample by using std::sin() by passing in currentAngle and then updating currentAngle
		forcedinline float getNextSample() noexcept;


		//update the angle by incrementing with angle delta; wrap the value when exceeding 2PI
		forcedinline void updateAngle() noexcept;

	private:
		float currentAngle = 0.0f, angleDelta = 0.0f;
};



class WavetableOscillator
{
	public:
		WavetableOscillator(const AudioSampleBuffer& wavetableToUse)
			: wavetable(wavetableToUse),
			subTableSize (wavetable.getNumSamples() - 1)
		{
			jassert(wavetable.getNumChannels() == 1);
		}
		
		//calculate the angle delta via 2pi * (frequency / samplerate)
		void setFrequency(float frequency, float sampleRate);
		forcedinline float getNextSample() noexcept;

	private:
		const AudioSampleBuffer& wavetable;
		float currentIndex = 0.0f, tableDelta = 0.0f;
		const int subTableSize;
};


//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainComponent   : public AudioAppComponent, public Timer
{
	public:
		//==============================================================================
		MainComponent();
		~MainComponent();

		//==============================================================================
		void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
		void getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill) override;
		void releaseResources() override;
		void timerCallback() override;

		//==============================================================================
		void paint (Graphics& g) override;
		void resized() override;
		void createWavetable();
		void createWavetableHarmonics();

	private:
		//==============================================================================
		//SinOsc std::sin variables
		float currentAngle = 0.0f, angleDelta = 0.0f;
		float level = 0.0f;
		OwnedArray<SineOscillator> oscillators;
		OwnedArray<WavetableOscillator> tabOscillators;

		//wavetable variables
		AudioSampleBuffer sineTable;
		const unsigned int tableSize = 1 << 7; //resolution of 128
	
		//CPU monitoring
		Label cpuUsageLabel;
		Label cpuUsageText;
		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
