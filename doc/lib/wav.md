# CP 标准库：RIFF/WAVE PCM 音频容器、混音与重采样手册 (`cap:wav` / `cap.wav`)

## 一、 架构总览与设计准则

`cap:wav` 是 CP 语言官方内置的纯原生 RIFF/WAVE 内存字节流编解码与基础音频处理标准库，支持 `Pcm8`、`Pcm16`、`Pcm24`、`Pcm32` 与 `Float32` 全格式无损归一化读写、未知扩展块跳过（含奇数字节对齐填充）、多声道转单声道、线性重采样及峰值归一化。

全模块**零 C 依赖、零 `std.posix`、零 `@import("bun")`**，不直接操作磁盘 syscall 或声卡驱动，支持 `import ... from "cap:wav"` 与 `import cap from "cap"; cap.wav.*` 编译期自动惰性挂载。

### 模块文件结构

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/wav/wav.cp` | `// Source: cp/lib/wav/wav.cp (Pure CP RIFF/WAVE PCM Audio Container, Mixer & Resampler)` | `WavError`、`SampleFormat`、`WavAudio`、`decode`、`encode` 及混音/重采样/峰值处理原语 |
| `lib/wav/index.cp` | `// Source: cp/lib/wav/index.cp (Unified Exports for cap:wav & cap.wav)` | `cap:wav` 与 `cap.wav` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误集与采样格式

- **`WavError`**：
  - `InvalidRiffHeader`：缺失或损坏的 `RIFF`/`WAVE` 文件头或非法 `riff_size`
  - `InvalidWaveFormat`：缺失或畸形的 `fmt ` 块、`block_align` 不匹配或编码尺寸越界
  - `UnsupportedSampleFormat`：非 PCM (`tag=1`, 8/16/24/32-bit) 或 IEEE Float (`tag=3`, 32-bit) 格式
  - `MissingDataChunk`：合法 RIFF/WAVE 容器中缺失 `data` 块
  - `TruncatedData`：缓冲区截断、`riff_size`/块长度越界或 `data` 长度未按帧对齐
  - `InvalidChannelCount`：声道数为 0
  - `InvalidSampleRate`：采样率为 0
  - `OutOfMemory`：分配器内存不足

- **`SampleFormat` (`u8`)**：
  - `Pcm8 = 1`：无符号 8-bit PCM（偏移 128）
  - `Pcm16 = 2`：有符号 16-bit 小端 PCM
  - `Pcm24 = 3`：有符号 24-bit 小端补码 PCM（3 字节）
  - `Pcm32 = 4`：有符号 32-bit 小端 PCM
  - `Float32 = 5`：IEEE 754 32-bit 小端浮点

### 2. `WavAudio` 容器与编解码

- **`WavAudio`**：
  - 字段：`sample_rate: u32`、`channels: u16`、`format: SampleFormat`、`samples_f32: []f32`（统一归一化至 `[-1.0, 1.0]` 交错浮点样本缓冲）
  - `frameCount(): usize`：返回总音频帧数（`samples_f32.len / channels`）
  - `durationSec(): f64`：返回音频时长（秒）
  - `deinit(*_, allocator: std.mem.Allocator)`：释放 `samples_f32` 内存缓冲

- **`decode(allocator: std.mem.Allocator, wav_bytes: []const u8): WavError!WavAudio`**
  - 解析 `RIFF`/`WAVE` 魔数，跳过 `LIST`/`INFO`/`JUNK`/`fact` 等非关键扩展块（自动处理奇数字节 Chunk 的 2 字节偶对齐填充），支持 PCM 8/16/24/32-bit 与 IEEE Float32 无损归一化解码。
- **`encode(allocator: std.mem.Allocator, audio: WavAudio, target_format: SampleFormat): WavError![]u8`**
  - 将 `WavAudio` 编码为指定采样格式的标准 44 字节头 RIFF/WAVE 字节流，对越界浮点样本执行严格饱和钳位与 `NaN` 归零。

### 3. 音频数字处理原语

- **`toMono(allocator: std.mem.Allocator, audio: WavAudio): WavError!WavAudio`**：多声道交错采样均值混音为单声道。
- **`resampleLinear(allocator: std.mem.Allocator, audio: WavAudio, target_sample_rate: u32): WavError!WavAudio`**：线性插值重采样（如 `48000 Hz -> 16000 Hz` ASR 预处理）。
- **`peakAmplitude(audio: WavAudio): f32`**：检测最大绝对值样本峰值。
- **`normalizePeak(audio: *WavAudio, target_peak: f32)`**：按目标峰值原位线性缩放增益。

---

## 三、 使用示例

```typescript
const std = @import("std");
import cap from "cap";
import { WavAudio, SampleFormat, decode, encode, toMono, resampleLinear, peakAmplitude, normalizePeak } from "cap:wav";

pub fn preprocessForAsr(alloc: std.mem.Allocator, wav_bytes: []const u8): ![]u8 {
    let audio = decode(alloc, wav_bytes)?;
    defer audio.deinit(alloc);

    let mono = toMono(alloc, audio)?;
    defer mono.deinit(alloc);

    let resampled = resampleLinear(alloc, mono, 16000)?;
    defer resampled.deinit(alloc);

    if (peakAmplitude(resampled) > 0.0) {
        normalizePeak(&resampled, 0.95);
    }

    return cap.wav.encode(alloc, resampled, SampleFormat.Pcm16);
}
```
