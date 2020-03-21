
#include "MainComponent.h"

StringSynthesiser::StringSynthesiser( double sampleRate, double frequencyInHz )
{
    doPluckForNextBuffer.set( false );
    prepareSynthesiserState( sampleRate, frequencyInHz );
}


void StringSynthesiser::stringPlucked( float pluckPosition )
{
    jassert( pluckPosition >= 0.0 && pluckPosition <= 1.0 );

    // we choose a very simple approach to communicate with the audio thread:
    // simply tell the synth to perform the plucking excitation at the beginning
    // of the next buffer (= when generateAndAddData is called the next time).

    if( doPluckForNextBuffer.compareAndSetBool( 1, 0 ) )
    {
        // plucking in the middle gives the largest amplitude;
        // plucking at the very ends will do nothing.
        //amplitude = std::sin(MathConstants<float>::pi * pluckPosition);
        amplitude = 0.5f;
        //delayLineLength = (size_t)roundToInt(sampleRateVar / freq * (2 + pluckPosition));
        prepareSynthesiserState( sampleRateVar, freq * ( 1.0f + pluckPosition ) );
        //prepareSynthesiserState(sampleRateVar, freq + pluckPosition);
        exciteInternalBuffer();
    }
}

void StringSynthesiser::generateAndAddData( float* outBuffer, int numSamples )
{
    if( doPluckForNextBuffer.compareAndSetBool( 0, 1 ) )
        exciteInternalBuffer();

    // cycle through the delay line and apply a simple averaging filter
    for( auto i = 0; i < numSamples; ++i )
    {
        auto nextPos = ( pos + 1 ) % delayLine.size();

        delayLine[ nextPos ] = ( float ) ( decay * 0.5 * ( delayLine[ nextPos ] + delayLine[ pos ] ) );
        outBuffer[ i ] += delayLine[ pos ];

        pos = nextPos;
    }
}

void StringSynthesiser::prepareSynthesiserState( double sampleRate, double frequencyInHz )
{
    delayLineLength = ( size_t ) roundToInt( sampleRate / frequencyInHz );
    sampleRateVar = sampleRate;
    //freq = frequencyInHz;

    // we need a minimum delay line length to get a reasonable synthesis.
    // if you hit this assert, increase sample rate or decrease frequency!
    jassert( delayLineLength > 50 );

    delayLine.resize( delayLineLength );
    std::fill( delayLine.begin(), delayLine.end(), 0.0f );

    excitationSample.resize( delayLineLength );

    // as the excitation sample we use random noise between -1 and 1
    // (as a simple approximation to a plucking excitation)

    std::generate( excitationSample.begin(),
        excitationSample.end(),
        [] { return ( Random::getSystemRandom().nextFloat() * 2.0f ) - 1.0f; } );
}

void StringSynthesiser::exciteInternalBuffer()
{
    // fill the buffer with the precomputed excitation sound (scaled with amplitude)

    jassert( delayLine.size() >= excitationSample.size() );

    std::transform( excitationSample.begin(),
        excitationSample.end(),
        delayLine.begin(),
        [ this ]( double sample ) { return static_cast< float > ( amplitude * sample ); } );
}

//==============================================================================
StringComponent::StringComponent( int lengthInPixels, Colour stringColour )
    : length( lengthInPixels ), colour( stringColour )
{
    // ignore mouse-clicks so that our parent can get them instead.
    setInterceptsMouseClicks( false, false );
    setSize( length, height );
    startTimerHz( 60 );
}

//==============================================================================
void StringComponent::stringPlucked( float pluckPositionRelative )
{
    amplitude = maxAmplitude * std::sin( pluckPositionRelative * MathConstants<float>::pi );
    phase = MathConstants<float>::pi;
}

void StringComponent::paint( Graphics& g )
{
    g.setColour( colour );
    g.strokePath( generateStringPath(), PathStrokeType( 2.0f ) );
}


Path StringComponent::generateStringPath() const
{
    auto y = height / 2.0f;

    Path stringPath;
    stringPath.startNewSubPath( 0, y );
    stringPath.quadraticTo( length / 2.0f, y + ( std::sin( phase ) * amplitude ), ( float ) length, y );
    return stringPath;
}

//==============================================================================
void StringComponent::timerCallback()
{
    updateAmplitude();
    updatePhase();
    repaint();
}

