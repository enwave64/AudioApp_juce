/*
  ==============================================================================

    MainComponent.cpp

  ==============================================================================
*/

//https://docs.juce.com/master/tutorial_wavetable_synth.html

#include "MainComponent.h"

//whether we use sineosc or wavetable implementation
bool useWaveTable = 1;

// First we define a large number of oscillators to evaluate the CPU load of such a number.
auto numberOfOscillators = 1; 
//==============================================================================
MainComponent::MainComponent()
{
    // Make sure you set the size of the component after
    // you add any child components.
    setSize (800, 600);

	cpuUsageLabel.setText("CPU Usage", dontSendNotification);
	cpuUsageText.setJustificationType(Justification::right);
	addAndMakeVisible(cpuUsageLabel);
	addAndMakeVisible(cpuUsageText);

	addAndMakeVisible(freqSlider);
	freqSlider.setRange(25.0, 85.0);

	freqSlider.onValueChange = [this]
	{
		auto midiNote = freqSlider.getValue();

		//In order to calculate the frequency of that midi note, we use a simple mathematical formula to retrieve the scalar 
		//to multiply the frequency of A440 with.
		//Since we know that the midi note number of A440 is 69, by subtracting the midi note by 69 we get the 
		//semitone distance from A440 that we can then plug into the following formula: 440 * 2 ^ (d / 12)
		auto frequency = 440.0 * pow(2.0, (midiNote - 69.0) / 12.0);

		for (int i = 0; i < tabOscillators.size(); i++) 
		{
			auto* oscillator = tabOscillators.getUnchecked(i);
			oscillator->setFrequency(frequency, currentSampleRate);
		}
	};

	addAndMakeVisible(waveSelect);
	waveSelect.addItem("SINE", 1);
	waveSelect.addItem("TRI", 2);
	waveSelect.addItem("HARMONICS", 3);
	waveSelect.addItem("SAW", 4);
	waveSelect.addItem("SQUARE", 5);
	waveSelect.addItem("NOISE", 6);
	waveSelect.setSelectedId(1);

	waveSelect.onChange = [this]
	{
		switch(waveSelect.getSelectedId())
		{
			case(1):
				createSinWavetable();
				break;
			case(2):
				createTriWavetable();
				break;
			case(3):
				createWavetableHarmonics();				
				break;
			case(4):
				createSawWavetable();				
				break;
			case(5):
				createSquareWavetable();
				break;
			case(6):
				createNoiseWavetable();
				break;
			default:
				break;
		}
	};

	//create the wavetable
	createSinWavetable();
	//createWavetableHarmonics();
	//createSawWavetable();
	//createSquareWavetable();
	//createTriWavetable();

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
        setAudioChannels (0, 2);

		startTimer(50);
    }
}

MainComponent::~MainComponent()
{
    // This shuts down the audio device and clears the audio source.
    shutdownAudio();
}

void MainComponent::timerCallback()
{
	auto cpu = deviceManager.getCpuUsage() * 100;
	cpuUsageText.setText(String(cpu, 6) + " %", dontSendNotification);
}

