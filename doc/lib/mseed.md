# `cap:mseed` — 零分配 IRIS MiniSEED v2.4 地震学时间序列解析库

> **模块路径**：`lib/mseed/`（入口 `lib/mseed/index.cp`）  
> **特性**：纯 CP 实现、零堆分配（Zero-Alloc）、零 C 依赖，完整遵循 FDSN SEED v2.4 MiniSEED 规范：校验 48 字节固定头与序列号/质量指示符，通过 BTIME（`year` ∈ `1900..2100`、`day_of_year` ∈ `1..366`）自动识别大端/小端字节序，链式解析 Blockette 1000 / 1001，精确计算 SEED 采样率公式，并支持多记录拼接文件零拷贝流式迭代。

---

## 一、导入方式

```cp
import {
    MSEED_FIXED_HEADER_SIZE,
    MSEED_MIN_RECORD_SIZE,
    MSEED_BLK_1000,
    MSEED_BLK_1001,
    MseedEncoding,
    mseedEncodingFromU8,
    MseedBtime,
    MseedRecord,
    MseedStats,
    isMseedRecord,
    parseMseedRecord,
    MseedIter,
    summarizeMseed,
} from "cap:mseed";
```

---

## 二、核心常量与枚举

| 符号 | 值 / 成员 | 说明 |
| :--- | :--- | :--- |
| `MSEED_FIXED_HEADER_SIZE` | `48` | FDSN SEED v2.4 固定头部长度 |
| `MSEED_MIN_RECORD_SIZE` | `256` | 最小合法 MiniSEED 记录长度（`2^8`） |
| `MSEED_BLK_1000` / `MSEED_BLK_1001` | `1000` / `1001` | Blockette 1000（数据编码/记录长）与 1001（高精度时间扩展） |
| `MseedEncoding: u8` | `ascii=0`, `int16=1`, `int24=2`, `int32=3`, `ieee_f32=4`, `ieee_f64=5`, `steim1=10`, `steim2=11`, `unknown=255` | Blockette 1000 编码类型 |

---

## 三、核心数据结构与 API

| 类型 / 函数 | 字段 / 方法 | 说明 |
| :--- | :--- | :--- |
| `isMseedRecord(data)` | `(data: []const u8): bool` | 校验 `data.len >= 48`、6B ASCII 数字序列号、质量码 `'D'`/`'R'`/`'Q'`/`'M'` 及第 8 字节空格 |
| `MseedBtime` | `year`, `day_of_year`, `hour`, `minute`, `second`, `tenths_ms` | 10B SEED BTIME 时间戳 |
| `MseedRecord` | `seq_num`, `quality_indicator`, `station`, `location`, `channel`, `network`, `start_time`, `num_samples`, `sample_rate_factor`, `sample_rate_mult`, `encoding`, `word_order`, `data_rec_len_exp`, `record_bytes`, `timing_quality`, `microsecond`, `frame_count`, `payload`; `sampleRateHz(): f64` | 零拷贝 MiniSEED 记录视图，自动识别大/小端与计算采样率 |
| `parseMseedRecord(data, offset)` | `(data: []const u8, offset: usize): MseedRecord?` | 解析单条 MiniSEED 记录并提取 `payload` 零拷贝切片 |
| `MseedIter` | `MseedIter.init(data)` / `next(*_): MseedRecord?` / `hasError(): bool` | 按每条记录的 `record_bytes` 步进迭代拼接 `.mseed` 文件 |
| `MseedStats` / `summarizeMseed(data)` | `record_count`, `total_samples`, `min_record_bytes`, `max_record_bytes` | 单遍零分配统计多记录文件摘要 |

---

## 四、使用示例

```cp
import { MseedIter, summarizeMseed } from "cap:mseed";

fn inspectMseed(data: []const u8) {
    if (stats := summarizeMseed(data)) {
        * = stats.record_count;
        * = stats.total_samples;
    }

    let it = MseedIter.init(data);
    while (rec := it.next()) {
        * = rec.station;
        * = rec.sampleRateHz();
        * = rec.payload;
    }
}
```
