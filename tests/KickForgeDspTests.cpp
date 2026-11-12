#include "KickSynth.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace
{
void require(bool condition, const char* message)
{
    if (!condition)
        throw std::runtime_error(message);
}

struct RenderStats
{
    float peak = 0.0f;
    float transientRms = 0.0f;
    float bodyRms = 0.0f;
    float lateRms = 0.0f;
    int nonZeroSamples = 0;
};

RenderStats renderKick(const KickSoundParameters& parameters, int sampleCount)
{
    constexpr float sampleRate = 48000.0f;
    KickSynth synth;
    synth.trigger(1.0f, 520.0f, 0x12345678u);
    RenderStats stats;
    double transientEnergy = 0.0;
    double bodyEnergy = 0.0;
    double lateEnergy = 0.0;

    for (int sample = 0; sample < sampleCount; ++sample)
    {
        const auto value = synth.render(sampleRate, parameters);
        require(std::isfinite(value), "render produced a non-finite sample");
        require(std::abs(value) <= 0.941f, "render exceeded the output ceiling");

        const auto magnitude = std::abs(value);
        stats.peak = std::max(stats.peak, magnitude);
        stats.nonZeroSamples += magnitude > 1.0e-6f ? 1 : 0;
        const auto time = static_cast<float>(sample) / sampleRate;
        if (time < 0.012f)
            transientEnergy += value * value;
        else if (time < 0.12f)
            bodyEnergy += value * value;
        else if (time < 0.28f)
            lateEnergy += value * value;
    }

    stats.transientRms = static_cast<float>(std::sqrt(transientEnergy / (sampleRate * 0.012)));
    stats.bodyRms = static_cast<float>(std::sqrt(bodyEnergy / (sampleRate * 0.108)));
    stats.lateRms = static_cast<float>(std::sqrt(lateEnergy / (sampleRate * 0.16)));
    return stats;
}

KickSoundParameters parametersFor(const KickGenreProfile& profile)
{
    KickSoundParameters parameters;
    parameters.pitchEnd = 48.0f * profile.pitchEndScale;
    parameters.decay = 0.34f * profile.decayScale;
    parameters.drive = 14.0f * profile.driveScale;
    parameters.punch = 0.86f * profile.punchScale;
    parameters.body = 0.58f;
    parameters.distortion = 0.84f;
    parameters.softClip = 0.66f;
    parameters.brightness = profile.brightness;
    parameters.distortionScale = profile.distortionScale;
    parameters.softClipScale = profile.softClipScale;
    parameters.sweepScale = profile.sweepScale;
    parameters.tailToneScale = profile.tailToneScale;
    parameters.bounce = profile.bounce;
    parameters.clickScale = profile.clickScale;
    parameters.clickFrequencyScale = profile.clickFrequencyScale;
    return parameters;
}

double bounceContrast(const KickSoundParameters& parameters)
{
    constexpr float sampleRate = 48000.0f;
    constexpr int sampleCount = 18000;
    auto bouncyParameters = parameters;
    auto straightParameters = parameters;
    straightParameters.bounce = 0.0f;
    KickSynth bouncy;
    KickSynth straight;
    bouncy.trigger(1.0f, 520.0f, 0x12345678u);
    straight.trigger(1.0f, 520.0f, 0x12345678u);
    double differenceEnergy = 0.0;
    double straightEnergy = 0.0;

    for (int sample = 0; sample < sampleCount; ++sample)
    {
        const auto bouncySample = bouncy.render(sampleRate, bouncyParameters);
        const auto straightSample = straight.render(sampleRate, straightParameters);
        if (sample >= static_cast<int>(sampleRate * 0.025f))
        {
            const auto difference = bouncySample - straightSample;
            differenceEnergy += difference * difference;
            straightEnergy += straightSample * straightSample;
        }
    }
    return std::sqrt(differenceEnergy / std::max(straightEnergy, 1.0e-12));
}

void checkHardstyleTransientAndTail()
{
    KickSoundParameters hardstyle;
    hardstyle.pitchEnd = 48.0f;
    hardstyle.decay = 0.34f;
    hardstyle.drive = 15.0f;
    hardstyle.punch = 0.92f;
    hardstyle.body = 0.62f;
    hardstyle.distortion = 0.90f;
    hardstyle.softClip = 0.72f;
    hardstyle.brightness = 1.18f;
    hardstyle.distortionScale = 1.20f;
    hardstyle.softClipScale = 1.12f;
    hardstyle.sweepScale = 0.78f;
    hardstyle.tailToneScale = 1.12f;

    const auto stats = renderKick(hardstyle, 24000);
    require(stats.peak > 0.45f, "kick transient/body is too quiet");
    require(stats.transientRms > 0.035f, "kick click transient is missing");
    require(stats.bodyRms > 0.06f, "kick body is missing");
    require(stats.lateRms > 0.008f, "kick distorted tail is missing");
    require(stats.nonZeroSamples > 5000, "kick is silent or ends too early");
}

void checkOtherGenreSettingsAndSilenceAfterTail()
{
    KickSoundParameters gabber;
    gabber.pitchEnd = 52.0f;
    gabber.decay = 0.48f;
    gabber.drive = 10.0f;
    gabber.punch = 0.78f;
    gabber.body = 0.58f;
    gabber.distortion = 0.62f;
    gabber.softClip = 0.42f;
    gabber.brightness = 0.90f;

    const auto stats = renderKick(gabber, 48000);
    require(stats.peak > 0.35f, "Gabber profile render is too quiet");

    KickSynth synth;
    synth.trigger(1.0f, 400.0f, 7u);
    for (int sample = 0; sample < 48000; ++sample)
        (void) synth.render(48000.0f, gabber);
    for (int sample = 0; sample < 64; ++sample)
        require(synth.render(48000.0f, gabber) == 0.0f, "voice remained active after its tail");
}

int countPositiveCrossings(const std::vector<float>& audio, int start, int end)
{
    auto crossings = 0;
    for (auto sample = start + 1; sample < end; ++sample)
    {
        if (audio[static_cast<std::size_t>(sample - 1)] <= 0.0f
            && audio[static_cast<std::size_t>(sample)] > 0.0f)
            ++crossings;
    }
    return crossings;
}

void checkPitchDropsQuicklyIntoLowEnd()
{
    constexpr float sampleRate = 48000.0f;
    KickSoundParameters clean;
    clean.pitchEnd = 50.0f;
    clean.decay = 0.4f;
    clean.drive = 1.0f;
    clean.punch = 0.0f;
    clean.body = 0.0f;
    clean.distortion = 0.0f;
    clean.softClip = 0.0f;

    KickSynth synth;
    synth.trigger(1.0f, 700.0f, 19u);
    std::vector<float> audio(12000);
    for (auto& sample : audio)
        sample = synth.render(sampleRate, clean);

    const auto earlyCycles = countPositiveCrossings(audio, 240, 1200);
    const auto lateCycles = countPositiveCrossings(audio, 7200, 12000);
    const auto earlyFrequency = earlyCycles * sampleRate / (1200 - 240);
    const auto lateFrequency = lateCycles * sampleRate / (12000 - 7200);
    require(earlyFrequency > lateFrequency * 4.0f,
            "pitch sweep did not fall rapidly from the initial thump into the low tail");
    require(lateFrequency < 90.0f, "kick tail did not settle into the low-end pitch");
}

void checkInvalidParametersStaySafe()
{
    KickSoundParameters invalid;
    invalid.decay = NAN;
    invalid.pitchEnd = INFINITY;
    invalid.drive = NAN;
    invalid.distortion = INFINITY;
    invalid.softClip = NAN;
    KickSynth synth;
    synth.trigger(NAN, INFINITY, 0u);
    for (int sample = 0; sample < 1024; ++sample)
    {
        const auto value = synth.render(48000.0f, invalid);
        require(std::isfinite(value), "invalid parameter caused a non-finite output");
        require(std::abs(value) <= 0.941f, "invalid parameter exceeded the output ceiling");
    }
}

void checkDistinctHardcoreGenreProfiles()
{
    const auto uptempo = parametersFor(kickGenreProfiles[1]);
    const auto krach = parametersFor(kickGenreProfiles[4]);
    const auto frenchcore = parametersFor(kickGenreProfiles[5]);
    const auto bouncy = parametersFor(kickGenreProfiles[7]);

    require(std::string_view(kickGenreNames[1]) == "Uptempo", "standard Uptempo profile was renamed");
    require(std::string_view(kickGenreNames[4]) == "Deutsche Krach", "Deutsche Krach profile is missing");
    require(std::string_view(kickGenreNames[5]) == "Frenchcore", "Frenchcore profile is missing");
    require(std::string_view(kickGenreNames[7]) == "Bouncy Uptempo", "bouncy Uptempo mode is missing");
    require(kickGenreProfiles[7].bounce > kickGenreProfiles[1].bounce * 5.0f,
            "bouncy Uptempo does not have a distinct tail pulse profile");
    require(kickGenreProfiles[5].sweepScale < kickGenreProfiles[1].sweepScale
            && kickGenreProfiles[5].clickScale > kickGenreProfiles[1].clickScale,
            "Frenchcore profile lacks a tighter sweep and sharper click");
    require(kickGenreProfiles[4].brightness > kickGenreProfiles[1].brightness
            && kickGenreProfiles[4].distortionScale > kickGenreProfiles[1].distortionScale,
            "Deutsche Krach profile lacks its brighter, crunchier character");

    const auto bouncyContrast = bounceContrast(bouncy);
    require(bouncyContrast > 0.06,
            "bouncy Uptempo rendered tail lacks a measurable rhythmic envelope pulse");

    const auto uptempoStats = renderKick(uptempo, 24000);
    const auto frenchStats = renderKick(frenchcore, 24000);
    const auto krachStats = renderKick(krach, 24000);
    require(frenchStats.transientRms > 0.025f && krachStats.lateRms > 0.008f,
            "Frenchcore transient or Deutsche Krach tail is not audible");
    require(frenchStats.transientRms > uptempoStats.transientRms * 1.05f,
            "Frenchcore render lacks its measured sharper click transient");
}
}

int main()
{
    try
    {
        checkHardstyleTransientAndTail();
        checkOtherGenreSettingsAndSilenceAfterTail();
        checkPitchDropsQuicklyIntoLowEnd();
        checkInvalidParametersStaySafe();
        checkDistinctHardcoreGenreProfiles();
        std::cout << "KickForge DSP render tests passed.\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "KickForge DSP render test failed: " << error.what() << '\n';
        return 1;
    }
}
