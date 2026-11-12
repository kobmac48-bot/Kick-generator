#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <array>
#include <cmath>
#include <memory>

namespace
{
}

KickForgeAudioProcessor::KickForgeAudioProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "KICKFORGE_STATE", createParameterLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout
KickForgeAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "genre", 1 }, "Genre",
        juce::StringArray(kickGenreNames.data(),
                          static_cast<int>(kickGenreNames.size())), 0));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "pitchStart", 1 }, "Pitch Start",
        juce::NormalisableRange<float>(70.0f, 900.0f, 1.0f), 320.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "pitchEnd", 1 }, "Pitch End",
        juce::NormalisableRange<float>(35.0f, 100.0f, 1.0f), 48.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "decay", 1 }, "Decay",
        juce::NormalisableRange<float>(0.12f, 1.2f, 0.01f, 0.5f), 0.42f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "drive", 1 }, "Drive",
        juce::NormalisableRange<float>(1.0f, 18.0f, 0.1f), 8.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "punch", 1 }, "Punch",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.65f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "body", 1 }, "Body",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.55f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "distortion", 1 }, "Distortion",
        juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f), 65.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "softClip", 1 }, "Soft Clip",
        juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f), 38.0f));
    return layout;
}

void KickForgeAudioProcessor::prepareToPlay(double sampleRate, int)
{
    currentSampleRate = sampleRate;
}

void KickForgeAudioProcessor::releaseResources()
{
    voice.stop();
}

bool KickForgeAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto output = layouts.getMainOutputChannelSet();
    return layouts.getMainInputChannelSet().isDisabled()
        && (output == juce::AudioChannelSet::mono() || output == juce::AudioChannelSet::stereo());
}

void KickForgeAudioProcessor::processBlock(juce::AudioBuffer<float>& audio,
                                           juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    audio.clear();

    juce::MidiBuffer::Iterator midiIterator(midi);
    juce::MidiMessage message;
    int eventSample = 0;
    auto hasEvent = midiIterator.getNextEvent(message, eventSample);

    const auto selectedGenre = parameters.getRawParameterValue("genre")->load();
    const auto genreIndex = juce::jlimit(0, static_cast<int>(kickGenreProfiles.size()) - 1,
        static_cast<int>(std::lround(selectedGenre)));
    const auto profile = kickGenreProfiles[static_cast<std::size_t>(genreIndex)];
    const auto pitchStart = parameters.getRawParameterValue("pitchStart")->load()
                          * profile.pitchStartScale;
    const auto pitchEnd = parameters.getRawParameterValue("pitchEnd")->load()
                        * profile.pitchEndScale;
    const auto decay = parameters.getRawParameterValue("decay")->load()
                     * profile.decayScale;
    const auto drive = parameters.getRawParameterValue("drive")->load()
                     * profile.driveScale;
    const auto punch = parameters.getRawParameterValue("punch")->load()
                     * profile.punchScale;
    const auto body = parameters.getRawParameterValue("body")->load();
    const auto distortion = parameters.getRawParameterValue("distortion")->load() * 0.01f;
    const auto softClip = parameters.getRawParameterValue("softClip")->load() * 0.01f;
    const auto channelCount = audio.getNumChannels();
    const KickSoundParameters soundParameters {
        pitchEnd,
        decay,
        drive,
        punch,
        body,
        distortion,
        softClip,
        profile.brightness,
        profile.distortionScale,
        profile.softClipScale,
        profile.sweepScale,
        profile.tailToneScale,
        profile.bounce,
        profile.clickScale,
        profile.clickFrequencyScale
    };

    for (int sample = 0; sample < audio.getNumSamples(); ++sample)
    {
        if (sample == 0 && previewRequested.exchange(false, std::memory_order_acquire))
        {
            previewSeed = previewSeed * 1664525u + 1013904223u;
            voice.trigger(1.0f, pitchStart, previewSeed);
        }

        while (hasEvent && eventSample <= sample)
        {
            if (message.isNoteOn())
            {
                const auto seed = static_cast<std::uint32_t>(
                    (message.getNoteNumber() + 1) * 7919 + (sample + 1) * 104729);
                voice.trigger(message.getFloatVelocity(), pitchStart, seed);
            }
            hasEvent = midiIterator.getNextEvent(message, eventSample);
        }

        const auto output = voice.render(static_cast<float>(currentSampleRate),
                                         soundParameters);
        for (int channel = 0; channel < channelCount; ++channel)
            audio.setSample(channel, sample, output);
    }
}

juce::AudioProcessorEditor* KickForgeAudioProcessor::createEditor()
{
    return new KickForgeAudioProcessorEditor(*this);
}

bool KickForgeAudioProcessor::hasEditor() const { return true; }
const juce::String KickForgeAudioProcessor::getName() const { return JucePlugin_Name; }
bool KickForgeAudioProcessor::acceptsMidi() const { return true; }
bool KickForgeAudioProcessor::producesMidi() const { return false; }
bool KickForgeAudioProcessor::isMidiEffect() const { return false; }
double KickForgeAudioProcessor::getTailLengthSeconds() const { return 2.0; }
int KickForgeAudioProcessor::getNumPrograms() { return 1; }
int KickForgeAudioProcessor::getCurrentProgram() { return 0; }
void KickForgeAudioProcessor::setCurrentProgram(int) {}
const juce::String KickForgeAudioProcessor::getProgramName(int) { return {}; }
void KickForgeAudioProcessor::changeProgramName(int, const juce::String&) {}

void KickForgeAudioProcessor::getStateInformation(juce::MemoryBlock& destinationData)
{
    const auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destinationData);
}

void KickForgeAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName(parameters.state.getType()))
        parameters.replaceState(juce::ValueTree::fromXml(*xml));
}

void KickForgeAudioProcessor::previewKick()
{
    previewRequested.store(true, std::memory_order_release);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new KickForgeAudioProcessor();
}
