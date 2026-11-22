# CP 标准库：音视频、容器、字幕与音频元数据套件手册 (`cap:media`)

> 来源：Pure-CP 零 C 依赖音视频与多媒体标准库收敛（23 个子模块统一聚合） | 创建日期：2026-11-22

## 一、 架构总览

`cap:media` 是 CP 语言官方内置的纯原生、零外部 C 依赖音视频与多媒体标准库，涵盖 **音频编解码与 Voice AI DSP/VAD、主流音视频二进制容器解析（MP4/MKV/WebM/FLV/Ogg/EBML/RIFF）、音色库（SoundFont SF2 / MMA DLS）、音轨模块（IT / S3M / XM）、芯片音乐（NSF / NSFe / SPC700 / VGM）、音频元数据（ID3v1/v2）以及字幕格式（SRT/WebVTT）**。

---

## 二、 模块文件拓扑与核心 API 索引

### 1. 音频编解码、音乐与 Voice AI DSP（7 个子模块）

| 子模块文件 | 领域分类 | 核心导出符号 |
| :--- | :--- | :--- |
| `lib/media/audioWav.cp` | RIFF/WAVE PCM16 & Float32 编解码 | `WavCodec`, `WavAudio`, `WavFloatAudio`, `AudioError` |
| `lib/media/audioDsp.cp` | Voice AI 重采样、RMS/dB 计量与能量 VAD | `AudioDsp`, `EnergyVad`, `SpeechSegment` |
| `lib/media/flac.cp` | FLAC 无损音频元数据块解析 | `isFlac`, `FlacBlockIter`, `parseStreamInfoPayload`, `parseFlacStreamInfo`, `FlacStreamInfo`, `FlacBlockType` |
| `lib/media/ogg.cp` | RFC 3533 Ogg 容器分页解析与 CRC32 | `isOgg`, `OggPageIter`, `oggCrc32`, `OggPage` |
| `lib/media/opus.cp` | RFC 7845/6716 Opus Head/Tags/TOC 解析 | `isOpusHead`, `isOpusTags`, `parseOpusHead`, `parseOpusTags`, `parseOpusToc`, `OpusCommentIter` |
| `lib/media/aac.cp` | AAC ADTS 帧与 AudioSpecificConfig 解析 | `isAdts`, `AdtsIter`, `parseAudioSpecificConfig`, `summarizeAdts`, `aacSampleRate`, `aacChannelCount` |
| `lib/media/mpeg.cp` | MPEG-1/2/2.5 (MP3) 帧与 Xing/VBRI 解析 | `isMpegAudio`, `MpegFrameIter`, `parseMpegFrameHeader`, `parseXingHeader`, `summarizeMpeg`, `skipId3v2` |
| `lib/media/midi.cp` | 标准 MIDI 文件 (SMF 0/1/2) 编解码 | `parse`, `encode`, `MidiFile`, `MidiTrack`, `MidiEvent`, `MidiEventKind`, `MidiFormat`, `MidiError` |

### 2. 视频与通用二进制媒体容器（5 个子模块）

| 子模块文件 | 领域分类 | 核心导出符号 |
| :--- | :--- | :--- |
| `lib/media/mp4.cp` | ISO BMFF / MP4 Box 容器解析 | `isMp4`, `BoxCursor`, `findBox`, `findBoxPath`, `parseFtyp`, `parseMvhd`, `parseMdhd`, `parseTkhd`, `fourccEquals` |
| `lib/media/mkv.cp` | Matroska / WebM EBML 容器解析 | `isMkv`, `isWebm`, `parseMkvHeader`, `MkvTrackIter`, `MkvHeader`, `MkvInfo`, `MkvTrack`, `MkvTrackType` |
| `lib/media/flv.cp` | Adobe FLV 容器与音视频 Tag 解析 | `isFlv`, `parseFlvHeader`, `FlvTagIter`, `parseFlvAudioHeader`, `parseFlvVideoHeader`, `FlvHeader`, `FlvTag` |
| `lib/media/ebml.cp` | RFC 8794 EBML VINT 与元素迭代器 | `isEbmlHeader`, `EbmlElementIter`, `decodeEbmlId`, `decodeEbmlSize`, `decodeEbmlUint`, `EbmlElement` |
| `lib/media/riff.cp` | RIFF / RIFX 容器分块扫描器 | `parseRiffHeader`, `parseListType`, `RiffChunkIter`, `findRiffChunk`, `RiffHeader`, `RiffChunk` |

### 3. 字幕与音频标签元数据（3 个子模块）

