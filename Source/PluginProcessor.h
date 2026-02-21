#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "DeepFilterNetProcessor.h"
#include "Resampler.hpp"
#include <vector>
#include <memory>

class SimpleFifo {
public:
    void setSize(int size) { 
        buffer.resize(size, 0.0f); 
        writePos = 0; readPos = 0; samplesInFifo = 0; 
    }
    
    void push(const float* data, int numSamples) {
        for (int i = 0; i < numSamples; ++i) {
            buffer[writePos] = data[i];
            writePos = (writePos + 1) % buffer.size();
        }
        samplesInFifo += numSamples;
        if (samplesInFifo > buffer.size()) samplesInFifo = buffer.size();
    }
    
    void peek(float* dest, int numSamples) {
        int tempRead = readPos;
        for (int i = 0; i < numSamples; ++i) {
            dest[i] = buffer[tempRead];
            tempRead = (tempRead + 1) % buffer.size();
        }
    }
    
    void discard(int numSamples) {
        readPos = (readPos + numSamples) % buffer.size();
        samplesInFifo -= numSamples;
    }
    
    int getAvailable() const { return samplesInFifo; }

private:
    std::vector<float> buffer;
    int writePos = 0;
    int readPos = 0;
    int samplesInFifo = 0;
};

class AltDenoiserProcessor : public juce::AudioProcessor {
public:
    AltDenoiserProcessor();
    ~AltDenoiserProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Alt Denoiser"; }
    
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return "Default"; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override
    {
        auto state = apvts.copyState();
        std::unique_ptr<juce::XmlElement> xml(state.createXml());
        copyXmlToBinary(*xml, destData);
    }

    void setStateInformation(const void* data, int sizeInBytes) override
    {
        std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

        if (xmlState.get() != nullptr)
        {
            if (xmlState->hasTagName(apvts.state.getType()))
            {
                apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
            }
        }
    }

    std::atomic<float> inputRmsLevel { 0.0f };
    std::atomic<float> outputRmsLevel { 0.0f };
    juce::AudioProcessorValueTreeState apvts;

private:
    std::unique_ptr<DeepFilterNetProcessor> dfProcessor;

    SimpleFifo inputFifo;
    SimpleFifo outputFifo;
    std::vector<float> tempInputFrame;
    std::vector<float> tempOutputFrame;
    bool modelLoaded = false;
    float lastAttenLim = -1.0f;

    std::unique_ptr<Resampler<1, 1>> resamplerHandler;
    std::vector<float> resampleInBuffer;
    std::vector<float> resampleOutBuffer;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AltDenoiserProcessor)
};