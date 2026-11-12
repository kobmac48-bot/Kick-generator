#pragma once

#include "PluginProcessor.h"

#include <array>
#include <memory>

class KickForgeAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit KickForgeAudioProcessorEditor(KickForgeAudioProcessor&);
    ~KickForgeAudioProcessorEditor() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void randomizeKick();

    KickForgeAudioProcessor& processor;
    juce::ComboBox genreBox;
    juce::Label genreLabel;
    juce::TextButton randomizeButton { "RANDOM KICK" };
    std::array<juce::Slider, 8> sliders;
    std::array<juce::Label, 8> labels;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::array<std::unique_ptr<SliderAttachment>, 8> sliderAttachments;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> genreAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(KickForgeAudioProcessorEditor)
};
