# `cap:xm` — 纯 CP FastTracker II Extended Module (`.xm`) 跟踪器音乐零分配解析库

## 概述

`cap:xm` 提供纯 CP、零外部依赖、零堆分配的 FastTracker II Extended Module（`.xm` v1.04 小端二进制规范）解析器：

- **文件头解析**：校验 `"Extended Module: "`（0..17）与 `0x1A`（偏移 37），解析模块名/制作器名（自动去尾 `\0` 与空格）、版本号（`versionMajor()` / `versionMinor()`）、通道数（`1..32`）、Linear/Amiga 频率表标志（`isLinearFreqTable()`）、默认 Tempo/BPM 与最多 256B Pattern Order 表；
- **Pattern 块零分配迭代**：`XmPatternIter` 依次解析全部 Pattern（支持空压缩 Pattern `packed_data_size == 0` 与带负载 Pattern 零拷贝切片），并在迭代结束后提供精确的 `instruments_offset`；
- **Instrument 块零分配迭代**：`XmInstrumentIter` 支持 `num_samples == 0` 与 `num_samples > 0` 乐器，自动按 `sample_header_size` 跳过全部 Sample 头与 PCM 数据区，精准推进至下一个 Instrument。

## 核心 API

| 符号 | 说明 |
| :--- | :--- |
| `XM_MAGIC` / `XM_SUB_BYTE` (`0x1A`) / `XM_MIN_HEADER_SIZE` (`80`) / `XM_MAX_CHANNELS` (`32`) | XM 规范魔数与边界常量 |
| `isXm(data): bool` | 校验最小长度、魔数 `"Extended Module: "` 与偏移 37 的 `0x1A` |
| `XmHeader` / `parseXmHeader(data): XmHeader?` | 解析 XM 头部全字段、Pattern Order 表及 `patterns_offset`；附 `versionMajor()`, `versionMinor()`, `isLinearFreqTable()` |
| `XmPattern` / `XmPatternIter` | 零分配迭代 `num_patterns` 个 Pattern，提供 `instruments_offset` 与 `had_error` |
| `XmInstrument` / `XmInstrumentIter` | 零分配迭代 `num_instruments` 个 Instrument（自动跳过 Sample 头与 PCM 区），提供 `total_sample_bytes` 与 `had_error` |

## 示例

```cp
import { parseXmHeader, XmPatternIter, XmInstrumentIter } from "cap:xm";

fn inspectXm(bytes: []const u8): usize {
    const hdr = parseXmHeader(bytes) ?? return 0;
    * = hdr.module_name;

    let pit = XmPatternIter.init(bytes, hdr);
    while (pat := pit.next()) {
        * = pat.num_rows;
    }
    if (pit.had_error) return 0;

    let iit = XmInstrumentIter.init(bytes, pit.instruments_offset, hdr.num_instruments);
    let total_pcm: usize = 0;
    while (inst := iit.next()) {
        total_pcm += inst.total_sample_bytes;
    }
    return total_pcm;
}
```
