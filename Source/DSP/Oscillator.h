#pragma once
#include <cmath>
#include <juce_core/juce_core.h>

namespace hunterice
{

class Oscillator
{
public:
    enum class Waveform
    {
        Sine = 0,
        Triangle,
        Saw,
        Square,
        NumWaveforms
    };

    Oscillator() = default;

    void prepare(double sampleRate) noexcept
    {
        sampleRate_ = sampleRate;
        phase_ = 0.0;
        phaseIncrement_ = 0.0;
    }

    void setFrequency(float frequency) noexcept
    {
        if (sampleRate_ > 0.0)
            phaseIncrement_ = static_cast<double>(frequency) / sampleRate_;
    }

    void setWaveform(Waveform w) noexcept { waveform_ = w; }
    void setPulseWidth(float pw) noexcept { pulseWidth_ = juce::jlimit(0.05f, 0.95f, pw); }

    // Полифоновый сброс фазы
    void resetPhase() noexcept { phase_ = 0.0; }

    // Генерация одного сэмпла с PolyBLEP-антиалиасингом
    float processSample() noexcept
    {
        double sample = 0.0;

        switch (waveform_)
        {
            case Waveform::Sine:
                sample = std::sin(2.0 * juce::MathConstants<double>::pi * phase_);
                break;

            case Waveform::Triangle:
            {
                // Полибандлимитированная треугольная волна
                double t = phase_;
                if (t < 0.25)
                    sample = 4.0 * t;
                else if (t < 0.75)
                    sample = 2.0 - 4.0 * t;
                else
                    sample = -4.0 + 4.0 * t;
                break;
            }

            case Waveform::Saw:
            {
                // 2x - 1 с PolyBLEP
                sample = 2.0 * phase_ - 1.0;
                sample -= polyBLEP(phase_, phaseIncrement_);
                break;
            }

            case Waveform::Square:
            {
                double pw = static_cast<double>(pulseWidth_);
                sample = (phase_ < pw) ? 1.0 : -1.0;
                sample += polyBLEP(phase_, phaseIncrement_);
                double shiftedPhase = phase_ + (1.0 - pw);
                if (shiftedPhase >= 1.0) shiftedPhase -= 1.0;
                sample -= polyBLEP(shiftedPhase, phaseIncrement_);
                break;
            }

            default:
                sample = 0.0;
        }

        // Инкремент фазы
        phase_ += phaseIncrement_;
        if (phase_ >= 1.0) phase_ -= 1.0;

        return static_cast<float>(sample);
    }

private:
    // PolyBLEP — подавление алиасинга на крутых фронтах
    double polyBLEP(double t, double dt) const noexcept
    {
        if (dt <= 0.0) return 0.0;
        if (t < dt)
        {
            t /= dt;
            return t + t - t * t - 1.0;
        }
        if (t > 1.0 - dt)
        {
            t = (t - 1.0) / dt;
            return t * t + t + t + 1.0;
        }
        return 0.0;
    }

    double sampleRate_ = 44100.0;
    double phase_ = 0.0;
    double phaseIncrement_ = 0.0;
    float pulseWidth_ = 0.5f;
    Waveform waveform_ = Waveform::Saw;
};

} // namespace hunterice