//==============================================================================
void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    // This function will be called when the audio device is started, or when
    // its settings (i.e. sample rate, block size, etc) are changed.

    // You can use this function to initialise any resources you might need,
    // but be careful - it will be called on the audio thread, not the GUI thread.
	
	currentSampleRate = sampleRate;

	//we initialise the oscillators and set their frequencies to play based on the sample rate as follows	
	for (auto i = 0; i < numberOfOscillators; ++i)
	{
		//We also select a random midi note using the Random class by shifting the lowest possible note by 4 octaves (+ 48) 
		//and defining a range of 3 octaves (* 36.0) starting from that lowest note.
		// ergo, lowest note 48 (C3), highest note 84 (C6)
		auto midiNote = Random::getSystemRandom().nextDouble() * 36.0 + 48.0;
		//auto midiNote = 48 + i * 6;
		//midiNote = 60.0;
		midiNote = freqSlider.getValue();

		//In order to calculate the frequency of that midi note, we use a simple mathematical formula to retrieve the scalar 
		//to multiply the frequency of A440 with.
		//Since we know that the midi note number of A440 is 69, by subtracting the midi note by 69 we get the 
		//semitone distance from A440 that we can then plug into the following formula: 440 * 2 ^ (d / 12)
		auto frequency = 440.0 * pow(2.0, (midiNote - 69.0) / 12.0);

		if (useWaveTable) 
		{
			//WavetableOsc implementation
			auto* oscillator = new WavetableOscillator(oscTable);

			//Then, we set the frequency of the oscillator by passing the frequency and sample rate as 
			//arguments to the setFrequency() function. We also add the oscillator to the array of oscillators.
			oscillator->setFrequency((float)frequency, sampleRate);
			tabOscillators.add(oscillator);
		}
		else 
		{
			//SineOsc implementation
			//For each oscillator, we instantiate a new SineOscillator object that generates a single sine wave voice.
			auto* oscillator = new SineOscillator();

			//Then, we set the frequency of the oscillator by passing the frequency and sample rate as 
			//arguments to the setFrequency() function. We also add the oscillator to the array of oscillators.
			oscillator->setFrequency((float)frequency, sampleRate);
			oscillators.add(oscillator);
		}
	}
	//Finally, we define the output level by dividing a quiet gain level by the number of oscillators to prevent clipping 
	//of the signal by summing such a large number of oscillator samples.
	level = 0.25f / numberOfOscillators;		
}

void MainComponent::getNextAudioBlock(const AudioSourceChannelInfo& bufferToFill)
{
	// Your audio-processing code goes here!

	//First, we retrieve the left and right channel pointers to write to the output buffers.
	auto* leftBuffer = bufferToFill.buffer->getWritePointer(0, bufferToFill.startSample);
	auto* rightBuffer = bufferToFill.buffer->getWritePointer(1, bufferToFill.startSample);

	bufferToFill.clearActiveBufferRegion();

	for (auto oscillatorIndex = 0; oscillatorIndex < numberOfOscillators; ++oscillatorIndex)
	{
		//For each oscillator in the array we retrieve a pointer to the oscillator instance.
		if (useWaveTable) 
		{
			auto* oscillator = tabOscillators.getUnchecked(oscillatorIndex);
			for (auto sample = 0; sample < bufferToFill.numSamples; ++sample)
			{
				//Then for each sample in the audio sample buffer we get the sine wave sample and trim the gain with the level variable.
				auto levelSample = oscillator->getNextSample() * level;

				//Finally we can add that sample value to the left and right channel samples and sum the signal with the other oscillators.
				leftBuffer[sample] += levelSample;
				rightBuffer[sample] += levelSample;
			}
		}
			
		else 
		{
			auto* oscillator = oscillators.getUnchecked(oscillatorIndex);

			for (auto sample = 0; sample < bufferToFill.numSamples; ++sample)
			{
				//Then for each sample in the audio sample buffer we get the sine wave sample and trim the gain with the level variable.
				auto levelSample = oscillator->getNextSample() * level;

				//Finally we can add that sample value to the left and right channel samples and sum the signal with the other oscillators.
				leftBuffer[sample] += levelSample;
				rightBuffer[sample] += levelSample;
			}
		}
	}
}

void MainComponent::releaseResources()
{
    // This will be called when the audio device stops, or when it is being
    // restarted due to a setting change.

    // For more details, see the help for AudioProcessor::releaseResources()
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
	cpuUsageLabel.setBounds(10, 10, getWidth() - 20, 20);
	cpuUsageText.setBounds(10, 10, getWidth() - 20, 20);
	waveSelect.setBounds(10, 30, getWidth() - 40, 20);
	freqSlider.setBounds(10, 70, getWidth() - 20, 20);
}



/*
  ==============================================================================

	WavetableOsc

  ==============================================================================
*/
void MainComponent::createNoiseWavetable()
{
	oscTable.setSize(1, tableSize + 1);
	auto* samples = oscTable.getWritePointer(0);

	auto increment = Random::getSystemRandom().nextDouble();

	for (auto i = 0; i < tableSize; ++i)
	{
		auto sample = Random::getSystemRandom().nextDouble();
		samples[i] = (float)sample;


	}

	samples[tableSize] = samples[0];
}



