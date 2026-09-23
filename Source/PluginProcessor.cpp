#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <juce_core/juce_core.h>

AltDenoiserProcessor::AltDenoiserProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{
    dfProcessor = std::make_unique<DeepFilterNetProcessor>(48000);
}

AltDenoiserProcessor::~AltDenoiserProcessor() {
}

juce::AudioProcessorValueTreeState::ParameterLayout AltDenoiserProcessor::createParameterLayout() 
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    auto range = juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f);
    range.setSkewForCentre(20.0f); 
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "atten_lim", 
        "Attenuation Limit", 
        range, 
        100.0f 
    ));

    return layout;
}

void AltDenoiserProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    inputFifo.setSize(48000);
    outputFifo.setSize(48000);
    outputFifo.pushSilence(1920); // Pre-fill with latency to prevent buffer underflows / glitching
    tempInputFrame.resize(480, 0.0f);
    tempOutputFrame.resize(480, 0.0f);

    int latencyInHost = juce::roundToInt(1920.0 * (sampleRate / 48000.0));
    setLatencySamples(latencyInHost);

    if (dfProcessor->initialize()) {
        modelLoaded = true;
    } else {
        modelLoaded = false;
    }

    // init resampler
    resamplerHandler = std::make_unique<Resampler<1, 1>>(sampleRate, 48000.0);
    double maxRatio = 48000.0 / sampleRate;
    int maxResampledSize = (int)(samplesPerBlock * maxRatio) + 128; // +128 for safety margin
    resampleInBuffer.resize(maxResampledSize);
    resampleOutBuffer.resize(maxResampledSize);
}

void AltDenoiserProcessor::releaseResources() {
}

void AltDenoiserProcessor::reset() {
    inputFifo.clear();
    outputFifo.clear();
    outputFifo.pushSilence(1920);
}

bool AltDenoiserProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    const auto& mainIn = layouts.getMainInputChannelSet();
    const auto& mainOut = layouts.getMainOutputChannelSet();

    // Support mono or stereo configurations
    if (mainIn != juce::AudioChannelSet::mono() && mainIn != juce::AudioChannelSet::stereo())
        return false;
    if (mainOut != juce::AudioChannelSet::mono() && mainOut != juce::AudioChannelSet::stereo())
        return false;

    return true;
}

void AltDenoiserProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    const int hostNumSamples    = buffer.getNumSamples();

    // safety check
    if (!modelLoaded || dfProcessor == nullptr || !dfProcessor->isReady() || hostNumSamples == 0) {
        buffer.clear(); 
        return; 
    }
 
    // input rms
    const float smoothAlpha = 0.5f; // smoothing factor for RMS
    float currentInRMS = 0.0f;
    if (totalNumInputChannels > 0)
        currentInRMS = buffer.getRMSLevel(0, 0, hostNumSamples);
    if (totalNumInputChannels > 1)
        currentInRMS = juce::jmax(currentInRMS, buffer.getRMSLevel(1, 0, hostNumSamples));
    float oldIn = inputRmsLevel.load();
    inputRmsLevel.store(oldIn * (1.0f - smoothAlpha) + currentInRMS * smoothAlpha);

    // clear unused output channels
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, hostNumSamples);

    // update attenuation limit parameter
    float newAttenLim = *apvts.getRawParameterValue("atten_lim");
    if (std::abs(newAttenLim - lastAttenLim) > 0.01f) {
        dfProcessor->setAttenLim(newAttenLim);
        lastAttenLim = newAttenLim;
    }

    // if stereo input, downmix to channel 0 for mono denoiser processing
    if (totalNumInputChannels > 1) {
        auto* ch0 = buffer.getWritePointer(0);
        const auto* ch1 = buffer.getReadPointer(1);
        for (int i = 0; i < hostNumSamples; ++i) {
            ch0[i] = 0.5f * (ch0[i] + ch1[i]);
        }
    }

    // resample & process
    float* sourceInputPtrs[] = { buffer.getWritePointer(0) }; 
    float* sourceOutputPtrs[] = { buffer.getWritePointer(0) }; 
    float* targetInputPtrs[] = { resampleInBuffer.data() };   
    float* targetOutputPtrs[] = { resampleOutBuffer.data() };

    resamplerHandler->process(
        sourceInputPtrs,
        sourceOutputPtrs,
        targetInputPtrs,
        targetOutputPtrs,
        hostNumSamples,
        // lambda callback
        [&](float* const* input_buffers, float* const* output_buffers, int sample_count_48k) {
            auto* readPtr = input_buffers[0];   // 48kHz input
            auto* writePtr = output_buffers[0]; // 48kHz output
            inputFifo.push(readPtr, sample_count_48k);

            // predict
            while (inputFifo.getAvailable() >= 480) {
                inputFifo.peek(tempInputFrame.data(), 480);
                inputFifo.discard(480);
                dfProcessor->processFrame(tempInputFrame.data(), tempOutputFrame.data());
                outputFifo.push(tempOutputFrame.data(), 480);
            }
            // outputfifo ---> writePtr
            int samplesAvailable = outputFifo.getAvailable();
            int samplesToRead = juce::jmin(samplesAvailable, sample_count_48k);
            if (samplesToRead > 0) {
                outputFifo.peek(writePtr, samplesToRead);
                outputFifo.discard(samplesToRead);
            }
            if (samplesToRead < sample_count_48k) {
                juce::FloatVectorOperations::clear(writePtr + samplesToRead, sample_count_48k - samplesToRead);
            }
        }
    );

    // replicate processed mono signal to other output channels (e.g. channel 1)
    for (int ch = 1; ch < totalNumOutputChannels; ++ch) {
        buffer.copyFrom(ch, 0, buffer, 0, 0, hostNumSamples);
    }

    // output RMS
    float currentOutRMS = 0.0f;
    if (totalNumOutputChannels > 0)
        currentOutRMS = buffer.getRMSLevel(0, 0, hostNumSamples);        
    float oldOut = outputRmsLevel.load();
    outputRmsLevel.store(oldOut * (1.0f - smoothAlpha) + currentOutRMS * smoothAlpha);
}

juce::AudioProcessorEditor* AltDenoiserProcessor::createEditor() {return new AltDenoiserEditor(*this, apvts);}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new AltDenoiserProcessor();}