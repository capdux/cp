# `cap:mpeg` — 纯 CP MPEG-1/2/2.5 Layer I/II/III（MP3）帧头与 Xing/Info/VBRI 零分配解析库

## 概述

`cap:mpeg` 提供纯 CP、零外部依赖、零堆分配的 MPEG 音频（MP1 / MP2 / MP3）帧解析与时长汇总库，支持：
- **完整帧头解码**：11-bit `0xFFE` 同步字校验、MPEG-1 / MPEG-2 / MPEG-2.5 Layer I/II/III 全量比特率与采样率查表、每帧采样数（384 / 576 / 1152）与精确帧字节长计算；
- **ID3v2 自动跳过**：识别 `"ID3"` 前置标签（含 4B synchsafe 长度与可选 10B footer）；
- **流式零拷贝帧迭代**：`MpegFrameIter` 自动跳过 ID3v2 并逐帧返回零拷贝视图；
- **Xing / Info / VBRI VBR 头解析与时长汇总**：解析 Side-Info 偏移后的 `"Xing"` / `"Info"` 以及偏移 36 处的 `"VBRI"` 头，并由 `summarizeMpeg` 给出总帧数、时长（ms）、平均码率与 VBR 标志。

## 核心 API

| 符号 | 说明 |
| :--- | :--- |
| `MpegVersion`, `MpegLayer`, `MpegChannelMode` | MPEG 版本（`v2_5`/`v2`/`v1`）、层级（`layer1`/`layer2`/`layer3`）与声道模式枚举 |
| `MpegFrameHeader` / `parseMpegFrameHeader(data)` | 解码 4 字节 MPEG 帧头，提供 `channels()` 与 `frameDurationUs()` |
| `skipId3v2(data)` / `isMpegAudio(data)` | 跳过前置 ID3v2 标签；验证有效 MPEG 音频帧及后续连续同步字 |
| `MpegFrame` / `MpegFrameIter` | 零拷贝音频帧与流式迭代器（`init(data)`、`next(*_): MpegFrame?`） |
| `XingHeader` / `parseXingHeader(frame_data, header)` | 解析 `"Xing"` / `"Info"` / `"VBRI"` 头中的 `is_cbr`、`frames`、`bytes`、`quality` |
| `MpegSummary` / `summarizeMpeg(data)` | 综合 Xing 头或逐帧迭代计算 `total_frames`、`duration_ms`、`avg_bitrate_kbps`、`is_vbr` |

## 示例

```cp
import { isMpegAudio, summarizeMpeg } from "cap:mpeg";

fn inspectMp3(data: []const u8): u64 {
    if (!isMpegAudio(data)) return 0;
    if (info := summarizeMpeg(data)) {
        return info.duration_ms;
    }
    return 0;
}
```
