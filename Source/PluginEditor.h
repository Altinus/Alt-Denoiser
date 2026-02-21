#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"

class ModernLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
        float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
        juce::Slider&) override
    {

        auto radius = (float)juce::jmin(width / 2, height / 2) - 20.0f; 
        auto center = juce::Point<float>((float)x + (float)width * 0.5f, (float)y + (float)height * 0.5f);
        const float trackWidth = 14.0f;

        // track
        juce::Path backgroundArc;
        backgroundArc.addArc(center.x - radius, center.y - radius, radius * 2.0f, radius * 2.0f,
                             rotaryStartAngle, rotaryEndAngle, true);

        g.setColour(juce::Colour(0xff1a1a1a));
        g.strokePath(backgroundArc, juce::PathStrokeType(trackWidth, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // fill
        auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
        juce::Path valueArc;
        valueArc.addArc(center.x - radius, center.y - radius, radius * 2.0f, radius * 2.0f,
                        rotaryStartAngle, angle, true);

        g.setColour(juce::Colour(0xffff9900));
        g.strokePath(valueArc, juce::PathStrokeType(trackWidth, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    juce::Label* createSliderTextBox(juce::Slider& slider) override
    {
        auto* l = LookAndFeel_V4::createSliderTextBox(slider);
        l->setColour(juce::Label::outlineColourId, juce::Colours::transparentBlack);
        l->setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
        l->setColour(juce::Label::textColourId, juce::Colours::lightgrey);
        l->setFont(juce::FontOptions(14.0f));
        return l;
    }
};

class DbMeter : public juce::Component
{
public:
    DbMeter(bool isInputMode) : isInput(isInputMode) {}
    void update(float newRawLevel) 
    {
        const float attack = 1.0f;
        const float release = 0.8f;
        
        if (newRawLevel > smoothedLevel) smoothedLevel = newRawLevel; // Attack
        else smoothedLevel *= release;                                // Release
        
        if (smoothedLevel < 1e-9f) smoothedLevel = 1e-9f;

        float currentDb = juce::Decibels::gainToDecibels(smoothedLevel, -100.0f);
        
        if (currentDb > displayedDb) {
            displayedDb = currentDb; 
        } else {
            displayedDb -= 0.5f;
        }

        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        
        const float topTextHeight = 18.0f;
        const float bottomTextHeight = 18.0f;
        const float internalPadding = 14.0f; 

        auto meterArea = bounds.withTrimmedTop(topTextHeight)
                               .withTrimmedBottom(bottomTextHeight)
                               .reduced(internalPadding, 0);

        const float barWidth = 12.0f;
        juce::Rectangle<float> barRect;
        juce::Rectangle<float> tickArea;

        if (isInput) {
            barRect = meterArea.removeFromLeft(barWidth);
            tickArea = meterArea.withTrimmedLeft(6.0f);
        } else {
            barRect = meterArea.removeFromRight(barWidth);
            tickArea = meterArea.withTrimmedRight(6.0f);
        }

        g.setColour(juce::Colour(0xff181818));
        g.fillRoundedRectangle(barRect, 2.0f);

        float db = juce::Decibels::gainToDecibels(smoothedLevel, -100.0f);

        const float maxdB = 6.0f;
        const float mindB = -60.0f;

        auto mapDbToY = [&](float val) {
            return juce::jmap(val, mindB, maxdB, barRect.getBottom(), barRect.getY());
        };

        float y0dB      = mapDbToY(0.0f);
        float yMinus6dB = mapDbToY(-6.0f);
        float yMinus24dB= mapDbToY(-24.0f);
        float yBottom   = barRect.getBottom();
    
        float yCurrent  = juce::jlimit(barRect.getY(), yBottom, mapDbToY(db));

        g.saveState();
        juce::Path clipPath;
        clipPath.addRoundedRectangle(barRect, 2.0f);
        g.reduceClipRegion(clipPath);

        float greenTop = juce::jmax(yCurrent, yMinus24dB);
        if (greenTop < yBottom) {
            g.setColour(juce::Colours::green);
            g.fillRect(barRect.withTop(greenTop).withBottom(yBottom));
        }
        float gradTop = juce::jmax(yCurrent, yMinus6dB);
        if (gradTop < yMinus24dB) {
            juce::ColourGradient gradient(
                juce::Colours::yellow, 0, yMinus6dB,
                juce::Colours::green,  0, yMinus24dB, 
                false
            );
            g.setGradientFill(gradient);
            g.fillRect(barRect.withTop(gradTop).withBottom(yMinus24dB));
        }
        float yellowTop = juce::jmax(yCurrent, y0dB);
        if (yellowTop < yMinus6dB) {
            g.setColour(juce::Colours::yellow);
            g.fillRect(barRect.withTop(yellowTop).withBottom(yMinus6dB));
        }
        if (yCurrent < y0dB) {
            g.setColour(juce::Colours::red);
            g.fillRect(barRect.withTop(yCurrent).withBottom(y0dB));
        }

        g.restoreState();

        float barCenterX = barRect.getCentreX();
        // top:dB
        g.setColour(juce::Colours::white);
        g.setFont(juce::FontOptions(12.0f));
        juce::String peakStr = (displayedDb <= -90.0f) ? "-inf" : juce::String(displayedDb, 1);
        juce::Rectangle<float> topTextRect(0, 0, 60, topTextHeight);
        topTextRect.setCentre(barCenterX, topTextHeight / 2.0f);
        g.drawFittedText(peakStr, topTextRect.toNearestInt(), juce::Justification::centred, 1);

        // bottom:IN/OUT
        g.setFont(juce::FontOptions(10.0f));
        g.setColour(juce::Colours::grey);
        juce::Rectangle<float> botTextRect(0, bounds.getBottom() - bottomTextHeight, 60, bottomTextHeight);
        botTextRect.setCentre(barCenterX, bounds.getBottom() - bottomTextHeight / 2.0f);
        g.drawFittedText(isInput ? "IN" : "OUT", botTextRect.toNearestInt(), juce::Justification::centred, 1);

        // meter ticks
        g.setFont(juce::FontOptions(10.0f));
        const float ticks[] = { 0.0f, -6.0f, -12.0f, -24.0f, -48.0f };
        for (float t : ticks) {
            float y = mapDbToY(t);
            if (y >= barRect.getY() && y <= barRect.getBottom()) {
                g.setColour(juce::Colours::white.withAlpha(0.3f));
                g.fillRect(barRect.getX(), y, barRect.getWidth(), 1.0f);
                g.setColour(juce::Colours::grey);
                juce::Justification just = isInput ? juce::Justification::centredLeft : juce::Justification::centredRight;
                auto numRect = tickArea.withY(y - 5.0f).withHeight(10.0f);
                g.drawText(juce::String((int)t), numRect, just, false);
            }
        }
    }

private:
    float smoothedLevel = 0.0f;
    float displayedDb = -100.0f;
    bool isInput;
};

class AltDenoiserEditor : public juce::AudioProcessorEditor, public juce::Timer
{
public:
    AltDenoiserEditor(AltDenoiserProcessor&, juce::AudioProcessorValueTreeState&);
    ~AltDenoiserEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    AltDenoiserProcessor& audioProcessor;
    juce::AudioProcessorValueTreeState& apvts;
    
    ModernLookAndFeel modernLook;

    juce::Slider attenSlider;
    juce::Label attenLabel;
    juce::TextButton aboutButton { "i" };

    DbMeter inputMeter { true };  // true = IN mode
    DbMeter outputMeter { false }; // false = OUT mode

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attenAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AltDenoiserEditor)
};