# CP 标准库：音频编解码与语音 AI DSP/VAD 引擎手册 (`cap:audio` / `cap.audio`)

## 一、 架构总览与设计准则

`cap:audio` 是 CP 语言官方内置的纯原生音频编解码与实时语音 AI 信号处理标准库，专为 **ASR / Whisper 16kHz PCM 预处理**、**TTS RIFF/WAVE 音频封装** 与 **能量语音活动检测（VAD）** 打造。

全模块**零 C 依赖、零 `std.posix`、零 `@import("bun")`**，支持 `import ... from "cap:audio"` 与 `import cap from "cap"; cap.audio.*` 编译期自动惰性挂载。

### 模块文件结构

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/audio/wav.cp` | `// Source: cp/lib/audio/wav.cp (RIFF/WAVE PCM16 & Float32 Audio Codec)` | `AudioError`、`WavAudio`、`WavFloatAudio` 与 `WavCodec` 编解码器 |
| `lib/audio/dsp.cp` | `// Source: cp/lib/audio/dsp.cp (Voice AI PCM Resampler, RMS/dB Meter & Energy VAD)` | `AudioDsp` 转换/重采样/电平分析与 `EnergyVad` 语音区间切分 |
| `lib/audio/index.cp` | `// Source: cp/lib/audio/index.cp (Unified Exports for cap:audio)` | `cap:audio` 与 `cap.audio` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. RIFF/WAVE 编解码器（`wav.cp`）

- **`AudioError`**：
  - `InvalidHeader`：缺失或损坏的 `RIFF`/`WAVE`/`fmt ` 块头
  - `UnsupportedFormat`：非目标编码格式（如将 Float32/24-bit WAV 传入 `decodePcm16`）
  - `TruncatedData`：缓冲区截断、RIFF 长度不匹配或缺失 `data` 块
  - `InvalidParameter`：零采样率、零声道数或样本数未按声道对齐
  - `OutOfMemory`：分配器内存不足
- **`WavAudio` / `WavFloatAudio`**：
  - 字段：`sample_rate: u32`、`channels: u16`、`bits_per_sample: u16`、`samples: []i16`（或 `[]f32`）
  - 方法：`durationMs(): u64`、`deinit(*_)`
- **`WavCodec`**：
  - `encodePcm16(alloc, samples: []const i16, sample_rate: u32, channels: u16): ([]u8)!`
  - `decodePcm16(alloc, bytes: []const u8): WavAudio!`（严格校验 `RIFF`/`WAVE`/`fmt `/`data` 块，自动跳过 `LIST`/`JUNK`/`fact` 等扩展块并按偶数字节对齐步进）
  - `encodeF32(alloc, samples: []const f32, sample_rate: u32, channels: u16): ([]u8)!`
  - `decodeF32(alloc, bytes: []const u8): WavFloatAudio!`

### 2. Voice AI DSP 与能量 VAD（`dsp.cp`）

- **`AudioDsp`**：
  - `pcm16ToF32(alloc, pcm: []const i16): ([]f32)!`：将有符号 16-bit PCM 无损映射至 `[-1.0, 1.0]`
  - `f32ToPcm16(alloc, floats: []const f32): ([]i16)!`：`[-1.0, 1.0]` 饱和限幅并舍入至 `i16`（`NaN` 安全归零）
  - `stereoToMono(alloc, stereo: []const i16): ([]i16)!`：交错双声道无溢出均值混音为单声道
  - `resampleLinear(alloc, samples: []const f32, from_rate: u32, to_rate: u32): ([]f32)!`：线性插值重采样（如 `48000 -> 16000` 适配 Whisper/ASR）
  - `computeRms(samples: []const f32): f64`：计算均方根能量（RMS）
  - `computeRmsDb(samples: []const f32): f64`：计算满刻度分贝（dBFS，静音兜底 `-120.0`）
  - `computePeak(samples: []const f32): f32`：计算绝对值峰值幅度
  - `zeroCrossingRate(samples: []const f32): f64`：计算过零率（ZCR）
  - `generateSineWave(alloc, freq_hz: f64, sample_rate: u32, num_samples: usize, amplitude: f32): ([]f32)!`：合成测试正弦波
- **`SpeechSegment`**：
  - 字段：`start_sample: usize`、`end_sample: usize`
  - 方法：`lenSamples(): usize`
- **`EnergyVad`**：
  - 配置字段：`frame_samples: usize`（默认 `480`，即 16kHz 下 30ms）、`rms_threshold: f64`（默认 `0.02`）、`hangover_frames: usize`（默认 `5`）
  - 构造：`EnergyVad.init(frame_samples, rms_threshold, hangover_frames)`
  - 检测：`detectSegments(alloc, samples: []const f32): ([]SpeechSegment)!`

---

## 三、 使用示例

```typescript
const std = @import("std");
import cap from "cap";
import { WavCodec, AudioDsp, EnergyVad } from "cap:audio";

pub fn preprocessForAsr(alloc: std.mem.Allocator, wav_bytes: []const u8): ! {
    let wav = WavCodec.decodePcm16(alloc, wav_bytes)?;
    defer wav.deinit();

    const mono_pcm = if (wav.channels == 2) AudioDsp.stereoToMono(alloc, wav.samples)? else wav.samples;
    defer if (wav.channels == 2) alloc.free(mono_pcm);

    const f32_src = AudioDsp.pcm16ToF32(alloc, mono_pcm)?;
    defer alloc.free(f32_src);

    const pcm_16k = AudioDsp.resampleLinear(alloc, f32_src, wav.sample_rate, 16000)?;
    defer alloc.free(pcm_16k);

    const vad = EnergyVad.init(480, 0.02, 5);
    const segments = vad.detectSegments(alloc, pcm_16k)?;
    defer alloc.free(segments);

    // 亦可直接通过 cap.audio 动态挂载调用
    const db = cap.audio.AudioDsp.computeRmsDb(pcm_16k);
    * = db;
}
```