| 子模块文件 | 领域分类 | 核心导出符号 |
| :--- | :--- | :--- |
| `lib/media/srt.cp` | SubRip (`.srt`) 字幕解析、格式化与时间偏移 | `SrtCueIter`, `parseSrtTimestampMs`, `formatSrtTimestampMs`, `formatSrtCue`, `shiftSrtCue`, `SrtCue` |
| `lib/media/vtt.cp` | W3C WebVTT (`.vtt`) 字幕解析与格式化 | `isWebVttHeader`, `VttCueIter`, `parseVttTimestampMs`, `formatVttTimestampMs`, `formatVttCue`, `VttCue` |
| `lib/media/id3.cp` | ID3v1 / ID3v2 音频元数据标签解析 | `parseId3v2Header`, `Id3v2FrameIter`, `parseId3v1Tail`, `decodeSyncsafeU32`, `Id3v2Header`, `Id3v1Tag` |

### 4. 音色库、音轨模块与芯片音乐（8 个子模块）

| 子模块文件 | 领域分类 | 核心导出符号 |
| :--- | :--- | :--- |
| `lib/media/dls.cp` | MMA Downloadable Sounds (`.dls`) RIFF 音色库解析 | `isDls`, `parseDls`, `DlsInstIter`, `DlsWaveIter`, `DlsHeader`, `DlsInfo`, `DlsInstrument`, `DlsWave` |
| `lib/media/sf2.cp` | SoundFont 2.01/2.04 (`.sf2`) RIFF `sfbk` 解析 | `isSf2`, `parseSf2`, `Sf2PresetIter`, `Sf2InstIter`, `Sf2SampleIter`, `Sf2Sections`, `Sf2Preset`, `Sf2Sample` |
| `lib/media/it.cp` | Impulse Tracker 2.14 (`.it`) 音轨模块解析 | `isIt`, `parseItHeader`, `parseItSample`, `parseItInstrument`, `parseItPattern`, `ItHeader`, `ItSample`, `ItInstrument` |
| `lib/media/s3m.cp` | Scream Tracker 3 (`.s3m`) 音轨模块解析 | `isS3m`, `parseS3mHeader`, `parseS3mInstrument`, `parseS3mPattern`, `S3mHeader`, `S3mInstrument`, `S3mPattern` |
| `lib/media/xm.cp` | FastTracker II Extended Module (`.xm` v1.04) 解析 | `isXm`, `parseXmHeader`, `XmPatternIter`, `XmInstrumentIter`, `XmHeader`, `XmPattern`, `XmInstrument` |
| `lib/media/nsf.cp` | NES Sound Format (`.nsf` / `.nsfe`) 芯片音乐解析 | `detectNsfFormat`, `isNsf`, `isNsfe`, `parseNsfHeader`, `NsfeChunkIter`, `findNsfeChunk`, `parseNsfeInfo`, `NsfHeader` |
| `lib/media/spc.cp` | SNES SPC700 (`.spc`) 音频快照与 ID666/xid6 解析 | `isSpc`, `parseSpcHeader`, `SpcXid6Iter`, `SpcHeader`, `SpcRegs`, `SpcXid6Item`, `SpcId666Format` |
| `lib/media/vgm.cp` | VGM 1.00-1.71 芯片音乐与 GD3 标签解析 | `isVgm`, `parseVgmHeader`, `parseVgmGd3`, `decodeUtf16LeToUtf8`, `scanVgmCommands`, `VgmHeader`, `VgmGd3`, `VgmCmdStats` |

> **重叠对照参考归档说明**：`lib/media/wav_.cp` 为与 `lib/media/audioWav.cp` 功能重叠的对照参考副本（文件首行带 `// Reference Only:` 声明），不在 `lib/media/index.cp` 冲突导出。

---

## 三、 使用示例

```typescript
import {
    isMp4,
    isMkv,
    parseSrtTimestampMs,
    isWebVttHeader,
    decodeSyncsafeU32,
} from "cap:media";

pub fn demoMedia(buf: []const u8) {
    const mp4_ok = isMp4(buf);
    const mkv_ok = isMkv(buf);
    const ms = parseSrtTimestampMs("00:01:23,456");
    const vtt_ok = isWebVttHeader("WEBVTT\n\n");
    const sz = decodeSyncsafeU32([0x00, 0x00, 0x02, 0x01]);
    * = mp4_ok;
    * = mkv_ok;
    * = ms;
    * = vtt_ok;
    * = sz;
}
```

---

## 四、 实时语音 AI 前端：SIMD 音频预处理与自适应 VAD 管线（`dspPipeline.cp`）

面向实时语音 Agent、ASR/Whisper 前端与低延迟流式语音检测的 100% 纯 CP 零堆分配 DSP 管线：

