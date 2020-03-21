
#pragma once

#include <JuceHeader.h>
//==============================================================================
/**
	A very basic generator of a simulated plucked string sound, implementing
	the Karplus-Strong algorithm.

	Not performance-optimised!
*/
class StringSynthesiser
{
public:
	//==============================================================================
	/** Constructor.

		@param sampleRate      The audio sample rate to use.
		@param frequencyInHz   The fundamental frequency of the simulated string in
							   Hertz.
	*/
	StringSynthesiser( double sampleRate, double frequencyInHz );

	//==============================================================================
	/** Excite the simulated string by plucking it at a given position.

		@param pluckPosition The position of the plucking, relative to the length
							 of the string. Must be between 0 and 1.
	*/
	void stringPlucked( float pluckPosition );

	//==============================================================================
	/** Generate next chunk of mono audio output and add it into a buffer.

		@param outBuffer  Buffer to fill (one channel only). New sound will be
						  added to existing content of the buffer (instead of
						  replacing it).
		@param numSamples Number of samples to generate (make sure that outBuffer
						  has enough space).
	*/
	void generateAndAddData( float* outBuffer, int numSamples );

private:
	//==============================================================================
	void prepareSynthesiserState( double sampleRate, double frequencyInHz );

	void exciteInternalBuffer();

	//==============================================================================
	const double decay = 0.998;
	double amplitude = 0.0;
	int delayLineLength = 0;
	double sampleRateVar = 48000.0;
	double freq = 20.0f;

	Atomic<int> doPluckForNextBuffer;

	std::vector<float> excitationSample, delayLine;
	size_t pos = 0;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR( StringSynthesiser )
};


//==============================================================================
/*
	This component represents a horizontal vibrating musical string of fixed height
	and variable length. The string can be excited by calling stringPlucked().
*/
class StringComponent : public Component, private Timer
{
public:
	StringComponent( int lengthInPixels, Colour stringColour );

	void stringPlucked( float pluckPositionRelative );

	void paint( Graphics& g ) override;

	Path generateStringPath() const;

	void timerCallback() override;

	void updateAmplitude();

	void updatePhase();

private:
	//==============================================================================
	int length;
	Colour colour;

	int height = 20;
	float amplitude = 0.0f;
	const float maxAmplitude = 12.0f;
	float phase = 0.0f;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR( StringComponent )
};

//==============================================================================
/*
	This component lives inside our window, and this is where you should put all
	your controls and content.
*/
class MainComponent : public AudioAppComponent
{
public:
	//==============================================================================
	MainComponent();
	~MainComponent();

	//==============================================================================
	void prepareToPlay( int samplesPerBlockExpected, double sampleRate ) override;
	void getNextAudioBlock( const AudioSourceChannelInfo& bufferToFill ) override;
	void releaseResources() override;

	//==============================================================================
	void paint( Graphics& g ) override;
	void resized() override;

private:
	//==============================================================================
	// Your private member variables go here...
	double currentSampleRate = 0.0;

	void mouseDown( const MouseEvent& e ) override;

	void mouseDrag( const MouseEvent& e ) override;

	//==============================================================================
	struct StringParameters
	{
		StringParameters( int midiNote );

		double frequencyInHz;
		int lengthInPixels;
	};

	static Array<StringParameters> getDefaultStringParameters();

	void createStringComponents();

	void generateStringSynths( double sampleRate );

	//==============================================================================
	OwnedArray<StringComponent> stringLines;
	OwnedArray<StringSynthesiser> stringSynths;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR( MainComponent )
};
