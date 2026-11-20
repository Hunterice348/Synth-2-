#pragma once
#include <juce_core/juce_core.h>

namespace hunterice
{

class ADSREnvelope
{
public:
    struct Parameters
    {
        float attack  = 0.01f;   // секунды
        float decay   = 0.2f;
        float sustain = 0.7f;    // 0..1
        float release = 0.3f;
    };

    ADSREnvelope() = default;

    void prepare(double sampleRate) noexcept
    {
        sampleRate_ = sampleRate;
        state_ = State::Idle;
        currentValue_ = 0.0f;
    }

    void setParameters(const Parameters& p) noexcept { params_ = p; }

    void noteOn() noexcept
    {
        state_ = State::Attack;
        currentValue_ = 0.0f;
    }

    void noteOff() noexcept
    {
        if (state_ != State::Idle)
            state_ = State::Release;
    }

    bool isActive() const noexcept { return state_ != State::Idle; }

    // Возвращает значение огибающей 0..1
    float processSample() noexcept
    {
        switch (state_)
        {
            case State::Idle:    return 0.0f;
            case State::Attack:  return processAttack();
            case State::Decay:   return processDecay();
            case State::Sustain: return params_.sustain;
            case State::Release: return processRelease();
        }
        return 0.0f;
    }

    void reset() noexcept
    {
        state_ = State::Idle;
        currentValue_ = 0.0f;
    }

private:
    enum class State { Idle, Attack, Decay, Sustain, Release };

    float processAttack() noexcept
    {
        float rate = computeRate(params_.attack);
        currentValue_ += rate;
        if (currentValue_ >= 1.0f)
        {
            currentValue_ = 1.0f;
            state_ = State::Decay;
        }
        return currentValue_;
    }

    float processDecay() noexcept
    {
        float rate = computeRate(params_.decay);
        currentValue_ -= rate;
        float target = params_.sustain;
        if (currentValue_ <= target)
        {
            currentValue_ = target;
            state_ = State::Sustain;
        }
        return currentValue_;
    }

    float processRelease() noexcept
    {
        float rate = computeRate(params_.release);
        currentValue_ -= rate;
        if (currentValue_ <= 0.0f)
        {
            currentValue_ = 0.0f;
            state_ = State::Idle;
        }
        return currentValue_;
    }

    float computeRate(float timeSeconds) const noexcept
    {
        if (timeSeconds <= 0.001f) return 1.0f;
        return static_cast<float>(1.0 / (sampleRate_ * timeSeconds));
    }

    double sampleRate_ = 44100.0;
    Parameters params_;
    State state_ = State::Idle;
    float currentValue_ = 0.0f;
};

} // namespace hunterice
