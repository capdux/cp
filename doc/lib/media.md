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

### 无上限外部切片 RFC 3550 RTP/RTCP、G.711 PCMU/PCMA 编解码与自适应 Jitter Buffer / PLC 引擎 (`rtpJitter.cp`)

面向实时音视频流媒体、VoIP/SIP 网关与语音 AI Agent 流式 ASR/TTS 传输的 100% 纯 CP 零动态堆分配引擎，所有包缓冲、PCM16 切片与 Jitter Buffer 槽位/负载池均由调用方外部切片传入：

- **常量与错误集**：
  - `RTP_VERSION = 2`、`RTP_MIN_HEADER_LEN = 12`、`RTCP_PT_SR = 200`、`RTCP_PT_RR = 201`、`RTCP_PT_BYE = 203`、`RTP_PT_PCMU = 0`、`RTP_PT_PCMA = 8`、`RTP_PT_OPUS = 111`
  - `RtpMediaError`（`PacketTooShort | UnsupportedVersion | InvalidPadding | InvalidExtension | InvalidRtcpLength | BufferTooSmall | PayloadTooLarge | InvalidConfig`）
- **RFC 3550 RTP 零拷贝编解码 & 16-bit 序号回绕展开**：
  - `RtpHeader`、`parseRtpPacket(raw)`、`encodeRtpPacket(hdr, payload, *out_buf)`（完整支持 CSRC 列表、RFC 3550 扩展头与 Padding 剥离）
  - `RtpSeqUnwrapper`（16-bit -> 64-bit 安全单调展开，无歧义处理 `65535 -> 0` 回绕与乱序旧包）
- **RFC 3550 RTCP SR/RR & 实时抖动/丢包统计**：
  - `RtcpSenderReport`（`encodeRtcpSr` / `parseRtcpSr`）、`RtcpReportBlock`（`encodeRtcpRr` / `parseRtcpRr`）
  - `RtpRecvStats`（RFC 3550 §A.8 1/16 平滑 `interarrival_jitter`、`jitterMs()`、`buildReportBlock(*_, lsr, dlsr)`）
- **ITU-T G.711 μ-law (PCMU) & A-law (PCMA) 实时语音编解码与转码**：
  - `linear16ToPcmu` / `pcmuToLinear16` / `linear16ToPcma` / `pcmaToLinear16`
  - `encodePcmuSlice` / `decodePcmuSlice` / `encodePcmaSlice` / `decodePcmaSlice` / `transcodePcmuToPcma` / `transcodePcmaToPcmu`
- **NetEQ 风格自适应 Jitter Buffer & 平滑淡出 PLC**：
  - `JitterPopKind`（`audio_packet | plc_concealment | empty_underrun`）、`JitterSlot`、`JitterPopResult`、`JitterBufferStats`
  - `AdaptiveJitterBuffer`（`init(slot_stride, sample_rate_hz, min_delay_ms, max_delay_ms, *slots, *payload_pool)`、`pushPacket`、`popNextFrame`、`stats`）
  - `applyPcm16Plc(prev_frame, consecutive_losses, *out_frame)`（多帧连续丢包指数淡出至静音防爆音）

```cp
import {
    RTP_PT_PCMU,
    RtpHeader,
    encodeRtpPacket,
    parseRtpPacket,
    encodePcmuSlice,
    decodePcmuSlice,
    JitterSlot,
    JitterPopKind,
    AdaptiveJitterBuffer,
    applyPcm16Plc,
} from "cap:media";

pub fn demoRealtimeVoicePipeline(pcm_frame: []const i16)! {
    // 1. PCM16 -> G.711 PCMU 编码并封装 RTP 报文
    let ulaw_buf: [160]u8 = [0; 160];
    let ulaw_s: []u8 = ulaw_buf[0..];
    const enc_n = encodePcmuSlice(pcm_frame, *ulaw_s)?;

    let wire: [256]u8 = [0; 256];
    let wire_s: []u8 = wire[0..];
    const hdr = RtpHeader{ payload_type: RTP_PT_PCMU, seq: 100, timestamp: 16000, ssrc: 0x12345678 };
    const wire_len = encodeRtpPacket(hdr, ulaw_buf[0..enc_n], *wire_s)?;

    // 2. 接收端解包并送入自适应 Jitter Buffer
    const parsed = parseRtpPacket(wire[0..wire_len])?;
    let slots: [16]JitterSlot = [JitterSlot{}; 16];
    let pool: [2560]u8 = [0; 2560];
    let slots_s: []JitterSlot = slots[0..];
    let pool_s: []u8 = pool[0..];
    let jb = AdaptiveJitterBuffer.init(160, 8000, 20, 200, *slots_s, *pool_s)?;
    * = jb.pushPacket(parsed, wire[parsed.payload_offset..(parsed.payload_offset + parsed.payload_len)], 100)?;

    // 3. 播放时刻出队并解码回 PCM16（缺包自动触发 PLC）
    let pop_buf: [160]u8 = [0; 160];
    let pop_s: []u8 = pop_buf[0..];
    const pop = jb.popNextFrame(125, *pop_s)?;
    let pcm_out: [160]i16 = [0; 160];
    let pcm_out_s: []i16 = pcm_out[0..];
    if (pop.kind == JitterPopKind.audio_packet) {
        * = decodePcmuSlice(pop_buf[0..pop.payload_len], *pcm_out_s)?;
    } else if (pop.kind == JitterPopKind.plc_concealment) {
        * = applyPcm16Plc(pcm_frame, 1, *pcm_out_s)?;
    }
}
```

