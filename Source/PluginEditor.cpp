#include "PluginProcessor.h"
#include "PluginEditor.h"

AltDenoiserEditor::AltDenoiserEditor(AltDenoiserProcessor& p, juce::AudioProcessorValueTreeState& vts)
    : AudioProcessorEditor(&p), audioProcessor(p), apvts(vts)
{
    setLookAndFeel(&modernLook);

    attenSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    attenSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 25); 
    attenSlider.setTextValueSuffix(" dB");
    addAndMakeVisible(attenSlider);

    attenLabel.setText("Reduction", juce::dontSendNotification);
    attenLabel.setJustificationType(juce::Justification::centred);
    attenLabel.setFont(juce::Font(juce::FontOptions(13.0f).withStyle("bold")));
    attenLabel.setColour(juce::Label::textColourId, juce::Colours::grey);
    addAndMakeVisible(attenLabel);

    attenAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, "atten_lim", attenSlider
    );

    addAndMakeVisible(aboutButton);
    aboutButton.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    aboutButton.setColour(juce::TextButton::textColourOffId, juce::Colours::grey);
    aboutButton.onClick = [this] { juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::InfoIcon,
            "About",
            "Alt Denoiser v1.0\n"
            "By Altinus\n\n"
            "Credits:\n"
            "DeepFilterNet (Rikorose)\n"
            "Resampler (niswegmann)\n"
            "JUCE Framework",
            "ok" 
            ); };


    addAndMakeVisible(inputMeter);
    addAndMakeVisible(outputMeter);

    startTimerHz(60);
    setSize(460, 320); 
}

AltDenoiserEditor::~AltDenoiserEditor()
{
    setLookAndFeel(nullptr);
    stopTimer();
}

void AltDenoiserEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1e1e1e));
    g.setColour(juce::Colour(0xffdddddd)); 
    g.setFont(juce::Font(juce::FontOptions(25.0f).withStyle("bold")));
    g.drawFittedText("Alt Denoiser", getLocalBounds().removeFromTop(40), juce::Justification::centred, 1);
}

void AltDenoiserEditor::resized()
{
    auto area = getLocalBounds();
    // 1. meter
    inputMeter.setBounds(20, 50, 70, getHeight() - 80); 
    outputMeter.setBounds(getWidth() - 90, 50, 70, getHeight() - 80);

    // 2. knob and label
    const int knobComponentSize = 200;     
    auto knobBounds = juce::Rectangle<int>(
        area.getCentreX() - knobComponentSize / 2, 
        area.getCentreY() - knobComponentSize / 2 + 10,
        knobComponentSize, 
        knobComponentSize
    );
    attenSlider.setBounds(knobBounds);

    attenLabel.setBounds(
        knobBounds.getX(), 
        knobBounds.getY() + knobBounds.getHeight() + 5,
        knobBounds.getWidth(), 
        20
    );

    // 3.info
    aboutButton.setBounds(getWidth() - 30, 10, 20, 20);
}

void AltDenoiserEditor::timerCallback()
{
    float rawIn = audioProcessor.inputRmsLevel.load();
    float rawOut = audioProcessor.outputRmsLevel.load();

    inputMeter.update(rawIn);
    outputMeter.update(rawOut);
}