void MainComponent::createTriWavetable() 
{
	oscTable.setSize(1, tableSize + 1);
	auto* samples = oscTable.getWritePointer(0);

	auto delta = 2.0f / (double)(tableSize - 1);
	auto angleDelta = MathConstants<double>::twoPi / (double)(tableSize - 1);
	double t = angleDelta / MathConstants<double>::twoPi;

	auto increment = -1.0;

	for (auto i = 0; i < tableSize; ++i)
	{
		auto sample = increment;
		//sample -= poly_blep(t, angleDelta);
		samples[i] = (float)sample;

		if (i < tableSize / 2)
			increment += delta * 2;
		else
			increment -= delta * 2;
	}

	samples[tableSize] = samples[0];
}

void MainComponent::createSawWavetable() 
{
	//In this function, initialise the AudioSampleBuffer by calling the setSize() method by specifying that we only need one channel 
	//and the number of samples equal to the table size, in our case a resolution of 128. 
	//Then retrieve the write pointer for that single channel buffer.
	oscTable.setSize(1, tableSize + 1);
	auto* samples = oscTable.getWritePointer(0);

	auto delta = 2.0f / (double)(tableSize - 1);
	auto angleDelta = MathConstants<double>::twoPi / (double)(tableSize - 1);
	double t = angleDelta / MathConstants<double>::twoPi;

	auto increment = -1.0;

	for (auto i = 0; i < tableSize; ++i) 
	{
		auto sample = increment;
		sample -= poly_blep(t, angleDelta);
		samples[i] = (float)sample;
		increment += delta;
	}

	samples[tableSize] = samples[0];
}

void MainComponent::createSquareWavetable() 
{
	//In this function, initialise the AudioSampleBuffer by calling the setSize() method by specifying that we only need one channel 
	//and the number of samples equal to the table size, in our case a resolution of 128. 
	//Then retrieve the write pointer for that single channel buffer.
	oscTable.setSize(1, tableSize + 1);
	auto* samples = oscTable.getWritePointer(0);

	auto angleDelta = MathConstants<double>::twoPi / (double)(tableSize - 1);
	double t = angleDelta / MathConstants<double>::twoPi;

	auto sample = 0.0;
	for (auto i = 0; i < tableSize; ++i)
	{
		if (i < tableSize / 2)
			sample = -1.0;
		else
			sample = 1.0;

		sample += poly_blep(t, angleDelta); // Layer output of Poly BLEP on top (flip)
		sample -= poly_blep(fmod(t + 0.5, 1.0), angleDelta); // Layer output of Poly BLEP on top (flop)

		samples[i] = (float)sample;

	}
	samples[tableSize] = samples[0];
}



void MainComponent::createSinWavetable()
{

	//In this function, initialise the AudioSampleBuffer by calling the setSize() method by specifying that we only need one channel 
	//and the number of samples equal to the table size, in our case a resolution of 128. 
	//Then retrieve the write pointer for that single channel buffer.
	oscTable.setSize(1, tableSize + 1);
	auto* samples = oscTable.getWritePointer(0);

	//Next, calculate the angle delta similarly to the SineOscillator, but this time using the table size and thus dividing the full 2pi cycle by 127.
	auto angleDelta = MathConstants<double>::twoPi / (double)(tableSize - 1);
	auto currentAngle = 0.0;
	for (auto i = 0; i < tableSize; ++i)
	{
		//Now for each point in our wavetable, retrieve the sine wave value using the std::sin() function, 
		//assign the value to the buffer sample and increment the current angle by the delta value.
		auto sample = std::sin(currentAngle);
		samples[i] = (float)sample;
		currentAngle += angleDelta;
	}

	samples[tableSize] = samples[0]; //the last sample is the same as the first
}

