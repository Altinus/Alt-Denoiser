# Alt Denoiser

![Platform](https://img.shields.io/badge/Platform-Windows%20%7C%20macOS%20%7C%20Linux-brightgreen)
![Format](https://img.shields.io/badge/Format-VST3%20%7C%20VST2%20%7C%20AU%20%7C%20Standalone-blue)
![License](https://img.shields.io/badge/License-GPLv3-red)

**Alt Denoiser** is a real-time AI audio noise suppression plugin based on **[DeepFilterNet](https://github.com/Rikorose/DeepFilterNet)**. It wraps the open-source DeepFilterNet model into a ready-to-use audio plugin, supporting Windows, macOS, and Linux platforms in VST3, Standalone, AU, and LV2 formats.

**Alt Denoiser** 是一个基于 **[DeepFilterNet](https://github.com/Rikorose/DeepFilterNet)** 的实时 AI 语音降噪插件。它将开源的实时语音增强模型打包成一个开箱即用的音频插件，支持 Windows、macOS、Linux 平台，以及 VST3、VST2、Standalone、AU (macOS)、LV2 (Linux) 多种格式。

---

## Features / 特性

* **Sample Rate Independent**: Supports any host sample rate with automatic high-quality resampling.
    * **采样率无关**：支持任意宿主采样率，自动重采样。
* **Simple Interface**: Single knob for attenuation control + Input/Output RMS metering.
    * **简单界面**：一个旋钮调节最大衰减量 + 输入/输出 RMS 电平表。
* **Embedded Model**: DeepFilterNet3 models are bundled within the plugin; no external downloads required.
    * **模型嵌入**：DeepFilterNet3 模型已打包进插件，无需额外下载。

## Interface / 界面

<img width="685" height="472" alt="Alt Denoiser UI" src="https://github.com/user-attachments/assets/e71a463d-8c70-4efd-b598-79a90e1edc9f" />

## Download / 下载

visit the **[Releases](https://github.com/Altinus/Alt-Denoiser/releases)** page to download the latest version.
*Note: Currently, pre-compiled binaries are available for Windows only*

前往 **[Releases](https://github.com/Altinus/Alt-Denoiser/releases)** 页面下载最新版本。
*注意：目前仅提供 Windows 版本的预编译文件*

---

## Build Instructions / 构建指南

If you want to build from source, please follow these steps.
如果您想自己编译代码，请参考以下步骤。

### 0. Prerequisites / 环境准备

Ensure you have the following installed:
你需要确保安装了以下工具：
* **CMake** (3.15+)
* **Rust Toolchain**
* **C++ Compiler** (Visual Studio 2022 / Xcode / GCC)

### 1. Clone Repository / 获取代码


```bash
git clone --recursive https://github.com/Altinus/Alt-Denoiser.git
cd Alt-Denoiser
```

### 2. Build / 开始构建


```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Build (Rust compilation may take time on first run)
cmake --build build --config Release --parallel 4
```

### 3. Artifacts / 获取产物

After the build, the plugin files will be located at:
编译完成后，你可以在以下目录找到插件文件：

* **Windows / macOS / Linux**: 
  `build/AltDenoiser_artefacts/Release/`

---

## Installation / 安装路径

Copy the generated plugin files (`.vst3` / `.component`) to your system's plugin directory:
将生成的插件文件复制到你系统的对应目录中：

| Format | Windows | macOS | Linux |
| :--- | :--- | :--- | :--- |
| **VST3** | `C:\Program Files\Common Files\VST3` | `/Library/Audio/Plug-Ins/VST3` | `~/.vst3` |
| **VST2** | `C:\Program Files\Steinberg\VstPlugins` (or `C:\Program Files\VSTPlugIns`) | `/Library/Audio/Plug-Ins/VST` | `~/.vst` |
| **AU** | N/A | `/Library/Audio/Plug-Ins/Components` | N/A |
| **LV2** | N/A | N/A | `~/.lv2` |

---

## License / 开源协议

This project is licensed under the **GPLv3** License.
本项目遵循 **GPLv3** 协议。

* **[JUCE](https://github.com/juce-framework/JUCE)** (GPLv3)
* **[DeepFilterNet](https://github.com/Rikorose/DeepFilterNet)** (MIT/Apache-2.0)

## Credits / 致谢

* **[DeepFilterNet](https://github.com/Rikorose/DeepFilterNet)**: noise reduction AI models and Rust library.
* **[JUCE](https://github.com/juce-framework/JUCE)**: Cross-platform audio plugin framework.
* **[Resampler](https://github.com/niswegmann/Resampler)**: High-quality sample rate conversion library.  
* **gemini**: For patiently explaining why my code wouldn't compile xd😂😂
