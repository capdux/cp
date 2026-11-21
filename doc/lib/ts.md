# `cap:ts` — 纯 CP ISO/IEC 13818-1 MPEG-TS 传输流零分配解析库

## 概述

`cap:ts` 提供纯 CP、零外部依赖、零堆分配的 ISO/IEC 13818-1 MPEG-TS（188B）与 M2TS（192B）传输流解析器，完整支持：
- **流探测与分组迭代**：`isMpegTs` 校验同步字节 `0x47` 与双包步长（兼容 188B TS / 192B M2TS），`TsPacketIter` 逐包零拷贝迭代；
- **TS 头与 Adaptation Field / PCR**：解析 13-bit `pid`、连续计数器、Adaptation Field 标志位（`discontinuity`、`random_access`）以及 33-bit 90kHz `pcr_base`（含 `pcr_ms()` 换算）；
- **PSI 表解析（PAT & PMT）**：自动跳过 `payload_unit_start` 时的 1B `pointer_field`，通过 `PatIter` 提取节目号到 PMT PID 的映射，通过 `parsePmt` + `PmtStreamIter` 提取 `pcr_pid` 与所有基本流（H.264 / HEVC / AAC / AC-3 等）PID。

## 核心 API

| 符号 | 说明 |
| :--- | :--- |
| `TS_PACKET_SIZE`, `M2TS_PACKET_SIZE`, `TS_SYNC_BYTE`, `TS_PID_PAT`, `TS_PID_CAT`, `TS_PID_NULL` | 分组长度、同步字节与保留 PID 常量 |
| `STREAM_TYPE_MPEG2_VIDEO`, `STREAM_TYPE_AAC_ADTS`, `STREAM_TYPE_H264`, `STREAM_TYPE_HEVC`, `STREAM_TYPE_AC3` | 常见音视频编码流类型常量 |
| `isMpegTs(data)` | 探测 188B TS 或 192B M2TS 传输流 |
| `TsPacket` / `parseTsPacket(pkt)` / `TsPacketIter` | 解码 188B 分组头、Adaptation Field、PCR（`pcr_ms()`）与零拷贝 `payload` |
| `PatEntry` / `PatIter` | 零拷贝迭代 PAT 表（`table_id == 0x00`）全部节目条目 |
| `PmtStream` / `PmtHeader` / `parsePmt(payload)` / `PmtStreamIter` | 解析 PMT 表（`table_id == 0x02`）并零拷贝迭代全部基本流 |

## 示例

```cp
import { TS_PID_PAT, TsPacketIter, PatIter, parsePmt } from "cap:ts";

fn findFirstPmtPid(stream: []const u8): u16 {
    let it = TsPacketIter.init(stream);
    while (pkt := it.next()) {
        if (pkt.pid == TS_PID_PAT && pkt.payload_unit_start) {
            let pat = PatIter.init(pkt.payload);
            while (entry := pat.next()) {
                if (entry.program_number != 0) return entry.pmt_pid;
            }
        }
    }
    return 0;
}
```
