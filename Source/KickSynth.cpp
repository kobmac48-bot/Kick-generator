#include "KickSynth.h"

#include <algorithm>
#include <cmath>

namespace
{
constexpr float twoPi = 6.28318530718f;

float finiteOr(float value, float fallback) noexcept
{
    return std::isfinite(value) ? value : fallback;
}

float bounded(float value, float low, float high, float fallback) noexcept
{
    return std::clamp(finiteOr(value, fallback), low, high);
}
}

void KickSynth::trigger(float noteVelocity, float initialPitch, std::uint32_t seed) noexcept
{
    active = true;
    velocity = bounded(noteVelocity, 0.0f, 1.0f, 1.0f);
    phase = 0.0f;
    clickPhase = 0.0f;
    elapsed = 0.0f;
    pitchStart = bounded(initialPitch, 35.0f, 1600.0f, 165.0f);
    previousNoise = 0.0f;
    tailFilter = 0.0f;
    noiseState = seed == 0 ? 1u : seed;
}

void KickSynth::stop() noexcept
{
    active = false;
    tailFilter = 0.0f;
    previousNoise = 0.0f;
}

float KickSynth::render(float sampleRate, const KickSoundParameters& parameters) noexcept
{
    if (!active)
        return 0.0f;

    sampleRate = bounded(sampleRate, 8000.0f, 384000.0f, 44100.0f);
    const auto decay = bounded(parameters.decay, 0.08f, 1.5f, 0.42f);
    const auto punch = bounded(parameters.punch, 0.0f, 1.0f, 0.65f);
    const auto body = bounded(parameters.body, 0.0f, 1.0f, 0.55f);
    const auto distortion = bounded(parameters.distortion, 0.0f, 1.0f, 0.65f)
                          * bounded(parameters.distortionScale, 0.0f, 2.0f, 1.0f);
    const auto softClip = bounded(parameters.softClip, 0.0f, 1.0f, 0.38f)
                        * bounded(parameters.softClipScale, 0.0f, 2.0f, 1.0f);
    const auto brightness = bounded(parameters.brightness, 0.4f, 1.8f, 1.0f);
    const auto pitchEnd = bounded(parameters.pitchEnd, 25.0f, sampleRate * 0.25f, 48.0f);
    const auto drive = bounded(parameters.drive, 1.0f, 36.0f, 8.0f);
    const auto tailProgress = std::clamp(elapsed / decay, 0.0f, 1.0f);
    const auto sweepScale = bounded(parameters.sweepScale, 0.5f, 1.5f, 1.0f);
    const auto sweepTime = (0.010f + 0.032f * (1.0f - punch)) * sweepScale;
    const auto pitch = pitchEnd + (std::min(pitchStart, sampleRate * 0.40f) - pitchEnd)
                                  * std::exp(-elapsed / sweepTime);
    phase += twoPi * pitch / sampleRate;
    if (phase >= twoPi)
        phase -= twoPi;

    const auto envelope = std::exp(-5.4f * elapsed / decay);
    const auto fundamental = std::sin(phase);
    const auto harmonics = fundamental
                         + std::sin(phase * 2.0f) * (0.16f + body * 0.20f)
                         + std::sin(phase * 3.0f) * (0.06f + body * 0.13f) * brightness
                         + std::sin(phase * 5.0f) * (0.025f + body * 0.055f) * brightness;

    noiseState ^= noiseState << 13;
    noiseState ^= noiseState >> 17;
    noiseState ^= noiseState << 5;
    const auto noise = static_cast<float>(noiseState & 0xffffu) / 32767.5f - 1.0f;
    const auto highPassedNoise = noise - previousNoise;
    previousNoise = noise;
    const auto clickEnvelope = std::exp(-elapsed / (0.0035f + punch * 0.0065f));
    const auto clickFrequencyScale = bounded(parameters.clickFrequencyScale, 0.7f, 1.6f, 1.0f);
    const auto clickFrequency = std::min(sampleRate * 0.30f,
                                          (2400.0f + brightness * 500.0f) * clickFrequencyScale);
    clickPhase += twoPi * clickFrequency / sampleRate;
    if (clickPhase >= twoPi)
        clickPhase -= twoPi;
    const auto clickScale = bounded(parameters.clickScale, 0.5f, 1.6f, 1.0f);
    const auto click = clickEnvelope * punch * clickScale
                     * (highPassedNoise * 0.105f + std::sin(clickPhase) * 0.19f);

    const auto baseDrive = 1.0f + (drive - 1.0f) * 0.40f;
    const auto cleanBody = std::tanh(harmonics * baseDrive);
    const auto tailDrive = baseDrive + distortion * 23.0f * (0.70f + 0.30f * tailProgress);
    const auto saturatedBody = std::tanh(harmonics * tailDrive);
    const auto shapedBody = cleanBody + (saturatedBody - cleanBody)
                                     * std::clamp(distortion, 0.0f, 1.0f);

    const auto toneScale = bounded(parameters.tailToneScale, 0.6f, 1.5f, 1.0f);
    const auto cutoff = std::clamp((1550.0f + 10500.0f
                                  * std::exp(-elapsed / (decay * 0.34f)))
                                  * brightness * toneScale,
                                   700.0f, sampleRate * 0.42f);
    const auto filterCoefficient = 1.0f - std::exp(-twoPi * cutoff / sampleRate);
    tailFilter += filterCoefficient * (shapedBody - tailFilter);

    const auto sub = fundamental * (0.30f + 0.12f * body);
    const auto bounce = bounded(parameters.bounce, 0.0f, 1.0f, 0.0f);
    const auto bouncePhase = std::min(twoPi * 10.5f * elapsed, twoPi * 12.0f);
    const auto bounceDip = 1.0f - bounce * 0.38f
                                      * (0.5f - 0.5f * std::cos(bouncePhase))
                                      * std::exp(-elapsed / (decay * 0.72f));
    auto bodySignal = (sub + tailFilter * 0.78f) * envelope * bounceDip;
    const auto clipAmount = std::clamp(softClip, 0.0f, 1.0f);
    const auto clipThreshold = 1.0f - clipAmount * 0.68f;
    const auto clippedBody = clipThreshold * std::tanh(bodySignal / clipThreshold);
    bodySignal += (clippedBody - bodySignal) * clipAmount;

    elapsed += 1.0f / sampleRate;
    if (elapsed >= decay * 1.5f)
        active = false;

    return 0.94f * std::tanh((bodySignal + click) * 1.25f) * velocity;
}