### 无上限外部切片图像处理、YUV420p/NV12 色彩互转、可分离缩放/卷积与 BMP/PPM 编解码引擎 (`imageOps.cp`)

面向 WebRTC 视频帧处理、摄像头 NV12/I420 转换与 VLM 视觉前处理（如 `336x336` / `1024x1024` / 4K 缩放）的 100% 纯 CP 零动态堆分配图像内核，全部像素与行缓冲由调用方外部切片传入：

- **错误集、枚举与视图元数据**：
  - `ImageOpsError`（`InvalidDimensions | OddDimensionsForYuv420 | BufferTooSmall | InvalidFormat | UnsupportedBitsPerPixel | InvalidKernel | InvalidCrop`）
  - `ColorMatrixKind`（`bt601_limited | bt601_full | bt709_limited | bt709_full`）
  - `ResizeFilterKind`（`nearest | bilinear | bicubic_catmull_rom | lanczos3`）
  - `ImageViewMeta`（`width`, `height`, `channels`, `stride_bytes`, `requiredBytes()`, `isValid()`）、`resizeRgbaScratchLen(src_h, dst_w)`
- **视频帧色彩空间互转**：
  - `rgbaToGrayscale`（BT.709 亮度）、`rgbaToYuv420p` / `yuv420pToRgba`（2x2 色度均值下采样）、`rgbaToNv12` / `nv12ToRgba`（半平面 Y + 交错 UV）
- **几何变换与两趟可分离缩放**：
  - `cropRgba` / `flipRgbaHorizontal` / `flipRgbaVertical` / `rotateRgba90Cw`
  - `resizeRgba(src, src_w, src_h, dst_w, dst_h, filter, *dst, *scratch_f32)`（水平+垂直两趟可分离滤波，下采样自动抗混叠缩放，零纯色漂移）
- **空间卷积与边缘检测**：
  - `gaussianBlurRgba(src, width, height, sigma, radius, *dst, *scratch_f32)`（归一化 1D 高斯核水平+垂直可分离卷积）
  - `sobelEdgeMagnitude(gray, width, height, *out_mag)`（3x3 Sobel Gx/Gy 梯度幅值）
- **无损容器编解码 & 客观画质**：
  - `encodePpmP6` / `decodePpmP6`（支持 `#` 注释与任意空白分隔）
  - `encodeBmpRgba` / `decodeBmpToRgba`（支持 24-bit BGR 4 字节对齐填充与 32-bit BGRA，同时支持正/负高度 bottom-up / top-down）
  - `computePsnrRgba(a, b, width, height): f64`

```cp
import {
    ColorMatrixKind,
    ResizeFilterKind,
    rgbaToNv12,
    nv12ToRgba,
    resizeRgba,
    encodeBmpRgba,
} from "cap:media";

pub fn demoVideoAndVlmPreprocess(src_rgba: []const u8)! {
    // 1. 16x16 RGBA -> NV12 (BT.709 Full) -> RGBA
    let y_buf: [256]u8 = [0; 256];
    let uv_buf: [128]u8 = [0; 128];
    let out_rgba: [1024]u8 = [0; 1024];
    let y_s: []u8 = y_buf[0..];
    let uv_s: []u8 = uv_buf[0..];
    let rgba_s: []u8 = out_rgba[0..];
    rgbaToNv12(src_rgba, 16, 16, ColorMatrixKind.bt709_full, *y_s, *uv_s)?;
    nv12ToRgba(y_buf[0..], uv_buf[0..], 16, 16, ColorMatrixKind.bt709_full, *rgba_s)?;

    // 2. Lanczos-3 缩放到 8x8 并导出 32-bit BMP
    let dst_8x8: [256]u8 = [0; 256];
    let scratch: [512]f32 = [0.0; 512]; // >= 16 * 8 * 4
    let dst_s: []u8 = dst_8x8[0..];
    let sc_s: []f32 = scratch[0..];
    resizeRgba(out_rgba[0..], 16, 16, 8, 8, ResizeFilterKind.lanczos3, *dst_s, *sc_s)?;

    let bmp: [512]u8 = [0; 512];
    let bmp_s: []u8 = bmp[0..];
    * = encodeBmpRgba(dst_8x8[0..], 8, 8, *bmp_s)?;
}
```

### 无上限外部切片 RFC 8216 HLS/LL-HLS 清单编解码、MPEG-DASH 模板与 Dual-EWMA/BOLA ABR 引擎 (`hlsDash.cp`)

面向现代自适应流媒体（HLS / LL-HLS / MPEG-DASH）服务端打包与客户端播放器的 100% 纯 CP 零动态堆分配引擎，所有分片、变体、低延迟 Part 与格式化缓冲均由调用方外部切片驱动：

- **错误集与枚举**：
  - `StreamManifestError`（`MissingExtM3u | InvalidTag | InvalidNumber | InvalidByteRange | BufferTooSmall | OutputSliceTooSmall | InvalidTemplate | InvalidConfig`）
  - `HlsPlaylistKind`（`master | media_vod | media_event | media_live`）
- **RFC 8216 HLS Master Playlist**：
  - `HlsVariant`（零拷贝 `codecs(src)` / `uri(src)` 视图，含 `bandwidth_bps`, `average_bandwidth_bps`, `width`, `height`, `frame_rate_milli`）、`HlsVariantSpec`
  - `parseHlsMasterPlaylist(raw, *out_variants)`（完整处理引号内逗号 `CODECS="avc1.640028,mp4a.40.2"`）、`formatHlsMasterPlaylist(variants_spec, *out_buf)`
