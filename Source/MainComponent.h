/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
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
	StringSynthesiser(double sampleRate, double frequencyInHz)
	{
		doPluckForNextBuffer.set(false);
		prepareSynthesiserState(sampleRate, frequencyInHz);
	}

	//==============================================================================
	/** Excite the simulated string by plucking it at a given position.

		@param pluckPosition The position of the plucking, relative to the length
							 of the string. Must be between 0 and 1.
	*/
	void stringPlucked(float pluckPosition)
	{
		jassert(pluckPosition >= 0.0 && pluckPosition <= 1.0);

		// we choose a very simple approach to communicate with the audio thread:
		// simply tell the synth to perform the plucking excitation at the beginning
		// of the next buffer (= when generateAndAddData is called the next time).

		if (doPluckForNextBuffer.compareAndSetBool(1, 0))
		{
			// plucking in the middle gives the largest amplitude;
			// plucking at the very ends will do nothing.
			//amplitude = std::sin(MathConstants<float>::pi * pluckPosition);
			amplitude = 0.5f;
			//delayLineLength = (size_t)roundToInt(sampleRateVar / freq * (2 + pluckPosition));
			prepareSynthesiserState(sampleRateVar, freq * (1.0f + pluckPosition));
			//prepareSynthesiserState(sampleRateVar, freq + pluckPosition);
			exciteInternalBuffer();
		}
	}

	//==============================================================================
	/** Generate next chunk of mono audio output and add it into a buffer.

		@param outBuffer  Buffer to fill (one channel only). New sound will be
						  added to existing content of the buffer (instead of
						  replacing it).
		@param numSamples Number of samples to generate (make sure that outBuffer
						  has enough space).
	*/
	void generateAndAddData(float* outBuffer, int numSamples)
	{
		if (doPluckForNextBuffer.compareAndSetBool(0, 1))
			exciteInternalBuffer();

		// cycle through the delay line and apply a simple averaging filter
		for (auto i = 0; i < numSamples; ++i)
		{
			auto nextPos = (pos + 1) % delayLine.size();

			delayLine[nextPos] = (float)(decay * 0.5 * (delayLine[nextPos] + delayLine[pos]));
			outBuffer[i] += delayLine[pos];

			pos = nextPos;
		}
	}

private:
	//==============================================================================
	void prepareSynthesiserState(double sampleRate, double frequencyInHz)
	{
		delayLineLength = (size_t)roundToInt(sampleRate / frequencyInHz);
		sampleRateVar = sampleRate;
		//freq = frequencyInHz;

		// we need a minimum delay line length to get a reasonable synthesis.
		// if you hit this assert, increase sample rate or decrease frequency!
		jassert(delayLineLength > 50);

		delayLine.resize(delayLineLength);
		std::fill(delayLine.begin(), delayLine.end(), 0.0f);

		excitationSample.resize(delayLineLength);

		// as the excitation sample we use random noise between -1 and 1
		// (as a simple approximation to a plucking excitation)

		std::generate(excitationSample.begin(),
			excitationSample.end(),
			[] { return (Random::getSystemRandom().nextFloat() * 2.0f) - 1.0f; });
	}

	void exciteInternalBuffer()
	{
		// fill the buffer with the precomputed excitation sound (scaled with amplitude)

		jassert(delayLine.size() >= excitationSample.size());

		std::transform(excitationSample.begin(),
			excitationSample.end(),
			delayLine.begin(),
			[this](double sample) { return static_cast<float> (amplitude * sample); });
	}

	//==============================================================================
	const double decay = 0.998;
	double amplitude = 0.0;
	int delayLineLength = 0;
	double sampleRateVar = 48000.0;
	double freq = 20.0f;

	Atomic<int> doPluckForNextBuffer;

	std::vector<float> excitationSample, delayLine;
	size_t pos = 0;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StringSynthesiser)
};


