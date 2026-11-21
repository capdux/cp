# `cap:edf` — 零分配 EDF / EDF+ 生物医学信号解析标准库

`cap:edf` 是纯 CP、零堆分配的 European Data Format（EDF / EDF+）解析库，面向脑电（EEG）、心电（ECG）、肌电（EMG）与多导睡眠图（PSG）等生物医学时间序列信号，提供 256 字节定宽 ASCII 主头解析、按字段列式排布的通道头解析、采样率与线性物理量换算，以及 16-bit 小端交错数据记录的安全随机采样读取。

---

## 快速导入

```typescript
import {
    EDF_MAIN_HEADER_SIZE,
    EDF_SIGNAL_HEADER_SIZE,
    EdfHeader,
    EdfSignal,
    isEdf,
    parseEdfHeader,
    parseEdfSignal,
    edfRecordByteSize,
    readEdfDigitalSample,
} from "cap:edf";
```

---

## 常量

| 常量 | 类型 | 值 | 说明 |
| :--- | :--- | :--- | :--- |
| `EDF_MAIN_HEADER_SIZE` | `usize` | `256` | EDF 固定主头字节数 |
| `EDF_SIGNAL_HEADER_SIZE` | `usize` | `256` | 每个信号通道的固定头字节数（列式分布） |

---

## 核心数据结构

### `EdfHeader`

256 字节定宽 ASCII 主头的零拷贝视图（文本字段自动去除尾部空格）：

- `version: []const u8`（字节 `0..8`，合法值必须为 `"0       "`）
- `patient_id: []const u8`（字节 `8..88`）
- `recording_id: []const u8`（字节 `88..168`）
- `start_date: []const u8`（字节 `168..176`，`dd.mm.yy`）
- `start_time: []const u8`（字节 `176..184`，`hh.mm.ss`）
- `header_bytes: u32`（字节 `184..192`，必须等于 `256 * (1 + num_signals)`）
- `reserved: []const u8`（字节 `192..236`，EDF+ 为 `"EDF+C"` 连续或 `"EDF+D"` 间断）
- `num_records: i64`（字节 `236..244`，`-1` 表示实时流/未知记录数）
- `record_duration_sec: f64`（字节 `244..252`，单条数据记录时长，单位：秒）
- `num_signals: u32`（字节 `252..256`，信号通道总数）

只读方法：
- `isEdfPlus(): bool` — 判断 `reserved` 是否以 `"EDF+C"` 或 `"EDF+D"` 开头。
- `totalDurationSec(): f64` — 返回 `num_records * record_duration_sec`；当 `num_records < 0` 时返回 `-1.0`。

### `EdfSignal`

单个信号通道的列式元数据零拷贝视图：

- `label: []const u8`（16B/ch，如 `"EEG Fp1-Cz"`）
- `transducer: []const u8`（80B/ch，传感器类型）
- `physical_dim: []const u8`（8B/ch，物理单位如 `"uV"` / `"mV"`）
- `physical_min: f64`（8B/ch）
- `physical_max: f64`（8B/ch）
- `digital_min: i32`（8B/ch）
- `digital_max: i32`（8B/ch）
- `prefiltering: []const u8`（80B/ch，硬件/软件预滤波说明）
- `samples_per_record: u32`（8B/ch，每条数据记录中该通道的采样点数）

只读方法：
- `sampleRateHz(header: EdfHeader): f64` — 返回 `samples_per_record / header.record_duration_sec`；当 `record_duration_sec <= 0.0` 时安全返回 `0.0`。
- `digitalToPhysical(raw: i16): f64` — 按 EDF 标准线性定标公式将 16 位数字采样换算为物理量：
  `physical_min + (raw - digital_min) * (physical_max - physical_min) / (digital_max - digital_min)`；
  当 `digital_max == digital_min` 时防除零返回 `physical_min`。

---

## 核心函数

| 函数签名 | 说明 |
| :--- | :--- |
| `isEdf(data: []const u8): bool` | 校验 `data.len >= 256`、前 8 字节为 `"0       "` 且 `header_bytes == 256 * (1 + num_signals)` |
| `parseEdfHeader(data: []const u8): EdfHeader?` | 零分配解析 256B 定宽主头，格式非法返回 `null` |
| `parseEdfSignal(data: []const u8, header: EdfHeader, sig_idx: u32): EdfSignal?` | 按 EDF 列式布局零分配解析第 `sig_idx` 个通道头 |
| `edfRecordByteSize(data: []const u8, header: EdfHeader): usize?` | 计算单条数据记录总字节数（`sum(samples_per_record) * 2`） |
| `readEdfDigitalSample(data: []const u8, header: EdfHeader, record_idx: usize, sig_idx: u32, sample_idx: usize): i16?` | 从 16-bit 小端交错数据记录中精确读取指定样本并防越界 |

---

## 使用示例

```typescript
import { isEdf, parseEdfHeader, parseEdfSignal, readEdfDigitalSample } from "cap:edf";

fn inspectFirstSample(buf: []const u8): f64? {
    if (!isEdf(buf)) {
        return null;
    }
    const hdr = parseEdfHeader(buf) ?? return null;
    const sig0 = parseEdfSignal(buf, hdr, 0) ?? return null;
    const raw = readEdfDigitalSample(buf, hdr, 0, 0, 0) ?? return null;
    return sig0.digitalToPhysical(raw);
}
```
