#include "PluginEditor.h"

#include <cmath>

namespace
{
const std::array<juce::String, 8> parameterIds {
    "pitchStart", "pitchEnd", "decay", "drive", "punch", "body",
    "distortion", "softClip"
};

const std::array<juce::String, 8> parameterLabels {
    "PITCH START", "PITCH END", "DECAY", "DRIVE", "PUNCH", "BODY",
    "DISTORTION", "SOFT CLIP"
};

struct ParameterRange
{
    float minimum;
    float maximum;
};

using KickRanges = std::array<ParameterRange, 8>;

constexpr std::array<KickRanges, 8> genreRanges {{
    KickRanges {{ { 240.0f, 430.0f }, { 42.0f, 58.0f }, { 0.28f, 0.55f },
                  { 8.0f, 13.0f }, { 0.62f, 0.96f }, { 0.40f, 0.78f },
                  { 48.0f, 78.0f }, { 26.0f, 58.0f } }},
    KickRanges {{ { 320.0f, 640.0f }, { 40.0f, 62.0f }, { 0.18f, 0.36f },
                  { 11.0f, 17.0f }, { 0.78f, 1.00f }, { 0.35f, 0.72f },
                  { 72.0f, 96.0f }, { 48.0f, 82.0f } }},
    KickRanges {{ { 260.0f, 560.0f }, { 36.0f, 58.0f }, { 0.22f, 0.45f },
                  { 11.0f, 17.0f }, { 0.58f, 0.88f }, { 0.28f, 0.66f },
                  { 68.0f, 94.0f }, { 42.0f, 76.0f } }},
    KickRanges {{ { 210.0f, 450.0f }, { 46.0f, 80.0f }, { 0.34f, 0.66f },
                  { 7.0f, 12.0f }, { 0.58f, 0.94f }, { 0.42f, 0.86f },
                  { 50.0f, 82.0f }, { 28.0f, 62.0f } }},
    KickRanges {{ { 340.0f, 720.0f }, { 36.0f, 58.0f }, { 0.18f, 0.38f },
                  { 13.0f, 18.0f }, { 0.82f, 1.00f }, { 0.38f, 0.78f },
                  { 80.0f, 100.0f }, { 62.0f, 92.0f } }},
    KickRanges {{ { 240.0f, 500.0f }, { 44.0f, 74.0f }, { 0.30f, 0.62f },
                  { 7.0f, 12.0f }, { 0.65f, 1.00f }, { 0.42f, 0.84f },
                  { 42.0f, 72.0f }, { 20.0f, 54.0f } }},
    KickRanges {{ { 380.0f, 760.0f }, { 36.0f, 64.0f }, { 0.14f, 0.32f },
                  { 12.0f, 18.0f }, { 0.72f, 1.00f }, { 0.28f, 0.66f },
                  { 76.0f, 98.0f }, { 56.0f, 90.0f } }},
    KickRanges {{ { 300.0f, 620.0f }, { 40.0f, 62.0f }, { 0.22f, 0.42f },
                  { 10.0f, 16.0f }, { 0.74f, 1.00f }, { 0.36f, 0.72f },
                  { 68.0f, 92.0f }, { 42.0f, 76.0f } }}
}};
}

KickForgeAudioProcessorEditor::KickForgeAudioProcessorEditor(KickForgeAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setSize(850, 420);

    genreLabel.setText("STYLE", juce::dontSendNotification);
    genreLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    genreLabel.setFont(juce::Font(12.0f, juce::Font::bold));
    addAndMakeVisible(genreLabel);

    genreBox.addItemList(
        juce::StringArray(kickGenreNames.data(), static_cast<int>(kickGenreNames.size())), 1);
    addAndMakeVisible(genreBox);
    genreAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        processor.parameters, "genre", genreBox);

    randomizeButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xffef6a3a));
    randomizeButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    randomizeButton.onClick = [this] { randomizeKick(); };
    addAndMakeVisible(randomizeButton);

    for (std::size_t index = 0; index < sliders.size(); ++index)
    {
        auto& slider = sliders[index];
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 82, 22);
        if (index >= 6)
            slider.setNumDecimalPlacesToDisplay(0);
        slider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xffef6a3a));
        slider.setColour(juce::Slider::thumbColourId, juce::Colours::white);
        addAndMakeVisible(slider);

        auto& label = labels[index];
        label.setText(parameterLabels[index], juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        label.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
        label.setFont(juce::Font(10.0f, juce::Font::bold));
        addAndMakeVisible(label);

        sliderAttachments[index] =
            std::make_unique<SliderAttachment>(processor.parameters, parameterIds[index], slider);
    }
}

void KickForgeAudioProcessorEditor::randomizeKick()
{
    const auto selectedGenre = processor.parameters.getRawParameterValue("genre")->load();
    const auto genreIndex = juce::jlimit(0, static_cast<int>(genreRanges.size()) - 1,
        static_cast<int>(std::lround(selectedGenre)));
    auto& random = juce::Random::getSystemRandom();

    for (std::size_t index = 0; index < parameterIds.size(); ++index)
    {
        auto* parameter = processor.parameters.getParameter(parameterIds[index]);
        const auto range = genreRanges[static_cast<std::size_t>(genreIndex)][index];
        const auto value = range.minimum + random.nextFloat() * (range.maximum - range.minimum);
        parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
    }

    processor.previewKick();
}

void KickForgeAudioProcessorEditor::paint(juce::Graphics& graphics)
{
    graphics.fillAll(juce::Colour(0xff17191d));
    graphics.setColour(juce::Colour(0xffef6a3a));
    graphics.fillRect(0, 0, getWidth(), 5);

    graphics.setColour(juce::Colours::white);
    graphics.setFont(juce::Font(28.0f, juce::Font::bold));
    graphics.drawText("KICKFORGE", 24, 22, 300, 38, juce::Justification::left);

    graphics.setColour(juce::Colour(0xffa4a8ae));
    graphics.setFont(juce::Font(12.0f));
    graphics.drawText("HARDCORE KICK SYNTH  |  DISTORTION + SOFT CLIP",
                      26, 58, 480, 20, juce::Justification::left);
    graphics.drawText("Randomize and preview a kick, or play MIDI notes to trigger kicks",
                      26, getHeight() - 28,
                      getWidth() - 52, 18, juce::Justification::centredLeft);
}

void KickForgeAudioProcessorEditor::resized()
{
    genreLabel.setBounds(24, 100, 100, 18);
    genreBox.setBounds(24, 122, getWidth() - 48, 34);
    randomizeButton.setBounds(24, 172, getWidth() - 48, 38);

    const auto columnWidth = (getWidth() - 48) / static_cast<int>(sliders.size());
    for (std::size_t index = 0; index < sliders.size(); ++index)
    {
        const auto x = 24 + static_cast<int>(index) * columnWidth;
        labels[index].setBounds(x, 232, columnWidth, 20);
        sliders[index].setBounds(x + 2, 252, columnWidth - 4, 116);
    }
}
