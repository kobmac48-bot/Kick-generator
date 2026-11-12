#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <array>
#include <cmath>
#include <memory>

namespace
{
struct GenreProfile
{
    float pitchStartScale;
    float pitchEndScale;
    float decayScale;
    float driveScale;
    float punchScale;
    float brightness;
};

constexpr std::array<GenreProfile, 7> genreProfiles {{
    { 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 0.85f }, // Gabber
    { 1.28f, 0.92f, 0.72f, 1.35f, 1.18f, 1.15f }, // Uptempo
    { 0.80f, 0.75f, 0.90f, 1.55f, 0.85f, 1.25f }, // Zaag
    { 1.12f, 1.08f, 1.20f, 1.22f, 1.12f, 1.00f }, // Rawstyle
    { 0.92f, 0.82f, 0.82f, 1.72f, 1.05f, 1.35f }, // Deutsche Krach
    { 1.18f, 0.98f, 1.05f, 1.12f, 1.25f, 0.95f }, // Frenchcore
    { 1.35f, 0.88f, 0.58f, 1.88f, 1.15f, 1.42f }  // Terror
}};

const juce::StringArray genreNames {
    "Gabber", "Uptempo", "Zaag", "Rawstyle",
    "Deutsche Krach", "Frenchcore", "Terror"
};
}

void KickForgeAudioProcessor::KickVoice::trigger(float noteVelocity,
                                                  float initialPitch,
                                                  std::uint32_t seed)
{
    active = true;
    velocity = noteVelocity;
    phase = 0.0f;
    elapsed = 0.0f;
    pitchStart = initialPitch;
    noiseState = seed == 0 ? 1u : seed;
}

float KickForgeAudioProcessor::KickVoice::render(float sampleRate, float pitchEnd,
                                                 float decay, float drive, float punch,
                                                 float body, float genreBrightness)
{
    if (! active)
        return 0.0f;

    const auto normalizedTime = juce::jlimit(0.0f, 1.0f, elapsed / decay);
    const auto pitch = pitchStart * std::pow(pitchEnd / pitchStart, normalizedTime);
    phase += juce::MathConstants<float>::twoPi * pitch / sampleRate;
    if (phase >= juce::MathConstants<float>::twoPi)
        phase = std::fmod(phase, juce::MathConstants<float>::twoPi);

    const auto envelope = std::exp(-5.0f * elapsed / decay);
    const auto fundamental = std::sin(phase);
    const auto secondHarmonic = std::sin(phase * 2.0f) * 0.24f * body;
    const auto thirdHarmonic = std::sin(phase * 3.0f) * 0.10f * body * genreBrightness;

    noiseState ^= noiseState << 13;
    noiseState ^= noiseState >> 17;
    noiseState ^= noiseState << 5;
    const auto noise = static_cast<float>(noiseState & 0xffffu) / 32767.5f - 1.0f;
    const auto transientEnvelope = std::exp(-95.0f * elapsed / decay);
    const auto transient = noise * transientEnvelope * punch * genreBrightness * 0.17f;

    const auto raw = (fundamental + secondHarmonic + thirdHarmonic) * envelope + transient;
    const auto normalizedDrive = juce::jmax(1.0f, drive);
    const auto output = std::tanh(raw * normalizedDrive) / std::tanh(normalizedDrive);

    elapsed += 1.0f / sampleRate;
    if (elapsed >= decay * 1.5f)
        active = false;

    return output * velocity * 0.82f;
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
        juce::ParameterID { "genre", 1 }, "Genre", genreNames, 0));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "pitchStart", 1 }, "Pitch Start",
        juce::NormalisableRange<float>(70.0f, 300.0f, 1.0f), 165.0f));
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
    return layout;
}

void KickForgeAudioProcessor::prepareToPlay(double sampleRate, int)
{
    currentSampleRate = sampleRate;
}

void KickForgeAudioProcessor::releaseResources()
{
    voice.active = false;
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
    const auto genreIndex = juce::jlimit(0, static_cast<int>(genreProfiles.size()) - 1,
        static_cast<int>(std::lround(selectedGenre)));
    const auto profile = genreProfiles[static_cast<std::size_t>(genreIndex)];
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
    const auto channelCount = audio.getNumChannels();

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
                                         pitchEnd, decay, drive, punch,
                                         body, profile.brightness);
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