- **RFC 8216 HLS & LL-HLS Media Playlist**：
  - `HlsByteRange`、`HlsSegment`、`HlsSegmentSpec`、`HlsPartialSegment`（`#EXT-X-PART`）、`HlsMediaPlaylistInfo`
  - `parseHlsMediaPlaylist(raw, *out_segments, *out_parts)`（完整支持 `#EXT-X-BYTERANGE` 隐式累加偏移、`#EXT-X-DISCONTINUITY`、`#EXT-X-KEY:METHOD=AES-128`、`#EXT-X-PART`、`#EXT-X-ENDLIST`）、`formatHlsMediaPlaylist(...)`
- **MPEG-DASH `SegmentTemplate` 展开器 (ISO/IEC 23009-1)**：
  - `DashTemplateParams`、`expandDashSegmentTemplate(template, params, *out_buf)`（支持 `$RepresentationID$`、`$Number%05d$`、`$Bandwidth$`、`$Time$` 与 `$$` 字面转义）
- **生产级 ABR 自适应码率控制器**：
  - `AbrStrategy`（`throughput_ewma | bola_buffer | hybrid_safe`）、`AbrLevel`、`AbrDecision`
  - `AbrController`（Fast/Slow 双 EWMA 吞吐估算 + BOLA 对数缓冲效用 + 安全系数 + 升档滞回冷却 + 低缓冲紧急降档；提供 `init`, `recordDownload`, `estimatedBandwidthBps`, `selectLevel`）

```cp
import {
    HlsVariant,
    parseHlsMasterPlaylist,
    DashTemplateParams,
    expandDashSegmentTemplate,
    AbrStrategy,
    AbrLevel,
    AbrController,
} from "cap:media";

pub fn demoStreamingAbr(master_m3u8: []const u8)! {
    // 1. 零堆解析 HLS Master Playlist
    let vars: [8]HlsVariant = [HlsVariant{}; 8];
    let vars_s: []HlsVariant = vars[0..];
    const n_vars = parseHlsMasterPlaylist(master_m3u8, *vars_s)?;

    // 2. 展开 MPEG-DASH SegmentTemplate URL
    let url_buf: [128]u8 = [0; 128];
    let url_s: []u8 = url_buf[0..];
    const url_len = expandDashSegmentTemplate(
        "seg/$RepresentationID$/chunk_$Number%05d$.m4s",
        DashTemplateParams{ representation_id: "1080p", number: 42 },
        *url_s,
    )?;

    // 3. Dual-EWMA + BOLA 混合 ABR 决策
    let abr = AbrController.init(AbrStrategy.hybrid_safe, 0.85, 2000, 20000, 4000)?;
    abr.recordDownload(1_500_000, 2000); // 6 Mbps
    const levels: [3]AbrLevel = [
        AbrLevel{ level_id: 0, bitrate_bps: 800_000, width: 640, height: 360 },
        AbrLevel{ level_id: 1, bitrate_bps: 2_500_000, width: 1280, height: 720 },
        AbrLevel{ level_id: 2, bitrate_bps: 5_000_000, width: 1920, height: 1080 },
    ];
    const pick = abr.selectLevel(levels[0..], 12_000, 10_000);
    * = n_vars;
    * = url_len;
    * = pick;
}
```

### 7. CMAF / Fragmented MP4 (fMP4) 与 Ogg-Opus 流式容器复用/解复用引擎 (`fmp4Mux.cp`)

面向 LL-HLS / MPEG-DASH CMAF 分片与实时语音 Opus 流的 100% 纯 CP 零动态堆分配、外部切片驱动流媒体容器引擎：

- **常量与错误集**：
  - ISO BMFF 4CC 常量：`FMP4_BOX_FTYP` / `MOOV` / `MVHD` / `TRAK` / `TKHD` / `MDIA` / `MDHD` / `HDLR` / `MINF` / `STBL` / `MVEX` / `TREX` / `MOOF` / `MFHD` / `TRAF` / `TFHD` / `TFDT` / `TRUN` / `MDAT`
  - 同步样本标志：`FMP4_SAMPLE_FLAG_SYNC` (`0x02000000`)、`FMP4_SAMPLE_FLAG_NON_SYNC` (`0x01010000`)
  - Ogg/Opus 常量：`OGG_CAPTURE_PATTERN` (`"OggS"`)、`OGG_PAGE_FLAG_CONT` / `BOS` / `EOS`、`OPUS_HEAD_MIN_LEN` (`19`)
  - `Fmp4MuxError`（`InvalidBox | TruncatedBox | BufferTooSmall | OutputSliceTooSmall | InvalidConfig | InvalidOggMagic | OggChecksumMismatch | InvalidOpusHead | TooManySegments`）
- **ISO BMFF / CMAF Init Segment 与 Box 迭代**：
  - `Fmp4TrackKind`（`video_avc | video_hevc | video_av1 | audio_aac | audio_opus`）、`Fmp4InitConfig`
  - `encodeFmp4InitSegment(cfg, out_buf): usize ! Fmp4MuxError`（自动嵌套回填 `ftyp` + `moov`/`mvhd`/`trak`/`mdia`/`mvex`/`trex`）
  - `Fmp4BoxHeader`、`Fmp4BoxIter`（支持 32-bit 与 64-bit `largesize` Box，安全拦截截断与畸形长度）
- **CMAF Media Fragment (`moof` + `mdat`) 复用与解复用**：
  - `Fmp4SampleMeta`、`Fmp4FragmentHeader`、`Fmp4ParsedSample`
  - `encodeFmp4Fragment(seq_num, track_id, base_decode_time, samples, concatenated_payload, out_buf)`（写入 64-bit `tfdt` 与含精确 `trun.data_offset == moof_size + 8` 的 `trun` v1）
  - `parseFmp4Fragment(raw, out_samples)`（还原每帧 `pts`/`dts`/`duration`/`data_offset`/`data_len`/`is_keyframe`）
