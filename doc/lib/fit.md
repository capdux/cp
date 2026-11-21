# `cap:fit` — 纯 CP Garmin / ANT+ FIT 运动遥测二进制文件零分配解析库

## 概述

`cap:fit` 提供纯 CP、零外部依赖、零堆分配的 ANT+ FIT Protocol 1.0 / 2.0 二进制遥测文件解析器：

- **FIT 专用 CRC-16**：内置 Garmin 官方半字节查表算法（多项式 `0xA001`），完整支持 14B 头部 CRC 与文件尾 2B CRC 校验；
- **12/14 字节文件头解析**：校验魔数 `".FIT"`、协议/配置文件版本、数据区长度边界及可选 14B 头 CRC；
- **Definition / Data Message 零分配流式解析**：维护 16 个 Local Message Type 槽位，支持 LE/BE 双端序、FIT 2.0 Developer Data Fields（`0x20`）扩展跳过及 Compressed Timestamp Data Message（`0x80`）解码；
- **遥测统计汇总**：`summarizeFit` 一键统计 Definition/Data 帧数、`record`（20）与 `file_id`（0）数量及全文件 CRC 状态。

## 核心 API

| 符号 | 说明 |
| :--- | :--- |
| `FIT_MAGIC` / `FIT_HEADER_SIZE_12` / `FIT_HEADER_SIZE_14` / `FIT_MAX_LOCAL_MESGS` / `FIT_MAX_FIELDS` | FIT 协议魔数与边界常量 |
| `FIT_MESG_FILE_ID` (`0`), `FIT_MESG_SESSION` (`18`), `FIT_MESG_LAP` (`19`), `FIT_MESG_RECORD` (`20`), `FIT_MESG_EVENT` (`21`), `FIT_MESG_DEVICE_INFO` (`23`), `FIT_MESG_ACTIVITY` (`34`) | 常用 Global Message Number 常量 |
| `fitCrc16(data): u16` | Garmin 官方 nibble 查表 CRC-16（多项式 `0xA001`） |
| `isFit(data)` / `FitHeader` / `parseFitHeader(data): FitHeader?` | 校验并解析 12/14 字节 FIT 文件头 |
| `verifyFileCrc(data): bool` | 校验文件尾 2B LE CRC-16 |
| `FitFieldDef` / `FitRecordKind` / `FitRecord` / `FitRecordIter` | Definition / Normal Data / Compressed Timestamp Data 零分配帧流迭代器 |
| `FitStats` / `summarizeFit(data): FitStats` | 统计 `definition_count`, `data_count`, `record_mesg_count`, `file_id_count`, `crc_valid` |

## 示例

```cp
import { parseFitHeader, verifyFileCrc, FitRecordIter, FitRecordKind, summarizeFit } from "cap:fit";

fn inspectFit(bytes: []const u8): usize {
    if (parseFitHeader(bytes) == null || !verifyFileCrc(bytes)) return 0;

    const stats = summarizeFit(bytes);
    * = stats.record_mesg_count;

    let it = FitRecordIter.init(bytes) ?? return 0;
    let total_payload: usize = 0;
    while (rec := it.next()) {
        if (rec.kind == FitRecordKind.data) {
            total_payload += rec.payload.len;
        }
    }
    return total_payload;
}
```
