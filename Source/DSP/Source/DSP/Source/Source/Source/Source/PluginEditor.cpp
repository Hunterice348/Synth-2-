#include "PluginEditor.h"
#include "PluginProcessor.h"

namespace hunterice
{

namespace Colours
{
    const juce::Colour background      = juce::Colour::fromRGB(18, 18, 22);
    const juce::Colour panel           = juce::Colour::fromRGB(32, 32, 38);
    const juce::Colour accent          = juce::Colour::fromRGB(180, 30, 40);   // dark red
    const juce::Colour text            = juce::Colour::fromRGB(230, 230, 235); // cold white
    const juce::Colour knobOutline     = juce::Colour::fromRGB(90, 90, 100);
    const juce::Colour knobNeedle      = juce::Colour::fromRGB(220, 220, 230);
}

HunterIceAbyssEditor::HunterIceAbyssEditor(HunterIceAbyssProcessor& p)
    : AudioProcessorEditor(&p), processorRef_(p)
{
    setSize(820, 520);
    setResizable(true, true);
    setResizeLimits(640, 400, 1600, 1000);

    auto setupSlider = [this](juce::Slider& s, const juce::String& label)
    {
        s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 18);
        s.setColour(juce::Slider::rotarySliderFillColourId, Colours::accent);
        s.setColour(juce::Slider::rotarySliderOutlineColourId, Colours::knobOutline);
        s.setColour(juce::Slider::thumbColourId, Colours::knobNeedle);
        s.setColour(juce::Slider::textBoxTextColourId, Colours::text);
        s.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        addAndMakeVisible(s);
    };

    setupSlider(masterSlider_, "MASTER");
    setupSlider(oscLevelSlider_, "OSC LEVEL");
    setupSlider(attackSlider_, "ATTACK");
    setupSlider(decaySlider_, "DECAY");
    setupSlider(sustainSlider_, "SUSTAIN");
    setupSlider(releaseSlider_, "RELEASE");
    setupSlider(pulseWidthSlider_, "PULSE WIDTH");

    waveformBox_.addItemList({"Sine", "Triangle", "Saw", "Square"}, 1);
    waveformBox_.setColour(juce::ComboBox::backgroundColourId, Colours::panel);
    waveformBox_.setColour(juce::ComboBox::textColourId, Colours::text);
    waveformBox_.setColour(juce::ComboBox::outlineColourId, Colours::knobOutline);
    addAndMakeVisible(waveformBox_);

    // Привязки
    masterAtt_      = std::make_unique<Attachment>(p.apvts, "master",     masterSlider_);
    oscLevelAtt_    = std::make_unique<Attachment>(p.apvts, "oscLevel",   oscLevelSlider_);
    attackAtt_      = std::make_unique<Attachment>(p.apvts, "attack",     attackSlider_);
    decayAtt_       = std::make_unique<Attachment>(p.apvts, "decay",      decaySlider_);
    sustainAtt_     = std::make_unique<Attachment>(p.apvts, "sustain",    sustainSlider_);
    releaseAtt_     = std::make_unique<Attachment>(p.apvts, "release",    releaseSlider_);
    pulseWidthAtt_  = std::make_unique<Attachment>(p.apvts, "pulseWidth", pulseWidthSlider_);
    waveformAtt_    = std::make_unique<ComboAttachment>(p.apvts, "waveform", waveformBox_);
}

void HunterIceAbyssEditor::paint(juce::Graphics& g)
{
    // Фон
    g.fillAll(Colours::background);

    // Верхняя панель — логотип
    juce::Rectangle<int> topBar(0, 0, getWidth(), 60);
    g.setGradientFill(juce::ColourGradient(
        juce::Colour::fromRGB(25, 25, 30), 0.0f, 0.0f,
        juce::Colour::fromRGB(15, 15, 18), 0.0f, 60.0f, false));
    g.fillRect(topBar);

    // Логотип HUNTERICE
    g.setColour(Colours::accent);
    g.setFont(juce::Font("Arial Black", 26.0f, juce::Font::bold));
    g.drawText("HUNTERICE", 20, 12, 220, 36, juce::Justification::left);

    // Название ABYSS
    g.setColour(Colours::text);
    g.setFont(juce::Font("Arial", 16.0f, juce::Font::plain));
    g.drawText("A B Y S S", 240, 20, 160, 24, juce::Justification::left);

    // Декоративная линия
    g.setColour(Colours::accent);
    g.fillRect(0, 58, getWidth(), 2);

    // Панели секций
    auto drawSection = [&](juce::Rectangle<int> r, const juce::String& title)
    {
        g.setColour(Colours::panel);
        g.fillRoundedRectangle(r.toFloat(), 6.0f);
        g.setColour(Colours::knobOutline);
        g.drawRoundedRectangle(r.toFloat(), 6.0f, 1.0f);
        g.setColour(Colours::accent);
        g.setFont(juce::Font(12.0f, juce::Font::bold));
        g.drawText(title, r.getX() + 10, r.getY() + 6, r.getWidth() - 20, 16,
                   juce::Justification::left);
    };

    // Секция OSCILLATOR
    int margin = 12;
    int topY = 70;
    int sectionH = 200;
    int oscW = (getWidth() - margin * 3) / 3;
    drawSection({margin, topY, oscW, sectionH}, "OSCILLATOR");
    drawSection({margin * 2 + oscW, topY, oscW, sectionH}, "ENVELOPE");
    drawSection({margin * 3 + oscW * 2, topY, oscW, sectionH}, "OUTPUT");
}

void HunterIceAbyssEditor::resized()
{
    int margin = 12;
    int topY = 100;
    int sectionH = 200;
    int oscW = (getWidth() - margin * 3) / 3;

    // Секция OSCILLATOR
    int x1 = margin;
    int knobSize = 80;
    int knobY = topY + 40;

    waveformBox_.setBounds(x1 + 20, knobY, oscW - 40, 26);
    pulseWidthSlider_.setBounds(x1 + (oscW - knobSize) / 2, knobY + 50, knobSize, knobSize + 20);

    // Секция ENVELOPE
    int x2 = margin * 2 + oscW;
    int envKnobSize = 70;
    int envY = topY + 35;
    attackSlider_.setBounds(x2 + 10,           envY, envKnobSize, envKnobSize + 20);
    decaySlider_.setBounds(x2 + 10 + envKnobSize + 5, envY, envKnobSize, envKnobSize + 20);
    sustainSlider_.setBounds(x2 + 10,          envY + envKnobSize + 30, envKnobSize, envKnobSize + 20);
    releaseSlider_.setBounds(x2 + 10 + envKnobSize + 5, envY + envKnobSize + 30, envKnobSize, envKnobSize + 20);

    // Секция OUTPUT
    int x3 = margin * 3 + oscW * 2;
    int outKnob = 90;
    masterSlider_.setBounds(x3 + (oscW - outKnob) / 2, topY + 40, outKnob, outKnob + 20);
    oscLevelSlider_.setBounds(x3 + (oscW - outKnob) / 2, topY + 40 + outKnob + 30, outKnob, outKnob + 20);
}

} // namespace hunterice
