# CP 标准库：音视频、容器、字幕与音频元数据套件手册 (`cap:media`)

> 来源：Pure-CP 零 C 依赖音视频与多媒体标准库收敛（23 个子模块统一聚合） | 创建日期：2026-11-22

## 一、 架构总览

`cap:media` 是 CP 语言官方内置的纯原生、零外部 C 依赖音视频与多媒体标准库，涵盖 **音频编解码与 Voice AI DSP/VAD、主流音视频二进制容器解析（MP4/MKV/WebM/FLV/Ogg/EBML/RIFF）、音色库（SoundFont SF2 / MMA DLS）、音轨模块（IT / S3M / XM）、芯片音乐（NSF / NSFe / SPC700 / VGM）、音频元数据（ID3v1/v2）以及字幕格式（SRT/WebVTT）**。

---

## 二、 模块文件拓扑与核心 API 索引

### 1. 音频编解码、音乐与 Voice AI DSP（7 个子模块）

| 子模块文件 | 领域分类 | 核心导出符号 |
| :--- | :--- | :--- |
| `lib/media/audio_wav.cp` | RIFF/WAVE PCM16 & Float32 编解码 | `WavCodec`, `WavAudio`, `WavFloatAudio`, `AudioError` |
| `lib/media/audio_dsp.cp` | Voice AI 重采样、RMS/dB 计量与能量 VAD | `AudioDsp`, `EnergyVad`, `SpeechSegment` |
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

> **重叠对照参考归档说明**：`lib/media/wav_.cp` 为与 `lib/media/audio_wav.cp` 功能重叠的对照参考副本（文件首行带 `// Reference Only:` 声明），不在 `lib/media/index.cp` 冲突导出。

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

## 四、 实时语音 AI 前端：SIMD 音频预处理与自适应 VAD 管线（`dsp_pipeline.cp`）

面向实时语音 Agent、ASR/Whisper 前端与低延迟流式语音检测的 100% 纯 CP 零堆分配 DSP 管线：

- **8-lane SIMD 帧能量与过零率分析（`AudioFrameStats` / `analyzeAudioFrameF32`）**：利用 `@dot` 硬件内积加速计算 RMS 能量、dBFS、绝对峰值与过零率（ZCR）。
- **自适应底噪与迟滞状态机 VAD（`VadConfig` / `VadState` / `VadDecision` / `AdaptiveEnergyVad`）**：融合能量阈值、自适应 EMA 底噪跟踪、ZCR 频段门控以及起音（`min_speech_frames`）/ 尾音（`hangover_frames`）迟滞防抖，精确输出 `Silence | SpeechStart | Speech | SpeechEnd`（与 `audio_dsp.cp` 的离线批量 `EnergyVad` 互补无冲突）。
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
