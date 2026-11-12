#pragma once

#include <array>
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
    float bounce = 0.0f;
    float clickScale = 1.0f;
    float clickFrequencyScale = 1.0f;
};

struct KickGenreProfile
{
    float pitchStartScale;
    float pitchEndScale;
    float decayScale;
    float driveScale;
    float punchScale;
    float brightness;
    float distortionScale;
    float softClipScale;
    float sweepScale;
    float tailToneScale;
    float bounce;
    float clickScale;
    float clickFrequencyScale;
};

inline constexpr std::array<KickGenreProfile, 8> kickGenreProfiles {{
    { 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 0.90f, 0.92f, 0.90f, 1.00f, 1.00f, 0.08f, 1.00f, 1.00f }, // Gabber
    { 1.08f, 0.92f, 0.78f, 1.32f, 1.14f, 1.18f, 1.20f, 1.12f, 0.78f, 1.12f, 0.12f, 1.08f, 1.00f }, // Uptempo
    { 0.92f, 0.82f, 0.92f, 1.45f, 0.92f, 1.24f, 1.16f, 1.10f, 0.88f, 1.20f, 0.08f, 0.96f, 1.00f }, // Zaag
    { 1.08f, 1.08f, 1.12f, 1.18f, 1.08f, 1.02f, 1.05f, 1.00f, 1.05f, 1.00f, 0.06f, 0.98f, 1.00f }, // Rawstyle
    { 1.12f, 0.86f, 0.84f, 1.52f, 1.10f, 1.30f, 1.32f, 1.24f, 0.72f, 1.18f, 0.10f, 1.18f, 1.10f }, // Deutsche Krach
    { 1.16f, 0.98f, 0.80f, 1.10f, 1.25f, 1.18f, 0.98f, 0.96f, 0.62f, 1.05f, 0.04f, 1.35f, 1.35f }, // Frenchcore
    { 1.22f, 0.88f, 0.62f, 1.58f, 1.10f, 1.36f, 1.30f, 1.24f, 0.70f, 1.22f, 0.08f, 1.22f, 1.05f }, // Terror
    { 1.06f, 0.92f, 0.88f, 1.28f, 1.18f, 1.16f, 1.12f, 1.08f, 0.92f, 1.12f, 0.92f, 1.16f, 1.00f }  // Bouncy Uptempo
}};

inline constexpr std::array<const char*, 8> kickGenreNames {{
    "Gabber", "Uptempo", "Zaag", "Rawstyle", "Deutsche Krach",
    "Frenchcore", "Terror", "Bouncy Uptempo"
}};

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