- **RFC 3533 Ogg 页封装 + RFC 7845 OpusHead / OpusTags**：
  - `computeOggCrc32(data)`（标准 Ogg 非反射多项式 `0x04C11DB7`）
  - `OggPageHeader`、`encodeOggPage`（自动 255 字节切分 lacing table 与 CRC32 回填）、`parseOggPage`（无修改全量 CRC32 校验）
  - `OpusHeadInfo`、`encodeOpusHead`、`parseOpusHeadInfo`、`encodeOpusTags`

```cp
import {
    Fmp4TrackKind,
    Fmp4InitConfig,
    encodeFmp4InitSegment,
    Fmp4SampleMeta,
    FMP4_SAMPLE_FLAG_SYNC,
    FMP4_SAMPLE_FLAG_NON_SYNC,
    encodeFmp4Fragment,
    Fmp4ParsedSample,
    parseFmp4Fragment,
    OpusHeadInfo,
    encodeOpusHead,
    encodeOggPage,
    OGG_PAGE_FLAG_BOS,
} from "cap:media";

pub fn demoFmp4AndOggOpus()! {
    // 1. 生成 CMAF Video Init Segment (ftyp + moov)
    let init_buf: [1024]u8 = [0; 1024];
    const init_len = encodeFmp4InitSegment(
        Fmp4InitConfig{ track_id: 1, timescale: 90000, kind: Fmp4TrackKind.video_avc, width: 1920, height: 1080 },
        init_buf[0..],
    )?;

    // 2. 封装 2 帧 Media Fragment (moof + mdat) 并解复用
    const samples: [2]Fmp4SampleMeta = [
        Fmp4SampleMeta{ duration: 3000, size: 4, flags: FMP4_SAMPLE_FLAG_SYNC, cts_offset: 3000 },
        Fmp4SampleMeta{ duration: 3000, size: 4, flags: FMP4_SAMPLE_FLAG_NON_SYNC, cts_offset: -3000 },
    ];
    const payload: [8]u8 = [1, 2, 3, 4, 5, 6, 7, 8];
    let frag_buf: [512]u8 = [0; 512];
    const frag_len = encodeFmp4Fragment(1, 1, 900000, samples[0..], payload[0..], frag_buf[0..])?;
    let parsed: [4]Fmp4ParsedSample = [Fmp4ParsedSample{}; 4];
    const hdr = parseFmp4Fragment(frag_buf[0..frag_len], parsed[0..])?;

    // 3. 打包 RFC 7845 OpusHead 到 RFC 3533 Ogg BOS 页
    let head_buf: [32]u8 = [0; 32];
    const head_len = encodeOpusHead(OpusHeadInfo{ channels: 2, pre_skip: 312, input_sample_rate: 48000 }, head_buf[0..])?;
    let page_buf: [128]u8 = [0; 128];
    const page_len = encodeOggPage(OGG_PAGE_FLAG_BOS, 0, 0x12345678, 0, head_buf[0..head_len], page_buf[0..])?;
    * = init_len;
    * = hdr.sample_count;
    * = page_len;
}
```

### 8. 实时语音 ASR/TTS 声学特征前端与端点检测引擎 (`asrFeature.cp`)

对标 OpenAI Whisper / Conformer 与 Kaldi/HTK 的 100% 纯 CP 零动态堆分配、外部切片驱动声学特征前端，直接复用 `fftRadix2InPlace` 并以原生 SIMD `@dot` 加速 Mel 投影与 DCT-II：

- **错误集、标度与配置**：
  - `AsrFeatureError`（`InvalidSampleRate | InvalidFftSize | InvalidHopLength | InvalidMelBins | InvalidMfccCount | InvalidFreqRange | BufferTooSmall | InputTooShort`）
  - `MelScaleKind`（`htk | slaney`）
  - `AsrFrontendConfig`（`whisperDefaults()` 80-bin / `whisperLargeV3Defaults()` 128-bin / `isValid()` / `effectiveWinLen()` / `nBins()`）
  - 精确缓冲容量计算：`asrFrameCount(num_samples, n_fft, hop_length)`、`asrMelBankLen(n_mels, n_fft)`、`asrMelOutLen(num_frames, n_mels)`
- **Mel 标度换算与 Whisper/Conformer Log-Mel 频谱图**：
  - `hzToMelScale(hz, kind)` / `melScaleToHz(mel, kind)`
  - `buildMelFilterbankMatrix(cfg, out_weights)`（支持 Slaney 等面积归一化 `2.0 / (f_r - f_l)`）
  - `extractLogMelSpectrogram(pcm, cfg, mel_weights, fft_re, fft_im, power_buf, out_log_mel)`（预加重 -> 周期 Hann 窗 -> `fftRadix2InPlace` -> `@dot` 投影 -> Whisper 标准 `log10` 动态范围钳位 `(max(v, max_v - 8) + 4) / 4`）
- **MFCC + Delta/Delta-Delta + CMVN + SpecAugment**：
  - `computeMfccFromLogMel` / `buildDct2Matrix` + `computeMfccFastDot`（`@dot` 加速）
  - `applyCepstralLifteringInPlace`、`computeDeltaFeatures`（HTK/Kaldi 回归斜率）、`packMfccWithDeltas`（打包 39 维 `[MFCC, Δ, ΔΔ]`）
  - `applyCmvnInPlace`（帧维零均值单位方差归一化）、`StreamingCmvnNormalizer`（Welford 因果在线流式归一化）、`applySpecAugmentInPlace`（确定性时频遮蔽）
