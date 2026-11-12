#include "PluginEditor.h"

#include <cmath>

namespace
{
const std::array<juce::String, 6> parameterIds {
    "pitchStart", "pitchEnd", "decay", "drive", "punch", "body"
};

const std::array<juce::String, 6> parameterLabels {
    "PITCH START", "PITCH END", "DECAY", "DRIVE", "PUNCH", "BODY"
};

struct ParameterRange
{
    float minimum;
    float maximum;
};

using KickRanges = std::array<ParameterRange, 6>;

constexpr std::array<KickRanges, 7> genreRanges {{
    KickRanges {{ { 135.0f, 210.0f }, { 42.0f, 58.0f }, { 0.32f, 0.62f },
                  { 6.5f, 11.5f }, { 0.45f, 0.85f }, { 0.35f, 0.75f } }},
    KickRanges {{ { 165.0f, 255.0f }, { 42.0f, 70.0f }, { 0.20f, 0.42f },
                  { 7.5f, 14.0f }, { 0.55f, 0.95f }, { 0.25f, 0.70f } }},
    KickRanges {{ { 105.0f, 185.0f }, { 35.0f, 58.0f }, { 0.25f, 0.55f },
                  { 9.0f, 16.0f }, { 0.35f, 0.70f }, { 0.25f, 0.65f } }},
    KickRanges {{ { 145.0f, 240.0f }, { 48.0f, 82.0f }, { 0.38f, 0.72f },
                  { 6.0f, 12.0f }, { 0.50f, 0.90f }, { 0.40f, 0.85f } }},
    KickRanges {{ { 125.0f, 220.0f }, { 38.0f, 66.0f }, { 0.22f, 0.48f },
                  { 9.0f, 17.0f }, { 0.45f, 0.85f }, { 0.25f, 0.70f } }},
    KickRanges {{ { 155.0f, 250.0f }, { 46.0f, 76.0f }, { 0.34f, 0.72f },
                  { 6.0f, 12.0f }, { 0.60f, 1.00f }, { 0.40f, 0.85f } }},
    KickRanges {{ { 180.0f, 290.0f }, { 38.0f, 65.0f }, { 0.14f, 0.36f },
                  { 10.0f, 18.0f }, { 0.55f, 1.00f }, { 0.20f, 0.60f } }}
}};
}

KickForgeAudioProcessorEditor::KickForgeAudioProcessorEditor(KickForgeAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setSize(650, 420);

    genreLabel.setText("STYLE", juce::dontSendNotification);
    genreLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    genreLabel.setFont(juce::Font(12.0f, juce::Font::bold));
    addAndMakeVisible(genreLabel);

    genreBox.addItemList({ "Gabber", "Uptempo", "Zaag", "Rawstyle",
                           "Deutsche Krach", "Frenchcore", "Terror" }, 1);
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
    graphics.drawText("HARDCORE KICK SYNTH", 26, 58, 280, 20, juce::Justification::left);
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
