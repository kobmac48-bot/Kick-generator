#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <atomic>
#include <cstdint>

class KickForgeAudioProcessor final : public juce::AudioProcessor
{
public:
    KickForgeAudioProcessor();
    ~KickForgeAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;
    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destinationData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    void previewKick();

    juce::AudioProcessorValueTreeState parameters;

private:
    struct KickVoice
    {
        void trigger(float noteVelocity, float initialPitch, std::uint32_t noiseSeed);
        float render(float sampleRate, float pitchEnd, float decay, float drive,
                     float punch, float body, float genreBrightness);

        bool active = false;
        float velocity = 1.0f;
        float phase = 0.0f;
        float elapsed = 0.0f;
        float pitchStart = 165.0f;
        std::uint32_t noiseState = 1;
    };

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    double currentSampleRate = 44100.0;
    KickVoice voice;
    std::atomic<bool> previewRequested { false };
    std::uint32_t previewSeed = 1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(KickForgeAudioProcessor)
};