- **8-lane SIMD 帧能量与过零率分析（`AudioFrameStats` / `analyzeAudioFrameF32`）**：利用 `@dot` 硬件内积加速计算 RMS 能量、dBFS、绝对峰值与过零率（ZCR）。
- **自适应底噪与迟滞状态机 VAD（`VadConfig` / `VadState` / `VadDecision` / `AdaptiveEnergyVad`）**：融合能量阈值、自适应 EMA 底噪跟踪、ZCR 频段门控以及起音（`min_speech_frames`）/ 尾音（`hangover_frames`）迟滞防抖，精确输出 `Silence | SpeechStart | Speech | SpeechEnd`（与 `audioDsp.cp` 的离线批量 `EnergyVad` 互补无冲突）。
- **零堆音频前端算子**：`applyPreEmphasisF32`（`@fma` 一阶预加重）、`applyHannWindowF32`（对称 Hann 窗）、`normalizePeakF32`（防削波峰值归一化）与 `resampleLinearF32`（任意采样率线性重采样，如 48kHz -> 16kHz）。

### QuickStart 示例（48kHz -> 16kHz 预处理 + 流式 VAD）

```cp
import {
    VadConfig,
    AdaptiveEnergyVad,
    analyzeAudioFrameF32,
    applyPreEmphasisF32,
    applyHannWindowF32,
    normalizePeakF32,
    resampleLinearF32,
} from "cap:media";

pub fn demoVoiceAiFrontend(pcm_48k: []const f32) {
    let pcm_16k_buf: [320]f32 = [0.0; 320];
    let pcm_16k: []f32 = pcm_16k_buf[0..];
    const n16 = resampleLinearF32(pcm_48k, 48000, pcm_16k, 16000) ?? return;

    let frame: []f32 = pcm_16k[0..n16];
    * = normalizePeakF32(frame, 0.95);
    let vad = AdaptiveEnergyVad.init(VadConfig{
        energy_threshold: 0.02,
        min_speech_frames: 2,
        hangover_frames: 3,
    }) ?? return;

    const decision = vad.processFrame(frame);
    let pre_buf: [320]f32 = [0.0; 320];
    let pre: []f32 = pre_buf[0..];
    if (written := applyPreEmphasisF32(frame, pre, 0.97)) {
        let win_slice: []f32 = pre[0..written];
        * = applyHannWindowF32(win_slice);
    }
    * = analyzeAudioFrameF32(frame);
    * = decision;
}
```

### 无上限外部切片驱动 Radix-2 FFT/IFFT、RBJ Biquad IIR/SOS、加窗 Sinc 重采样与 K-Weighted 响度引擎 (`fftFilter.cp`)

面向专业音频 DSP、频谱分析、实时均衡滤波、高质量重采样与广播级响度计量的 100% 纯 CP 零动态堆分配引擎。全部算子直接作用于调用方传入的任意长度外部切片（支持任意 2 的幂 FFT `64..65536+` 与任意时长 PCM 缓冲）：

- **错误集**：`DspKernelError`（`NotPowerOfTwo | LengthMismatch | InvalidSampleRate | InvalidCutoff | InvalidQ | InvalidRatio | OutputTooSmall`）
- **窗函数族**：
  - `DspWindowKind`（`rectangular | hann | hamming | blackman_harris`）
  - `windowSampleAt(kind, index, len)` / `windowCoherentGain(kind)` / `applyDspWindowF64(*buf, kind)` / `applyDspWindowF32(*buf, kind)`
- **Radix-2 Cooley-Tukey 迭代 FFT / IFFT 与频谱分析**：
  - `fftRadix2InPlace(*re, *im): void ! DspKernelError`（位反转置换 + 周期性旋转因子重锚，零递归零堆分配）
  - `ifftRadix2InPlace(*re, *im): void ! DspKernelError`（共轭对称 + `1/N` 归一化，往返精度 `< 1e-12`）
  - `computeMagnitudeSpectrum(re, im, *out_mag)` / `computePowerDbSpectrum(re, im, floor_db, *out_db)`（兼容 `N/2 + 1` 单边与 `N` 双边缓冲）
  - `findPeakFrequencyHz(mag, sample_rate_hz): f64 ! DspKernelError`（三点抛物线亚频点插值精确测频）
  - `computeStftFrameMagnitude(frame_f32, window, *work_re, *work_im, *out_mag)`（单帧 STFT 加窗 + FFT + 幅度谱）
