/*
  ==============================================================================

    PluginProcessor.cpp
    Created: 10 Oct 2019 10:03:03am
    Authors: Lily H, Loïc J

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
FTMSynthAudioProcessor::FTMSynthAudioProcessor()
    :
#ifndef JucePlugin_PreferredChannelConfigurations
    AudioProcessor(BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput("Input",  AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput("Output", AudioChannelSet::stereo(), true)
                     #endif
                       ),
#endif
    tree(*this, nullptr, "Parameters",
    {
        std::make_unique<AudioParameterChoice>("algorithm", "Algorithm", StringArray{"Ivan", "Rabenstein"}, 0),
        std::make_unique<AudioParameterFloat>("volume", "Volume", NormalisableRange<float>(0.0f, 1.0f), 0.75f),
        std::make_unique<AudioParameterFloat>("pitch", "Pitch", NormalisableRange<float>(-24.0f, 24.0f, 0.001f), 0.0f,  // in semitones
                                              AudioParameterFloatAttributes().withStringFromValueFunction([] (auto value, auto) { return String(value, 3); })),
        std::make_unique<AudioParameterBool>("kbTrack", "Keyboard Tracking", true),
        std::make_unique<AudioParameterFloat>("sustain", "Sustain", NormalisableRange<float>(0.01f, 0.8f, 0.0f, log(0.5f)/log(0.19f/0.79f)), 0.07f),
        std::make_unique<AudioParameterBool>("susGate", "Sustain Gate", false),
        std::make_unique<AudioParameterFloat>("release", "Release", NormalisableRange<float>(0.01f, 0.8f, 0.0f, log(0.5f)/log(0.19f/0.79f)), 0.07f),
        std::make_unique<AudioParameterFloat>("damp", "Damp", NormalisableRange<float>(0.0f, 0.5f, 0.0f, log(0.5f)/log(0.1f/0.5f)), 0.0f),
        std::make_unique<AudioParameterBool>("dampGate", "Damp Gate", false),
        std::make_unique<AudioParameterFloat>("ring", "Ring", NormalisableRange<float>(0.0f, 0.5f, 0.0f, log(0.5f)/log(0.1f/0.5f)), 0.0f),
        std::make_unique<AudioParameterFloat>("dispersion", "Inharmonicity", NormalisableRange<float>(0.0f, 5.0f, 0.0f, log(0.5f)/log(1.0f/5.0f)), 0.06f),
        std::make_unique<AudioParameterFloat>("squareness", "Squareness", NormalisableRange<float>(0.01f, 1.0f), 0.5f),
        std::make_unique<AudioParameterFloat>("cubeness", "Cubeness", NormalisableRange<float>(0.01f, 1.0f), 0.5f),
        std::make_unique<AudioParameterFloat>("r1", "Impulse X", NormalisableRange<float>(0.005f, 0.995f), 0.5f),
        std::make_unique<AudioParameterFloat>("r2", "Impulse Y", NormalisableRange<float>(0.005f, 0.995f), 0.5f),
        std::make_unique<AudioParameterFloat>("r3", "Impulse Z", NormalisableRange<float>(0.005f, 0.995f), 0.5f),
        std::make_unique<AudioParameterInt>("m1", "Modes X", 1, MAX_M1, 5),
        std::make_unique<AudioParameterInt>("m2", "Modes Y", 1, MAX_M2, 5),
        std::make_unique<AudioParameterInt>("m3", "Modes Z", 1, MAX_M3, 5),
        std::make_unique<AudioParameterInt>("dimensions", "Dimensions", 1, 3, 2),
        std::make_unique<AudioParameterInt>("voices", "Polyphony voices", 1, 16, 4)
    })
{
    SynthVoice::computeSinLUT();

    // clear and add voices
    mySynth.clearVoices();
    int numVoices = int(tree.getRawParameterValue("voices")->load());
    for (int i = 0; i < numVoices; i++)
    {
        mySynth.addVoice(new SynthVoice());
    }

    // clear and add sounds
    mySynth.clearSounds();
    mySynth.addSound(new SynthSound());
}


FTMSynthAudioProcessor::~FTMSynthAudioProcessor()
{
}

//==============================================================================
const String FTMSynthAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool FTMSynthAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool FTMSynthAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool FTMSynthAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double FTMSynthAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int FTMSynthAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int FTMSynthAudioProcessor::getCurrentProgram()
{
    return 0;
}

void FTMSynthAudioProcessor::setCurrentProgram(int /*index*/)
{
}

const String FTMSynthAudioProcessor::getProgramName(int /*index*/)
{
    return {};
}

void FTMSynthAudioProcessor::changeProgramName(int /*index*/, const String& /*newName*/)
{
}

