# `cap:segy` — 零分配 SEG-Y Rev 1/2 地球物理地震反射解析标准库

`cap:segy` 是纯 CP、零堆分配的 SEG-Y Rev 1 / Rev 2 地震反射数据解析库，支持 3200B 文本卷头、400B 二进制卷头、扩展文本头偏移计算、240B 地震道头零分配迭代，以及 IBM S/360 32-bit 十六进制浮点到 IEEE-754 `f64` 的精确转换。

---

## 快速导入

```typescript
import {
    SEGY_TEXT_HEADER_SIZE,
    SEGY_BINARY_HEADER_SIZE,
    SEGY_TRACE_HEADER_SIZE,
    SEGY_MIN_HEADER_SIZE,
    SegySampleFormat,
    segySampleBytes,
    SegyBinaryHeader,
    SegyTraceHeader,
    SegyTrace,
    SegyTraceIter,
    ibmFloatToF64,
    parseSegyHeader,
    readSampleF64,
    applyCoordScalar,
} from "cap:segy";
```

---

## 常量与枚举

| 符号 | 值 / 成员 | 说明 |
| :--- | :--- | :--- |
| `SEGY_TEXT_HEADER_SIZE` | `3200` | 标准文本卷头与单扩展文本头字节数 |
| `SEGY_BINARY_HEADER_SIZE` | `400` | 二进制卷头字节数（偏移 `3200..3600`） |
| `SEGY_TRACE_HEADER_SIZE` | `240` | 单道地震道头字节数 |
| `SEGY_MIN_HEADER_SIZE` | `3600` | 最小卷头长度（`3200 + 400`） |
| `SegySampleFormat: u16` | `unknown=0`, `ibm_f32=1`, `int32=2`, `int16=3`, `ieee_f32=5`, `ieee_f64=6`, `int8=8` | 采样格式编码 |

---

## 核心数据结构

### `SegyBinaryHeader`
- `job_id: i32` (`3200`), `line_number: i32` (`3204`), `reel_number: i32` (`3208`)
- `traces_per_ensemble: i16` (`3212`), `sample_interval_us: u16` (`3216`), `samples_per_trace: u16` (`3220`)
- `sample_format: SegySampleFormat` (`3224`), `measurement_system: i16` (`3254`: `1`=meters, `2`=feet)
- `format_rev: u16` (`3500`: `0x0100`=Rev1, `0x0200`=Rev2), `fixed_length_flag: i16` (`3502`), `num_ext_text_headers: i16` (`3504`)
- `traces_offset: usize`：`3600 + max(0, num_ext_text_headers) * 3200`
- `is_little_endian: bool`：自动识别 Rev 2 小端标记（偏移 `3296..3300` 为 `[4, 3, 2, 1]`）

### `SegyTraceHeader` & `SegyTrace` & `SegyTraceIter`
- `SegyTraceHeader` 包含 `trace_seq_line`, `trace_seq_file`, `field_record`, `cdp`, `trace_id_code`, `coord_scalar`, `source_x`, `source_y`, `group_x`, `group_y`, `trace_samples`, `trace_interval_us`。
- `SegyTrace` 包含 `header: SegyTraceHeader`、`raw_samples: []const u8`（零拷贝切片）与 `is_little_endian: bool`。
- `SegyTraceIter.init(data, header)` 从 `header.traces_offset` 开始顺序迭代，调用 `iter.next()` 零分配产出每条 `SegyTrace`；遇截断或非法格式置 `had_error = true` 并返回 `null`。

---

## 核心函数

| 函数签名 | 说明 |
| :--- | :--- |
| `segySampleBytes(fmt: SegySampleFormat): usize?` | 返回单样本字节数（`unknown` 返回 `null`） |
| `ibmFloatToF64(bits: u32): f64` | 精确解码 IBM S/360 32-bit 十六进制浮点为 `f64` |
| `parseSegyHeader(data: []const u8): SegyBinaryHeader?` | 解析 400B 二进制卷头并校验 `traces_offset <= data.len` |
| `readSampleF64(trace: SegyTrace, sample_format: SegySampleFormat, idx: usize): f64?` | 按采样格式统一解码指定样本为 `f64` 并防越界 |
| `applyCoordScalar(raw: i32, scalar: i16): f64` | SEG-Y 坐标标量换算（`>0` 乘、`<0` 除以 `-scalar`、`0` 原值） |

---

## 使用示例

```typescript
import { parseSegyHeader, SegyTraceIter, readSampleF64, applyCoordScalar } from "cap:segy";

fn inspectFirstTrace(data: []const u8): f64? {
    const hdr = parseSegyHeader(data) ?? return null;
    let it = SegyTraceIter.init(data, hdr);
    const trace = it.next() ?? return null;
    const sx = applyCoordScalar(trace.header.source_x, trace.header.coord_scalar);
    const s0 = readSampleF64(trace, hdr.sample_format, 0) ?? return null;
    return sx + s0;
}
```
