#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "DSP/Oscillator.h"
#include "DSP/ADSREnvelope.h"

namespace hunterice
{

// Один голос синтезатора
struct Voice
{
    Oscillator oscillator;
    ADSREnvelope envelope;
    double noteHz = 0.0;
    int  midiNote = -1;
    bool active = false;

    void prepare(double sampleRate)
    {
        oscillator.prepare(sampleRate);
        envelope.prepare(sampleRate);
    }

    void noteOn(int note, float velocity)
    {
        midiNote = note;
        noteHz = juce::MidiMessage::getMidiNoteInHertz(note);
        oscillator.setFrequency(static_cast<float>(noteHz));
        oscillator.resetPhase();
        envelope.noteOn();
        active = true;
    }

    void noteOff()
    {
        envelope.noteOff();
    }
};

class HunterIceAbyssProcessor : public juce::AudioProcessor
{
public:
    HunterIceAbyssProcessor();
    ~HunterIceAbyssProcessor() override = default;

    // AudioProcessor
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override          { return "HUNTERICE ABYSS"; }
    bool acceptsMidi() const override                    { return true; }
    bool producesMidi() const override                   { return false; }
    double getTailLengthSeconds() const override         { return 0.5; }

    int getNumPrograms() override                        { return 1; }
    int getCurrentProgram() override                     { return 0; }
    void setCurrentProgram(int) override                 {}
    const juce::String getProgramName(int) override      { return "Default"; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // Параметры, доступные редактору
    juce::AudioProcessorValueTreeState apvts;

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Полифония
    static constexpr int kMaxVoices = 16;
    std::array<Voice, kMaxVoices> voices_;
    double currentSampleRate_ = 44100.0;

    // Глобальные сглаженные параметры
    juce::SmoothedValue<float> smoothedMaster_;
    juce::SmoothedValue<float> smoothedOscLevel_;

    juce::CriticalSection voiceLock_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HunterIceAbyssProcessor)
};

} // namespace hunterice