//==============================================================================
void FTMSynthAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    ignoreUnused(samplesPerBlock);
    lastSampleRate=sampleRate;
    mySynth.setCurrentPlaybackSampleRate(lastSampleRate);
}

void FTMSynthAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool FTMSynthAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    ignoreUnused(layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    if (layouts.getMainOutputChannelSet() != AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void FTMSynthAudioProcessor::processBlock(AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
    // Change the number of voices if needed
    int deltaVoices = int(tree.getRawParameterValue("voices")->load());
    deltaVoices -= mySynth.getNumVoices();

    if (deltaVoices > 0)
    {
        // Add new free voice
        for (int i = 0; i < deltaVoices; i++)
            mySynth.addVoice(new SynthVoice());
    }
    else if (deltaVoices < 0)
    {
        // Remove all found inactive voices starting from the end
        int removedVoices = 0;
        int voice = mySynth.getNumVoices()-1;
        while (voice > 0)
        {
            myVoice = dynamic_cast<SynthVoice*>(mySynth.getVoice(voice));
            if (myVoice != nullptr)
            {
                if (!myVoice->isVoiceActive())
                {
                    mySynth.removeVoice(voice);
                    removedVoices++;
                }
            }
            voice--;

            if (removedVoices >= (-deltaVoices)) break;
        }

        // If after a first pass, not enough voices were removed
        // then cut the least recent voices starting from the oldest
        if (removedVoices < (-deltaVoices))
        {
            SynthesiserVoice* voiceToCompare;
            int oldest;

            int leftToRemove = -deltaVoices;
            while (leftToRemove > 0)
            {
                oldest = -1;
                int voiceCount = mySynth.getNumVoices();

                for (int i = 0; i < voiceCount; i++)
                {
                    myVoice = dynamic_cast<SynthVoice*>(mySynth.getVoice(i));
                    bool isOldest = true; // until proven otherwise

                    for (int j = 0; j < voiceCount; j++)
                    {
                        if (i != j)
                        {
                            voiceToCompare = mySynth.getVoice(j);

                            if (myVoice != nullptr)
                            {
                                if (!myVoice->wasStartedBefore(*voiceToCompare))
                                    isOldest = false;
                            }
                        }
                    }
                    if (isOldest)
                    {
                        oldest = i;
                    }
                }
                if (oldest != -1)
                {
                    mySynth.removeVoice(oldest);
                    leftToRemove--;
                }
            }
        }
    }

    // Retrieve parameters from sliders and pass them to the model
    for (int i=0; i < mySynth.getNumVoices(); i++)
    {
        myVoice = dynamic_cast<SynthVoice*>(mySynth.getVoice(i));
        if (myVoice != nullptr)
        {
            // this is the actual step that gets values from the tree, which are linked to the sliders
            // IMPORTANT NOTE: the parameters need to be passed from tree.getRawParameterValue("name"),
            //                 and NOT tree.getParameterAsValue("name").getValue(), otherwise they'll be
            //                 applied AFTER the next note press, instead of before, which means the
            //                 parameters will be updated one hit too late, which is what we *don't* want.
            myVoice->getcusParam(tree.getRawParameterValue("algorithm"),
                                 tree.getRawParameterValue("volume"),
                                 tree.getRawParameterValue("pitch"),
                                 tree.getRawParameterValue("kbTrack"),
                                 tree.getRawParameterValue("sustain"),
                                 tree.getRawParameterValue("susGate"),
                                 tree.getRawParameterValue("release"),
                                 tree.getRawParameterValue("damp"),
                                 tree.getRawParameterValue("dampGate"),
                                 tree.getRawParameterValue("ring"),
                                 tree.getRawParameterValue("dispersion"),
                                 tree.getRawParameterValue("squareness"),
                                 tree.getRawParameterValue("cubeness"),
                                 tree.getRawParameterValue("r1"),
                                 tree.getRawParameterValue("r2"),
                                 tree.getRawParameterValue("r3"),
                                 tree.getRawParameterValue("m1"),
                                 tree.getRawParameterValue("m2"),
                                 tree.getRawParameterValue("m3"),
                                 tree.getRawParameterValue("dimensions"));
        }
    }

    buffer.clear();

    mySynth.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples()); // which is the callback function
}

//==============================================================================
bool FTMSynthAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

AudioProcessorEditor* FTMSynthAudioProcessor::createEditor()
{
    return new FTMSynthAudioProcessorEditor(*this);
}

//==============================================================================
void FTMSynthAudioProcessor::getStateInformation(MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    auto state = tree.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void FTMSynthAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName(tree.state.getType()))
            tree.replaceState(juce::ValueTree::fromXml(*xmlState));
}

//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new FTMSynthAudioProcessor();
}