- **实时双门限端点检测器（ASR Endpointing VAD）**：
  - `EndpointState`（`silence | speech_onset | speech_active | speech_hangover`）、`SpeechSegmentSpan`、`EndpointVadConfig`
  - `StreamingEndpointVad`（自适应底噪跟踪 + RMS 能量 + 过零率 + Wiener 频谱平坦度 + 迟滞状态机）与批量切分 `detectSpeechSegments`

```cp
import {
    AsrFrontendConfig,
    buildMelFilterbankMatrix,
    extractLogMelSpectrogram,
    computeMfccFromLogMel,
    computeDeltaFeatures,
    applyCmvnInPlace,
    EndpointVadConfig,
    SpeechSegmentSpan,
    detectSpeechSegments,
} from "cap:media";

pub fn demoWhisperAsrFrontend(pcm_1s_16k: []const f32)! {
    const cfg = AsrFrontendConfig.whisperDefaults(); // 16kHz, n_fft=512, win=400, hop=160, 80-bin Slaney
    let mel_w: [20560]f32 = [0.0; 20560]; // 80 * 257
    * = buildMelFilterbankMatrix(cfg, mel_w[0..])?;

    let fft_re: [512]f64 = [0.0; 512];
    let fft_im: [512]f64 = [0.0; 512];
    let power: [257]f32 = [0.0; 257];
    let log_mel: [8000]f32 = [0.0; 8000]; // up to 100 frames x 80 bins
    const n_frames = extractLogMelSpectrogram(pcm_1s_16k, cfg, mel_w[0..], fft_re[0..], fft_im[0..], power[0..], log_mel[0..])?;

    let mfcc: [1300]f32 = [0.0; 1300];
    let delta: [1300]f32 = [0.0; 1300];
    computeMfccFromLogMel(log_mel[0..n_frames * 80], n_frames, 80, 13, mfcc[0..])?;
    computeDeltaFeatures(mfcc[0..n_frames * 13], n_frames, 13, 2, delta[0..])?;
    applyCmvnInPlace(mfcc[0..n_frames * 13], n_frames, 13, true)?;

    let segs: [8]SpeechSegmentSpan = [SpeechSegmentSpan{}; 8];
    * = detectSpeechSegments(pcm_1s_16k, cfg, EndpointVadConfig.defaults(), fft_re[0..], fft_im[0..], power[0..], segs[0..])?;
}
```

### 9. H.264/AVC + H.265/HEVC NAL 比特流、SPS 解析、Annex-B↔AVCC 与 RFC 6184/7798 RTP 分包重组引擎 (`videoNal.cp`)

打通 CP WebRTC / RTP / CMAF fMP4 / HLS 实时视频管线的 100% 纯 CP 零动态堆分配、外部切片驱动视频比特流核心：

- **NAL 扫描、Annex-B↔AVCC 互转与 EBSP↔RBSP**：
  - `VideoNalError`、`VideoCodecKind` (`h264 | h265`)、`H264_NAL_*` / `H265_NAL_*` 类型常量
  - `AnnexBNalUnit`、`AnnexBIter`（精确识别 `00 00 01` 与 `00 00 00 01`）、`AvccIter`（支持 1/2/4 字节长度前缀）
  - `annexBToAvcc` / `avccToAnnexB`、`ebspToRbsp` / `rbspToEbsp`（ITU-T §7.3.1 `00 00 03` 防竞争字节脱壳/加壳）
- **Exp-Golomb 比特流与 H.264 / H.265 SPS 解析**：
  - `NalBitReader`（`readBit` / `readBits` / `skipBits` / `readUe` / `readSe`）
  - H.264：`H264NalHeader`、`parseH264NalHeader`、`H264SpsInfo`、`parseH264Sps`（含 High/High10/High422/High444 扩展、scaling matrix 跳过与精确裁剪宽高如 `1920x1088 -> 1920x1080`）、`encodeAvcDecoderConfigRecord`（生成 ISO/IEC 14496-15 `avcC` 载荷）
  - H.265：`H265NalHeader`、`parseH265NalHeader`、`H265SpsInfo`、`parseH265SpsBasic`（`profile_tier_level` + `conformance_window` 精确裁剪）与统一关键帧判定 `isVideoKeyframe`
- **WebRTC 视频 RTP 分包与抗丢包重组（RFC 6184 & RFC 7798）**：
  - `RtpPayloadSpan`、`packetizeH264Rtp`（Single NAL + FU-A `type=28`）、`packH264StapA`（STAP-A `type=24`）、`H264RtpDepacketizer`（Single / STAP-A / FU-A 重组回 Annex-B，含残缺分片自动回滚）
  - `packetizeH265Rtp`（Single NAL + FU `type=49` 3 字节头）与 `H265RtpDepacketizer`（Single / AP `type=48` / FU `type=49` 重组回 Annex-B）

