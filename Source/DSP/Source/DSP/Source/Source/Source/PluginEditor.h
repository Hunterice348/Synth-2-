#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

namespace hunterice
{

class HunterIceAbyssProcessor;

class HunterIceAbyssEditor : public juce::AudioProcessorEditor
{
public:
    explicit HunterIceAbyssEditor(HunterIceAbyssProcessor&);
    ~HunterIceAbyssEditor() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    HunterIceAbyssProcessor& processorRef_;

    // Ручки (Knobs)
    juce::Slider masterSlider_;
    juce::Slider oscLevelSlider_;
    juce::Slider attackSlider_;
    juce::Slider decaySlider_;
    juce::Slider sustainSlider_;
    juce::Slider releaseSlider_;
    juce::Slider pulseWidthSlider_;
    juce::ComboBox waveformBox_;

    // Привязка к APVTS
    using Attachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    std::unique_ptr<Attachment> masterAtt_;
    std::unique_ptr<Attachment> oscLevelAtt_;
    std::unique_ptr<Attachment> attackAtt_;
    std::unique_ptr<Attachment> decayAtt_;
    std::unique_ptr<Attachment> sustainAtt_;
    std::unique_ptr<Attachment> releaseAtt_;
    std::unique_ptr<Attachment> pulseWidthAtt_;
    std::unique_ptr<ComboAttachment> waveformAtt_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HunterIceAbyssEditor)
};

} // namespace hunterice