void StringComponent::updateAmplitude()
{
    // this determines the decay of the visible string vibration.
    amplitude *= 0.99f;
}

void StringComponent::updatePhase()
{
    // this determines the visible vibration frequency.
    // just an arbitrary number chosen to look OK:
    auto phaseStep = 400.0f / length;

    phase += phaseStep;

    if( phase >= MathConstants<float>::twoPi )
        phase -= MathConstants<float>::twoPi;
}

//==============================================================================

MainComponent::MainComponent()
{
    // Make sure you set the size of the component after
    // you add any child components.
    setSize( 800, 600 );

    // Some platforms require permissions to open input channels so request that here
    if( RuntimePermissions::isRequired( RuntimePermissions::recordAudio )
        && !RuntimePermissions::isGranted( RuntimePermissions::recordAudio ) )
    {
        RuntimePermissions::request( RuntimePermissions::recordAudio,
            [ & ]( bool granted ) { if( granted )  setAudioChannels( 2, 2 ); } );
    }
    else
    {
        // Specify the number of input and output channels that we want to open
        setAudioChannels( 2, 2 );
    }
}

MainComponent::~MainComponent()
{
    // This shuts down the audio device and clears the audio source.
    shutdownAudio();
}

//==============================================================================
void MainComponent::prepareToPlay( int samplesPerBlockExpected, double sampleRate )
{
    // This function will be called when the audio device is started, or when
    // its settings (i.e. sample rate, block size, etc) are changed.

    // You can use this function to initialise any resources you might need,
    // but be careful - it will be called on the audio thread, not the GUI thread.

    // For more details, see the help for AudioProcessor::prepareToPlay()
}

void MainComponent::getNextAudioBlock( const AudioSourceChannelInfo& bufferToFill )
{
    // Your audio-processing code goes here!

    // For more details, see the help for AudioProcessor::getNextAudioBlock()

    // Right now we are not producing any data, in which case we need to clear the buffer
    // (to prevent the output of random noise)
    bufferToFill.clearActiveBufferRegion();
}

void MainComponent::releaseResources()
{
    // This will be called when the audio device stops, or when it is being
    // restarted due to a setting change.

    // For more details, see the help for AudioProcessor::releaseResources()
}

//==============================================================================
void MainComponent::paint( Graphics& g )
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll( getLookAndFeel().findColour( ResizableWindow::backgroundColourId ) );

    // You can add your drawing code here!
}

void MainComponent::resized()
{
    // This is called when the MainContentComponent is resized.
    // If you add any child components, this is where you should
    // update their positions.
}

void MainComponent::mouseDown( const MouseEvent& e )
{
    mouseDrag( e );
}

void MainComponent::mouseDrag( const MouseEvent& e )
{
    for( auto i = 0; i < stringLines.size(); ++i )
    {
        auto* stringLine = stringLines.getUnchecked( i );

        if( stringLine->getBounds().contains( e.getPosition() ) )
        {
            auto position = ( e.position.x - stringLine->getX() ) / stringLine->getWidth();

            stringLine->stringPlucked( position );
            stringSynths.getUnchecked( i )->stringPlucked( position );
        }
    }
}


MainComponent::StringParameters::StringParameters( int midiNote ) :
    frequencyInHz( MidiMessage::getMidiNoteInHertz( midiNote ) ),
    //lengthInPixels((int)(760 / (frequencyInHz / MidiMessage::getMidiNoteInHertz(42))))
    lengthInPixels( ( int ) ( 760 ) )
{}


Array<MainComponent::StringParameters> MainComponent::getDefaultStringParameters()
{
    //return Array<StringParameters>(42, 44, 46, 49, 51, 54, 56, 58, 61, 63, 66, 68, 70);
    return Array<StringParameters>( 21 );
}

void MainComponent::createStringComponents()
{
    for( auto stringParams : getDefaultStringParameters() )
    {
        stringLines.add( new StringComponent( stringParams.lengthInPixels,
            Colour::fromHSV( Random().nextFloat(), 0.6f, 0.9f, 1.0f ) ) );
    }
}

void MainComponent::generateStringSynths( double sampleRate )
{
    stringSynths.clear();

    for( auto stringParams : getDefaultStringParameters() )
    {
        stringSynths.add( new StringSynthesiser( sampleRate, stringParams.frequencyInHz ) );
    }
}