```cp
import {
    parseH264Sps,
    encodeAvcDecoderConfigRecord,
    RtpPayloadSpan,
    packetizeH264Rtp,
    H264RtpDepacketizer,
} from "cap:media";

pub fn demoH264RtpPipeline(sps_nal: []const u8, pps_nal: []const u8, idr_nal: []const u8)! {
    let rbsp: [256]u8 = [0; 256];
    const sps = parseH264Sps(sps_nal, rbsp[0..])?; // e.g. 1920x1080
    let avcc_rec: [256]u8 = [0; 256];
    * = encodeAvcDecoderConfigRecord(sps_nal, pps_nal, avcc_rec[0..])?;

    // MTU=1200 切分为 RFC 6184 FU-A 并重组回 Annex-B
    let rtp_buf: [4096]u8 = [0; 4096];
    let spans: [16]RtpPayloadSpan = [RtpPayloadSpan{}; 16];
    const n_pkts = packetizeH264Rtp(idr_nal, 1200, rtp_buf[0..], spans[0..])?;

    let annexb_out: [4096]u8 = [0; 4096];
    let depkt = H264RtpDepacketizer.init(annexb_out[0..]);
    let i: usize = 0;
    while (i < n_pkts) : (i += 1) {
        * = depkt.pushPacket(spans[i].slice(rtp_buf[0..]))?;
    }
    * = sps.width;
    * = depkt.bytes().len;
}
```

### 10. WebM/EBML 流式实时封装解封装、VP8/VP9/AV1 OBU 帧头解析与 RFC 7741 VP8 RTP 引擎 (`webmMux.cp`)

面向 WebRTC 录制、浏览器 MediaSource 实时流与免专利视频编码管线的 100% 纯 CP 零动态堆分配、外部切片驱动核心：

- **RFC 8794 EBML VINT 与元素 ID**：
  - `encodeEbmlVint` / `encodeEbmlVintFixed` / `decodeEbmlVint`（完整支持 1..8 字节变长整数与流式未知长度哨兵 `WEBM_EBML_UNKNOWN_SIZE_RAW = 0x01FFFFFFFFFFFFFF`）；
  - `encodeEbmlElementId` / `decodeEbmlElementId`（1..4 字节元素 ID）。
- **流式 WebM 封装与零拷贝解封装**：
  - `WebmMuxer`（支持已知长度回填与直播未知长度双模式）：写入合标 `EBML`（`DocType="webm"`, version 4/2）、`Segment`、`Info`（`TimecodeScale`、`MuxingApp`、`WritingApp`）、多轨 `Tracks`（`V_VP8`/`V_VP9`/`V_AV1` + `A_OPUS` 含 `OpusHead` `CodecPrivate`）及自动按 `i16` 相对时间码切分 `Cluster`/`SimpleBlock`；
  - `WebmDemuxIter`：零拷贝遍历已知/未知长度 `Segment` 与 `Cluster`，解析轨道元数据与逐帧 `WebmDemuxFrame`（`track_number`、`timestamp_ms`、`is_keyframe`、`payload`）。
- **VP8 / VP9 / AV1 OBU 比特流帧头解析**：
  - `parseVp8FrameHeader`（RFC 6386 3 字节未压缩头、关键帧 `0x9D 0x01 0x2A` 同步码与宽高/缩放因子）；
  - `parseVp9FrameHeader`（`frame_marker==0b10`、profile 0..3、`show_existing_frame`、`0x49 0x83 0x42` 同步码、色彩配置与帧宽高）；
  - `Av1ObuIter`（LEB128 尺寸与扩展头 `temporal_id`/`spatial_id`）与 `parseAv1SequenceHeaderBasic`（解析 `seq_profile`、`still_picture`、`reduced_still_picture_header` 与最大帧宽高）。
- **WebRTC VP8 RTP 分包与抗丢包重组（RFC 7741）**：
  - `Vp8RtpDescriptor` / `encodeVp8RtpDescriptor` / `decodeVp8RtpDescriptor`（完整支持 `X/N/S/PID` 及 `I` 7/15-bit `PictureID`、`L` `TL0PICIDX`、`T/K` `TID/Y/KEYIDX`）；
  - `packetizeVp8Rtp`（按 MTU 切分，首包 `S=1`、续包 `S=0`）与 `Vp8RtpDepacketizer`（跨 16-bit 序列号回绕重组，丢包/残缺帧自动丢弃并统计）。

```cp
import {
    WEBM_DEFAULT_TIMECODE_SCALE_NS,
    WebmVideoCodec,
    WebmTrackConfig,
    WebmMuxer,
    WebmDemuxTrackInfo,
    WebmDemuxIter,
} from "cap:media";

pub fn demoWebmMuxRoundtrip(vp9_keyframe: []const u8, opus_pkt: []const u8)! {
    let wire: [4096]u8 = [0; 4096];
    let mux = WebmMuxer.init(wire[0..], WEBM_DEFAULT_TIMECODE_SCALE_NS, false)?;
    const tracks: [2]WebmTrackConfig = [
        WebmTrackConfig.video(1, WebmVideoCodec.vp9, 1280, 720),
        WebmTrackConfig.opusAudio(2, 48000.0, 2, "OpusHead\x01\x02\x38\x01\x80\xBB\x00\x00\x00\x00\x00"),
    ];
    * = mux.writeHeader(tracks[0..], "cp-mux", "cap:media")?;
    * = mux.writeSimpleBlock(1, 0, true, false, false, vp9_keyframe)?;
    * = mux.writeSimpleBlock(2, 20, true, false, false, opus_pkt)?;
    const total = mux.finish()?;

    let out_tracks: [4]WebmDemuxTrackInfo = [WebmDemuxTrackInfo{}; 4];
    let demux = WebmDemuxIter.init(wire[0..total], out_tracks[0..])?;
    while (true) {
        const opt = demux.nextFrame()?;
        if (f := opt) { * = f.timestamp_ms; } else { break; }
    }
}
```

### 11. 生产级零堆 WAV / RF64 / WAVEFORMATEXTENSIBLE / PCM / Float32 / G.711 / IMA-ADPCM 引擎 (`audioWav.cp`)

