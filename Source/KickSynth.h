#pragma once

#include <cstdint>

struct KickSoundParameters
{
    float pitchEnd = 48.0f;
    float decay = 0.42f;
    float drive = 8.0f;
    float punch = 0.65f;
    float body = 0.55f;
    float distortion = 0.65f;
    float softClip = 0.38f;
    float brightness = 1.0f;
    float distortionScale = 1.0f;
    float softClipScale = 1.0f;
    float sweepScale = 1.0f;
    float tailToneScale = 1.0f;
};

class KickSynth
{
public:
    void trigger(float noteVelocity, float initialPitch, std::uint32_t noiseSeed) noexcept;
    void stop() noexcept;
    float render(float sampleRate, const KickSoundParameters& parameters) noexcept;

private:
    bool active = false;
    float velocity = 1.0f;
    float phase = 0.0f;
    float clickPhase = 0.0f;
    float elapsed = 0.0f;
    float pitchStart = 165.0f;
    float previousNoise = 0.0f;
    float tailFilter = 0.0f;
    std::uint32_t noiseState = 1;
};