void MainComponent::createWavetableHarmonics()
{
	oscTable.setSize(1, tableSize + 1);
	oscTable.clear();
	auto* samples = oscTable.getWritePointer(0);
	int harmonics[] = { 1, 3, 5, 6, 7, 9, 13, 15 };
	//int harmonics[] = { 1, 2, 4, 6, 8, 10, 12, 14 };
	float harmonicWeights[] = { 0.5f, 0.1f, 0.05f, 0.125f, 0.09f, 0.005, 0.002f, 0.001f }; // [1]
	jassert(numElementsInArray(harmonics) == numElementsInArray(harmonicWeights));
	for (auto harmonic = 0; harmonic < numElementsInArray(harmonics); ++harmonic)
	{
		auto angleDelta = MathConstants<double>::twoPi / (double)(tableSize - 1) * harmonics[harmonic]; // [2]
		auto currentAngle = 0.0;
		for (auto i = 0; i < tableSize; ++i)
		{
			auto sample = std::sin(currentAngle);
			samples[i] += (float)sample * harmonicWeights[harmonic];                      // [3]
			currentAngle += angleDelta;
		}
	}
	samples[tableSize] = samples[0];
}

void WavetableOscillator::setFrequency(float frequency, float sampleRate)
{
	auto tableSizeOverSampleRate = subTableSize / sampleRate;
	tableDelta = frequency * tableSizeOverSampleRate;
}

forcedinline float WavetableOscillator::getNextSample() noexcept
{
	auto tableSize = wavetable.getNumSamples();

	//First, temporarily store the two indices of the wavetable that surround the sample value that we are trying to retrieve. 
	
	auto index0 = (unsigned int)currentIndex;  
	//***used before  when we were wrapping the higher index***
	//If the higher index goes beyond the size of the wavetable then we wrap the value to the start of the table.
	//auto index1 = index0 == (tableSize - 1) ? (unsigned int)0 : index0 + 1;
	auto index1 = index0 + 1;

	//Next, calculate the interpolation value as a fraction between the two indices 
	//by subtracting the actual current sample by the truncated lower index. 
	//This should give us a value between 0 .. 1 that defines the fraction.
	auto frac = currentIndex - (float)index0;  // [7]

	//Then get a pointer to the AudioSampleBuffer and read the values at the two indices and store these values temporarily.
	auto* table = wavetable.getReadPointer(0); // [8]
	auto value0 = table[index0];
	auto value1 = table[index1];

	//The interpolated sample value can then be retrieved by using the standard interpolation formula and the fraction value calculated previously.
	auto currentSample = value0 + frac * (value1 - value0); // [9]

	//Finally, increment the angle delta of the table and wrap the value around if the value exceeds the table size.
	if ((currentIndex += tableDelta) > subTableSize)           // [10]
		currentIndex -= subTableSize;

	return currentSample;
}


/*
  ==============================================================================

	SineOsc

  ==============================================================================
*/

//calculate the angle delta via 2pi * (frequency / samplerate)
void SineOscillator::setFrequency(float frequency, float sampleRate)
{
	auto cyclesPerSample = frequency / sampleRate;
	angleDelta = cyclesPerSample * MathConstants<float>::twoPi;
}

//called by getNextAudioBlock() on every sample in the buffer to get sample value from oscillator.
//Here we calculate sample by using std::sin() by passing in currentAngle and then updating currentAngle
forcedinline float SineOscillator::getNextSample() noexcept {
	auto currentSample = std::sin(currentAngle);
	updateAngle();
	return currentSample;
}

//update the angle by incrementing with angle delta; wrap the value when exceeding 2PI
forcedinline void SineOscillator::updateAngle() noexcept {
	currentAngle += angleDelta;

	if (currentAngle >= MathConstants<float>::twoPi)
		currentAngle -= MathConstants<float>::twoPi;
}


// This function calculates the PolyBLEPs
//Bleps are a mechanism for reducting aliasing on complex waveforms like saw, square, tri, etc
//http://metafunction.co.uk/all-about-digital-oscillators-part-2-blits-bleps/
double MainComponent::poly_blep(double t, double mPhaseIncrement)
{
	double dt = mPhaseIncrement / MathConstants<double>::twoPi;

	// t-t^2/2 +1/2
	// 0 < t <= 1
	// discontinuities between 0 & 1
	if (t < dt)
	{
		t /= dt;
		return t + t - t * t - 1.0;
	}

	// t^2/2 +t +1/2
	// -1 <= t <= 0
	// discontinuities between -1 & 0
	else if (t > 1.0 - dt)
	{
		t = (t - 1.0) / dt;
		return t * t + t + t + 1.0;
	}

	// no discontinuities 
	// 0 otherwise
	else return 0.0;
}