- **RBJ Audio EQ Cookbook Biquad IIR 滤波器与 SOS 级联**：
  - `BiquadFilterKind`（`low_pass | high_pass | band_pass | notch | peaking_eq | low_shelf | high_shelf | all_pass`）
  - `BiquadCoeffs`（`b0, b1, b2, a1, a2`；提供 `identity()`、`magnitudeResponseAt(freq_hz, sample_rate_hz)` 与 `magnitudeDbAt(...)`）
  - `designBiquad(kind, sample_rate_hz, freq_hz, q, gain_db): BiquadCoeffs ! DspKernelError`
  - `BiquadFilter`（Direct Form II Transposed 稳定结构：`init`, `reset`, `setCoeffs`, `processSample`, `processSliceInPlaceF32`, `processSliceF32`）
  - `BiquadCascade`（最多 `BIQUAD_CASCADE_MAX_STAGES = 8` 阶串联 SOS：`init`, `addStage`, `stageCount`, `reset`, `processSample`, `processSliceInPlaceF32`, `processSliceF32`, `magnitudeResponseAt`, `designButterworthLowPass`, `designKWeighting`）
- **加窗 Sinc 抗混叠重采样器**：
  - `resampleSincF32(src, src_rate_hz, *dst, dst_rate_hz, half_taps): usize ! DspKernelError`（Hann 加窗 sinc 核 + 下采样自动抗混叠截止缩放 + 直流增益归一化）
- **专业响度与动态计量**：
  - `LoudnessMetrics`（`rms_dbfs`, `peak_dbfs`, `crest_factor_db`, `k_weighted_lufs`, `dc_offset`, `clipped_samples`）
  - `measureAudioLoudnessF32(samples, sample_rate_hz): LoudnessMetrics ! DspKernelError`（ITU-R BS.1770 两级 K-Weighting 预滤波 + `-70 LUFS` 400ms 块绝对门控）

```cp
import {
    DspWindowKind,
    BiquadFilterKind,
    designBiquad,
    BiquadFilter,
    computeStftFrameMagnitude,
    findPeakFrequencyHz,
    resampleSincF32,
    measureAudioLoudnessF32,
} from "cap:media";

pub fn demoDspMasteringPipeline(pcm_48k: []const f32) {
    // 1. 高质量抗混叠重采样 48kHz -> 16kHz
    let pcm_16k_buf: [1024]f32 = [0.0; 1024];
    let pcm_16k_s: []f32 = pcm_16k_buf[0..];
    const n16 = resampleSincF32(pcm_48k, 48000, *pcm_16k_s, 16000, 12) catch return;

    // 2. 施加 80Hz 高通滤除低频直流/风噪
    const hp_c = designBiquad(BiquadFilterKind.high_pass, 16000.0, 80.0, 0.7071, 0.0) catch return;
    let hp = BiquadFilter.init(hp_c);
    let active_16k: []f32 = pcm_16k_buf[0..n16];
    hp.processSliceInPlaceF32(*active_16k);

    // 3. STFT 频谱与抛物线精确主频检测
    let re: [512]f64 = [0.0; 512];
    let im: [512]f64 = [0.0; 512];
    let mag: [257]f64 = [0.0; 257];
    let re_s: []f64 = re[0..];
    let im_s: []f64 = im[0..];
    let mag_s: []f64 = mag[0..];
    const bins = computeStftFrameMagnitude(active_16k, DspWindowKind.hann, *re_s, *im_s, *mag_s) catch return;
    const peak_hz = findPeakFrequencyHz(mag[0..bins], 16000.0) catch 0.0;

    // 4. ITU-R BS.1770 K-Weighted 响度与削波计量
    const loudness = measureAudioLoudnessF32(active_16k, 16000.0) catch return;
    * = peak_hz;
    * = loudness;
}
```

---

## 收敛子模块深度文档

- [aac.md](media/aac.md)
- [audio.md](media/audio.md)
- [dls.md](media/dls.md)
- [ebml.md](media/ebml.md)
- [flac.md](media/flac.md)
- [flv.md](media/flv.md)
- [id3.md](media/id3.md)
- [it.md](media/it.md)
- [midi.md](media/midi.md)
- [mkv.md](media/mkv.md)
- [mp4.md](media/mp4.md)
- [mpeg.md](media/mpeg.md)
- [nsf.md](media/nsf.md)
- [ogg.md](media/ogg.md)
- [opus.md](media/opus.md)
- [riff.md](media/riff.md)
- [s3m.md](media/s3m.md)
- [sf2.md](media/sf2.md)
- [spc.md](media/spc.md)
- [srt.md](media/srt.md)
- [vgm.md](media/vgm.md)
- [vtt.md](media/vtt.md)
- [xm.md](media/xm.md)