在 100% 向下兼容既有 `AudioError` / `WavAudio` / `WavFloatAudio` / `WavCodec` 的基础上，原地深化为全格式零堆外部切片音频容器与编解码核心：

- **容器与扩展头**：
  - `WavSampleFormat`（`u8_pcm` / `s16_le` / `s24_le` / `s32_le` / `f32_le` / `mulaw` / `alaw` / `ima_adpcm`）、`WAV_FORMAT_*`、`WAV_CHANNEL_MASK_*`、`parseWavSubFormatGuid`；
  - `WavChunkIter` / `parseWavContainer`：零拷贝解析标准 `RIFF/WAVE`、`WAVE_FORMAT_EXTENSIBLE`（`0xFFFE` + 16 字节 `SubFormat` GUID）、EBU Tech 3306 `RF64` + `ds64` 64-bit 长度（>4GB）、`fact`、`LIST/INFO`（INAM/IART/IALB/ICRD/ICMT/ISFT）、`cue ` 与 `smpl`；
  - `encodeRf64Header` 与流式 `WavStreamWriter`（`init` -> `writePcm16`/`writeF32`/`writeRawBytes` -> `finalize`/`finalizeWithCues` 自动回填 `RIFF`/`data`/`fact` 长度）。
- **全位深转换与任意声道混音**：
  - `u8ToF32Slice`/`f32ToU8Slice`、`s16LeToF32Slice`/`f32ToS16LeSlice`（含可选 TPDF 抖动）、`s24LeToF32Slice`/`f32ToS24LeSlice`（3 字节紧缩读写）、`s32LeToF32Slice`/`f32ToS32LeSlice`、`f32LeBytesToF32Slice`/`f32SliceToF32LeBytes`；
  - `stereoToMonoF32`、`monoToStereoF32`、`interleaveChannelsF32`、`deinterleaveChannelsF32`（任意声道数，无上限）。
- **IMA-ADPCM (`0x0011`) 与 G.711 μ-law (`0x0007`) / A-law (`0x0006`)**：
  - `ImaAdpcmState`、`imaAdpcmBlockSamples`、`encodeImaAdpcmBlock`、`decodeImaAdpcmBlock`（标准 89 级步长表，支持单声道/立体声 4-byte 交错块，4:1 压缩比）；
  - `wavPcm16ToMulaw`/`wavMulawToPcm16`、`wavPcm16ToAlaw`/`wavAlawToPcm16`、`encodeWavMulaw`/`decodeWavMulaw`、`encodeWavAlaw`/`decodeWavAlaw`。

### 12. ISO/IEC 14496-3 AAC ADTS / ASC / MP4 `esds` / LOAS-LATM 与 RFC 3640 `AAC-hbr` RTP 引擎 (`aac.cp`)

在 100% 向下兼容既有 `AdtsFrame` / `AdtsIter` / `parseAudioSpecificConfig` / `summarizeAdts` 的基础上，原地深化为全套零堆外部切片 AAC 流媒体核心：

- **ADTS 帧编码、CRC16-IBM 校验与抗噪声重同步**：
  - `computeAdtsCrc16` / `verifyAdtsFrameCrc`（多项式 `0x8005`）、`AdtsEncodeConfig`、`encodeAdtsFrame`（7B 无 CRC）、`encodeAdtsFrameWithCrc`（9B 含 CRC16）；
  - `AdtsFrameIter`：跳过前置垃圾字节寻找 `0xFFF` 同步字并执行下一帧前瞻同步校验。
- **`AudioSpecificConfig` (ASC) 与 MP4 `esds` FullBox**：
  - `AacAudioSpecificConfig`、`encodeAudioSpecificConfig` / `decodeAudioSpecificConfig`（支持扩展 AOT>=32、24-bit 显式采样率、`GASpecificConfig`、HE-AAC SBR `0x2B7` 与 PS `0x548` 扩展）；
  - `adtsToAudioSpecificConfig` / `audioSpecificConfigToAdtsHeader` 无损互转；
  - `Mp4EsdsInfo`、`encodeMp4EsdsBox` / `decodeMp4EsdsBox`（Tag `0x03`/`0x04`/`0x05`/`0x06` + 7-bit 变长扩展长度，直通 `fmp4Mux.cp`）。
- **LOAS / LATM (ISO/IEC 14496-3 §1.7.3)**：
  - `LoasFrameView`、`encodeLoasFrame`、`LoasFrameIter`（11-bit `0x2B7` 同步字 + 13-bit 长度）；
  - `LatmParsedFrame`、`parseLatmStreamMuxConfigBasic`（解析 `StreamMuxConfig`、内嵌 ASC 与 `0xFF` 累加 `PayloadLengthInfo`，提取纯 AAC Access Unit）。
- **RFC 3640 `AAC-hbr` WebRTC/RTSP RTP 分包重组与 SDP `fmtp`**：
  - `AacRtpAuSpan`、`AacRtpPacketSpan`、`packetizeAacRtpHbr`（多帧聚合 + 超 MTU 分片）、`depacketizeAacRtpHbr`（零拷贝多帧解包）与 `formatAacSdpFmtp`。

### 13. RFC 6716 Opus 帧打包解包、LBRR-FEC/DTX 比特级探测与 RFC 7587 WebRTC RTP 引擎 (`opus.cp`)

在 100% 向下兼容既有 RFC 7845 `OpusHead`/`OpusTags` 与 RFC 6716 `parseOpusToc` 的基础上，原地深化为全套零堆外部切片 Opus 实时语音核心：

