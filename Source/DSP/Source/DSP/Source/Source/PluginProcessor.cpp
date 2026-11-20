#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace hunterice
{

using APVTS = juce::AudioProcessorValueTreeState;

HunterIceAbyssProcessor::HunterIceAbyssProcessor()
    : AudioProcessor(BusesProperties()
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

APVTS::ParameterLayout HunterIceAbyssProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    auto rangeNorm = juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f);

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"master", 1}, "Master Volume", rangeNorm, 0.7f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"oscLevel", 1}, "Oscillator Level", rangeNorm, 0.8f));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"waveform", 1}, "Waveform",
        juce::StringArray{"Sine", "Triangle", "Saw", "Square"}, 2));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"pulseWidth", 1}, "Pulse Width",
        juce::NormalisableRange<float>(0.05f, 0.95f, 0.001f), 0.5f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"attack", 1}, "Attack",
        juce::NormalisableRange<float>(0.001f, 5.0f, 0.001f, 0.3f), 0.01f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"decay", 1}, "Decay",
        juce::NormalisableRange<float>(0.001f, 5.0f, 0.001f, 0.3f), 0.2f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"sustain", 1}, "Sustain", rangeNorm, 0.7f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"release", 1}, "Release",
        juce::NormalisableRange<float>(0.001f, 10.0f, 0.001f, 0.3f), 0.3f));

    return {params.begin(), params.end()};
}

void HunterIceAbyssProcessor::prepareToPlay(double sampleRate, int /*samplesPerBlock*/)
{
    currentSampleRate_ = sampleRate;
    for (auto& v : voices_)
        v.prepare(sampleRate);

    smoothedMaster_.reset(sampleRate, 0.02);
    smoothedOscLevel_.reset(sampleRate, 0.02);
}

void HunterIceAbyssProcessor::releaseResources() {}

bool HunterIceAbyssProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    return true;
}

void HunterIceAbyssProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                           juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    // Читаем параметры один раз на блок
    const float masterTarget  = *apvts.getRawParameterValue("master");
    const float oscLevelTarget = *apvts.getRawParameterValue("oscLevel");
    const int   waveformIdx   = static_cast<int>(*apvts.getRawParameterValue("waveform"));
    const float pulseWidth    = *apvts.getRawParameterValue("pulseWidth");

    ADSREnvelope::Parameters envParams;
    envParams.attack  = *apvts.getRawParameterValue("attack");
    envParams.decay   = *apvts.getRawParameterValue("decay");
    envParams.sustain = *apvts.getRawParameterValue("sustain");
    envParams.release = *apvts.getRawParameterValue("release");

    smoothedMaster_.setTargetValue(masterTarget);
    smoothedOscLevel_.setTargetValue(oscLevelTarget);

    const auto waveform = static_cast<Oscillator::Waveform>(
        juce::jlimit(0, static_cast<int>(Oscillator::Waveform::NumWaveforms) - 1, waveformIdx));

    // Обработка MIDI-событий по сэмплам
    juce::MidiBuffer::Iterator it(midiMessages);
    juce::MidiMessage msg;
    int samplePos = 0;
    int nextMidiEventPos = -1;
    bool hasMidi = it.getNextEvent(msg, nextMidiEventPos);

    for (int sample = 0; sample < numSamples; ++sample)
    {
        // Обрабатываем все MIDI-события на текущем сэмпле
        while (hasMidi && nextMidiEventPos == sample)
        {
            if (msg.isNoteOn())
            {
                // Ищем свободный голос или вытесняем самый старый
                int freeIdx = -1;
                for (int i = 0; i < kMaxVoices; ++i)
                {
                    if (!voices_[i].active) { freeIdx = i; break; }
                }
                if (freeIdx == -1) freeIdx = 0; // voice stealing

                voices_[freeIdx].oscillator.setWaveform(waveform);
                voices_[freeIdx].oscillator.setPulseWidth(pulseWidth);
                voices_[freeIdx].envelope.setParameters(envParams);
                voices_[freeIdx].noteOn(msg.getNoteNumber(), msg.getFloatVelocity());
            }
            else if (msg.isNoteOff())
            {
                int note = msg.getNoteNumber();
                for (auto& v : voices_)
                {
                    if (v.active && v.midiNote == note)
                        v.noteOff();
                }
            }
            hasMidi = it.getNextEvent(msg, nextMidiEventPos);
        }

        // Суммируем все голоса
        float mix = 0.0f;
        for (auto& v : voices_)
        {
            if (!v.active) continue;

            v.oscillator.setWaveform(waveform);
            v.oscillator.setPulseWidth(pulseWidth);
            v.envelope.setParameters(envParams);

            float env = v.envelope.processSample();
            float osc = v.oscillator.processSample();
            mix += osc * env;

            if (!v.envelope.isActive())
                v.active = false;
        }

        mix *= smoothedOscLevel_.getNextValue();
        mix *= smoothedMaster_.getNextValue();

        // Мягкий клиппер — защита от перегрузки
        if (mix > 1.0f)  mix = 1.0f;
        if (mix < -1.0f) mix = -1.0f;

        for (int ch = 0; ch < numChannels; ++ch)
            buffer.setSample(ch, sample, mix);
    }

    midiMessages.clear();
}

juce::AudioProcessorEditor* HunterIceAbyssProcessor::createEditor()
{
    return new HunterIceAbyssEditor(*this);
}

void HunterIceAbyssProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void HunterIceAbyssProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr)
        if (xmlState->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

} // namespace hunterice

// Точка входа VST3
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new hunterice::HunterIceAbyssProcessor();
}
