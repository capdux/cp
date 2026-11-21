# `cap:it` — 纯 CP Impulse Tracker (`.it`) 跟踪器音乐零分配解析库

## 概述

`cap:it` 提供纯 CP、零外部依赖、零堆分配的 Impulse Tracker 2.14（`.it` 小端二进制规范）解析器：

- **192B `"IMPM"` 文件头**：校验魔数与 4 张偏移表边界，提取曲名、版本、标志位（Stereo / Instruments / Linear Slides / Message）、初始速度与 BPM 及零拷贝 `songMessage`；
- **32-bit 偏移表寻址**：提供 `ordersSlice`、`instrumentOffset`、`sampleOffset` 与 `patternOffset` 零拷贝随机寻址；
- **`"IMPI"` 乐器与 80B `"IMPS"` 采样头**：解析 DOS 文件名、音量/声像、16-bit/Stereo/Compressed/Loop 标志、`length`/`loop_begin`/`loop_end`/`c5_speed`/`sample_pointer`；
- **Pattern 解析**：支持偏移为 0 的标准空 64 行 Pattern 与带 8B 头（`1..200` 行）的压缩 Pattern 零拷贝切片。

## 核心 API

| 符号 | 说明 |
| :--- | :--- |
| `IT_MAGIC` (`"IMPM"`), `IT_INST_MAGIC` (`"IMPI"`), `IT_SAMPLE_MAGIC` (`"IMPS"`), `IT_HEADER_SIZE` (`192`), `IT_SAMPLE_HEADER_SIZE` (`80`) | IT 规范魔数与头长常量 |
| `IT_FLAG_STEREO`, `IT_FLAG_USE_INSTRUMENTS`, `IT_FLAG_LINEAR_SLIDES`, `IT_SPECIAL_HAS_MESSAGE` | 头部标志位常量 |
| `isIt(data): bool` / `ItHeader` / `parseItHeader(data): ItHeader?` | 解析 192B 文件头与偏移表；附 `isStereo()`, `usesInstruments()`, `usesLinearSlides()`, `hasMessage()`, `songMessage(data)`, `ordersSlice(data)`, `instrumentOffset`, `sampleOffset`, `patternOffset` |
| `ItSample` / `parseItSample(data, header, idx): ItSample?` | 解析 80B `"IMPS"` 采样头；附 `hasData()`, `is16Bit()`, `isStereo()`, `isCompressed()`, `hasLoop()` |
| `ItInstrument` / `parseItInstrument(data, header, idx): ItInstrument?` | 解析 `"IMPI"` 乐器头 |
| `ItPattern` / `parseItPattern(data, header, idx): ItPattern?` | 解析空 Pattern（偏移 0 -> 64 行）或打包 Pattern |

## 示例

```cp
import { parseItHeader, parseItSample, parseItPattern } from "cap:it";

fn inspectIt(bytes: []const u8): u32 {
    const hdr = parseItHeader(bytes) ?? return 0;
    * = hdr.song_name;

    if (pat := parseItPattern(bytes, hdr, 0)) {
        * = pat.rows;
    }
    const smp = parseItSample(bytes, hdr, 0) ?? return 0;
    return smp.c5_speed;
}
```