- **RFC 6716 Code 0 / 1 / 2 / 3 帧解析与最紧凑重打包**：
  - `encodeOpusFrameLen` / `decodeOpusFrameLen`（1 字节 `0..251` 与 2 字节 `252..1275`）；
  - `parseOpusPacketFrames`（外部切片零拷贝提取 1..48 帧，完整支持 Code 0/1/2/3 CBR/VBR、多字节 `255` 累加填充跳过及 `<= 120ms` 强校验）；
  - `repackOpusFrames`（自动选择最紧凑 Code 0/1/2/3 封装，支持目标包长 `target_packet_len` 精确零填充）。
- **RFC 6716 §4.2.7 SILK/Hybrid VAD、In-Band FEC (LBRR) 与 DTX 诊断**：
  - `OpusFrameSignalInfo`、`inspectOpusFrameSignal`、`OpusPacketDiagnostics`、`diagnoseOpusPacket`（一键输出模式、带宽、时长、样本数、DTX、VAD 与 LBRR FEC 标志）。
- **RFC 7587 WebRTC Opus SDP `fmtp` 与 RTP 聚合/拆包器**：
  - `OpusSdpFmtpConfig`、`parseOpusSdpFmtp`、`formatOpusSdpFmtp`（全量支持 `minptime`/`maxplaybackrate`/`maxaveragebitrate`/`stereo`/`sprop-stereo`/`cbr`/`useinbandfec`/`usedtx`）；
  - `OpusRtpPacketizer`（48kHz RTP 时间戳递增、DTX 静音后首个语音包自动置位 `Marker`、`aggregatePackets` 如 20ms×2 -> 40ms）与 `OpusRtpUnpacker`（`observePacket` 统计 + `splitToSingleFramePackets` 多帧拆分）。

### 14. MP3 / MPEG-1/2/2.5 帧同步、CRC16、Xing/LAME/VBRI TOC 寻址、Side-Info 与 RFC 2250 MPA RTP 引擎 (`mpeg.cp`)

在 100% 向下兼容既有 `MpegFrameHeader`/`MpegFrameIter`/`parseXingHeader`/`summarizeMpeg` 的基础上，原地深化为全套零堆外部切片 MP3/MPEG 音频核心：

- **帧头编码、CRC16-IBM (`0x8005`) 与抗噪声重同步**：
  - `Mp3HeaderSpec`、`encodeMp3FrameHeader`、`mp3SideInfoLen`、`computeMp3Crc16`、`verifyMp3FrameCrc`、`stampMp3FrameCrc`；
  - `hasTrailingId3v1` 与 `Mp3StreamIter`（跳过前置 ID3v2 + 垃圾字节，执行下一帧前瞻校验，并忽略尾部 128B ID3v1 标签）。
- **Xing / Info / LAME Gapless、VBRI、O(1) TOC 寻址与流扫描**：
  - `Mp3XingHeader`、`parseMp3XingHeader`（完整提取 100 字节 TOC 表 + LAME `encoder_delay` / `encoder_padding` 12-bit/12-bit 无缝播放参数）；
  - `Mp3VbriHeader`、`parseVbriHeader`（偏移 36 处 Fraunhofer VBRI 全字段）；
  - `seekMp3ByteOffsetByMs`（Xing TOC 线性插值或 CBR O(1) 毫秒寻址）与 `Mp3StreamSummary` / `scanMp3Stream`（min/max/avg 码率 + Gapless 裁剪样本统计）。
- **Layer III Side-Info 与 RFC 2250 MPA RTP**：
  - `Mp3GranuleChannelInfo`、`Mp3Layer3SideInfo`、`parseLayer3SideInfo`（`main_data_begin`、`private_bits`、`scfsi`、`part2_3_length`、`big_values`、`global_gain`）；
  - `Mp3RtpSpan`、`packetizeMp3Rtp`（4B `[MBZ:16 | Frag_offset:16]` 多帧聚合与超 MTU 分片）与 `Mp3RtpDepacketizer`。

---

---

## 原生 SIMD 加速 DSP & 图像核（`fftFilter.cp` / `imageOps.cp`）

- **音频 DSP SIMD 加速（`fftFilter.cp`）**：
  - `resampleSincF32`：64-lane 分块 `@dot` 加窗 Sinc 重采样核（支持任意 `half_taps`，内置溢出与非线性边界保护）。
  - `measureAudioLoudnessF32`：64-sample 分块 `@dot` 平方和能量归约（自动清洗 NaN/Inf）。
  - `dotF32Simd(a, b): f32`：原生 SIMD `@dot` 点积封装。
  - `convolveFirF32Simd(input, coeffs, *out): usize ! DspKernelError`：外部切片通用因果 FIR 滤波 SIMD 核（64-lane 分块 `@dot`，无抽头数上限）。
- **图像处理 SIMD 加速与防溢出加固（`imageOps.cp`）**：
  - `resizeRgba`（Pass 1/2 水平+垂直可分离缩放）与 `gaussianBlurRgba`（对称 1D 高斯卷积）：全面接入 `@dot` 向量化计算。
  - `computePsnrRgba`：64 像素分块 `@l2sq` 硬件向量化 MSE/PSNR 计算。
  - `rgbaChannelDotF32Simd(a, b, width, height): f64`：基于 `@dot` 的 RGB 归一化互相关（NCC）模板匹配核。
  - `decodePpmP6` / `decodeBmpToRgba` / `sobelEdgeMagnitude`：全面防整数溢出与 `i32::MIN` 翻转守卫，经 5,000 次确定性畸形输入 Fuzz 轰炸零崩溃验证。

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