//==============================================================================
/*
	This component represents a horizontal vibrating musical string of fixed height
	and variable length. The string can be excited by calling stringPlucked().
*/
class StringComponent : public Component,
	private Timer
{
public:
	StringComponent(int lengthInPixels, Colour stringColour)
		: length(lengthInPixels), colour(stringColour)
	{
		// ignore mouse-clicks so that our parent can get them instead.
		setInterceptsMouseClicks(false, false);
		setSize(length, height);
		startTimerHz(60);
	}

	//==============================================================================
	void stringPlucked(float pluckPositionRelative)
	{
		amplitude = maxAmplitude * std::sin(pluckPositionRelative * MathConstants<float>::pi);
		phase = MathConstants<float>::pi;
	}

	void paint(Graphics& g) override
	{
		g.setColour(colour);
		g.strokePath(generateStringPath(), PathStrokeType(2.0f));
	}


	Path generateStringPath() const
	{
		auto y = height / 2.0f;

		Path stringPath;
		stringPath.startNewSubPath(0, y);
		stringPath.quadraticTo(length / 2.0f, y + (std::sin(phase) * amplitude), (float)length, y);
		return stringPath;
	}

	//==============================================================================
	void timerCallback() override
	{
		updateAmplitude();
		updatePhase();
		repaint();
	}

	void updateAmplitude()
	{
		// this determines the decay of the visible string vibration.
		amplitude *= 0.99f;
	}

	void updatePhase()
	{
		// this determines the visible vibration frequency.
		// just an arbitrary number chosen to look OK:
		auto phaseStep = 400.0f / length;

		phase += phaseStep;

		if (phase >= MathConstants<float>::twoPi)
			phase -= MathConstants<float>::twoPi;
	}

private:
	//==============================================================================
	int length;
	Colour colour;

	int height = 20;
	float amplitude = 0.0f;
	const float maxAmplitude = 12.0f;
	float phase = 0.0f;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StringComponent)
};

//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainComponent   : public AudioAppComponent
{
public:
    //==============================================================================
    MainComponent();
    ~MainComponent();

    //==============================================================================
    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;

    //==============================================================================
    void paint (Graphics& g) override;
    void resized() override;

private:
    //==============================================================================
    // Your private member variables go here...
	double currentSampleRate = 0.0;

	void mouseDown(const MouseEvent& e) override
	{
		mouseDrag(e);
	}

	void mouseDrag(const MouseEvent& e) override
	{
		for (auto i = 0; i < stringLines.size(); ++i)
		{
			auto* stringLine = stringLines.getUnchecked(i);

			if (stringLine->getBounds().contains(e.getPosition()))
			{
				auto position = (e.position.x - stringLine->getX()) / stringLine->getWidth();

				stringLine->stringPlucked(position);
				stringSynths.getUnchecked(i)->stringPlucked(position);
			}
		}
	}

	//==============================================================================
	struct StringParameters
	{
		StringParameters(int midiNote)
			: frequencyInHz(MidiMessage::getMidiNoteInHertz(midiNote)),
			//lengthInPixels((int)(760 / (frequencyInHz / MidiMessage::getMidiNoteInHertz(42))))
			lengthInPixels((int)(760))
		{}

		double frequencyInHz;
		int lengthInPixels;
	};

	static Array<StringParameters> getDefaultStringParameters()
	{
		//return Array<StringParameters>(42, 44, 46, 49, 51, 54, 56, 58, 61, 63, 66, 68, 70);
		return Array<StringParameters>(21);
	}

	void createStringComponents()
	{
		for (auto stringParams : getDefaultStringParameters())
		{
			stringLines.add(new StringComponent(stringParams.lengthInPixels,
				Colour::fromHSV(Random().nextFloat(), 0.6f, 0.9f, 1.0f)));
		}
	}

	void generateStringSynths(double sampleRate)
	{
		stringSynths.clear();

		for (auto stringParams : getDefaultStringParameters())
		{
			stringSynths.add(new StringSynthesiser(sampleRate, stringParams.frequencyInHz));
		}
	}

	//==============================================================================
	OwnedArray<StringComponent> stringLines;
	OwnedArray<StringSynthesiser> stringSynths;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
