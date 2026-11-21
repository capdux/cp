# `cap:ntp` — RFC 5905 NTPv4 / SNTP 零分配编解码与时钟偏移库

纯 CP、零堆分配的网络时间协议（NTPv4 / SNTP）48 字节报文编解码、Unix <-> NTP 64-bit 时间戳双向换算与 RFC 5905 时钟偏移/RTT 计算库。

## 导入

```cp
import {
    NTP_UDP_PORT,
    NTP_PACKET_SIZE,
    NTP_V3,
    NTP_V4,
    NTP_UNIX_EPOCH_DELTA,
    LI_NO_WARNING,
    LI_61_SEC,
    LI_59_SEC,
    LI_ALARM,
    MODE_RESERVED,
    MODE_SYMMETRIC_ACTIVE,
    MODE_SYMMETRIC_PASSIVE,
    MODE_CLIENT,
    MODE_SERVER,
    MODE_BROADCAST,
    MODE_CONTROL,
    MODE_PRIVATE,
    NtpTimestamp,
    NtpPacket,
    isNtp,
    parseNtp,
    encodeNtp,
    makeClientRequest,
    calcOffsetNs,
    calcDelayNs,
} from "cap:ntp";
```

## 核心 API

- **时间戳 (`NtpTimestamp`)**：`fromUnixMs(ms: i64)`、`toUnixMs(): i64`、`toUnixNs(): i128`、`isZero()`、`zero()`。
- **报文 (`NtpPacket`)**：`isKissOfDeath()`（`stratum == 0`）、`isPrimary()`（`stratum == 1`）、`rootDelayMs()`、`rootDispersionMs()`。
- **编解码**：`isNtp(data)`、`parseNtp(data): NtpPacket?`、`encodeNtp(pkt, out: []u8): usize?`、`makeClientRequest(tx_ts): NtpPacket`。
- **RFC 5905 时钟算法**：
  - `calcOffsetNs(t1, t2, t3, t4): i128` — `((T2 - T1) + (T3 - T4)) / 2`
  - `calcDelayNs(t1, t2, t3, t4): i128` — `(T4 - T1) - (T3 - T2)`

## 示例

```cp
import std from "std";
import { NtpTimestamp, makeClientRequest, encodeNtp, parseNtp } from "cap:ntp";

fn example() {
    const req = makeClientRequest(NtpTimestamp.fromUnixMs(1_700_000_000_000));
    let buf: [48]u8 = std.mem.zeroes([48]u8);
    if (n := encodeNtp(req, buf[0..])) {
        if (pkt := parseNtp(buf[0..n])) {
            * = pkt.tx_ts.toUnixMs();
        }
    }
}
```
