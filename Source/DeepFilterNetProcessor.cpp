#include "DeepFilterNetProcessor.h"
#include "BinaryData.h"
#include <fstream>
#include <iostream>
#include <string>
#include <juce_core/juce_core.h>

DeepFilterNetProcessor::DeepFilterNetProcessor(uint32_t sampleRate)
    : sampleRate(sampleRate) {
}

DeepFilterNetProcessor::~DeepFilterNetProcessor() {
    if (state != nullptr) {
        df_free(state);
    }
}

bool DeepFilterNetProcessor::initialize()
{
    const char* modelData = AltDenoiserBinaryData::DeepFilterNet3_onnx_tar_gz;
    const int modelSize   = AltDenoiserBinaryData::DeepFilterNet3_onnx_tar_gzSize;

    if (modelSize <= 0 || modelData == nullptr)
    {
        DBG("Embedded model data invalid or missing");
        return false;
    }

    auto tempDir = juce::File::getSpecialLocation(juce::File::tempDirectory);
    auto tempModel = tempDir.getChildFile("alt_denoiser_model.tar.gz");

    {
        juce::FileOutputStream stream(tempModel);
        if (!stream.openedOk()) 
        {
            DBG("Failed to open temp file for model");
            return false;
        }
        stream.write(modelData, modelSize);
        stream.flush();
    }

    state = df_create(tempModel.getFullPathName().toRawUTF8(), 100.0f, nullptr);
    return state != nullptr;
}

void DeepFilterNetProcessor::setAttenLim(float limitDB) {
    if (state != nullptr) {
        // 调用 df.h 中定义的 C 接口
        df_set_atten_lim(state, limitDB);
    }
}

void DeepFilterNetProcessor::processFrame(const float* input, float* output) {
    if (state) {
        df_process_frame(state, (float*)input, output);
    }
}