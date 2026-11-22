# `cap:opus` — 纯 CP RFC 7845 OpusHead/OpusTags 与 RFC 6716 TOC 零分配解析库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:media`，导入请使用 `import { ... } from "cap:media"`。

## 概述

`cap:opus` 提供纯 CP、零外部依赖、零堆分配的 Opus 音频元信息与分组头解析库，完整覆盖：
- **RFC 7845 `OpusHead`**：19 字节基础标识头解码（主版本校验、`channels >= 1`、`pre_skip`、`input_sample_rate`、Q7.8 `output_gain`、`channel_mapping_family`），并在 `family != 0` 时解析多声道扩展映射表（`stream_count`、`coupled_count` 与 `channel_mapping` 合法性校验）；
- **RFC 7845 `OpusTags`**：厂商字符串与零拷贝 `KEY=VALUE` 用户注释流式迭代器（`OpusCommentIter`）；
- **RFC 6716 Opus TOC**：全量 32 档 `config`（SILK / Hybrid / CELT、NB/MB/WB/SWB/FB 带宽、2.5ms~60ms 帧长）、立体声位、`c=0/1/2/3` 帧计数、VBR/Padding 标志以及 120ms 包时长上限校验。

## 核心 API

| 符号 | 说明 |
| :--- | :--- |
| `OPUS_HEAD_MAGIC`, `OPUS_TAGS_MAGIC`, `OPUS_SAMPLE_RATE`, `OPUS_MAX_PACKET_DURATION_US` | 8 字节魔数常量、`48000` Hz 内部采样率与 `120000` us 单包时长上限 |
| `isOpusHead(data)` / `isOpusTags(data)` | 探测 8 字节 `"OpusHead"` / `"OpusTags"` 魔数 |
| `OpusHead` / `parseOpusHead(data)` | 解析 RFC 7845 标识头与多声道扩展映射表；提供 `outputGainDb()`、`preSkipUs()` |
| `OpusTag` / `OpusTagsInfo` / `OpusCommentIter` / `parseOpusTags(data)` | 解析 RFC 7845 注释头并零拷贝迭代 `KEY=VALUE` 标签 |
| `OpusMode` / `OpusBandwidth` / `OpusToc` / `parseOpusToc(packet)` | 解码 RFC 6716 TOC 字节与包内总帧数/总时长；提供 `channels()`、`samplesPerFrame()`、`totalSamples()` |

## 示例

```cp
import { parseOpusHead, parseOpusTags, parseOpusToc } from "cap:media";

fn inspectOpus(head_pkt: []const u8, tags_pkt: []const u8, audio_pkt: []const u8): u32 {
    if (head := parseOpusHead(head_pkt)) {
        if (tags := parseOpusTags(tags_pkt)) {
            let it = tags.comments();
            while (tag := it.next()) {
                * = tag.key.len;
            }
        }
        if (toc := parseOpusToc(audio_pkt)) {
            return toc.packet_duration_us + (head.channels as u32);
        }
    }
    return 0;
}
```
