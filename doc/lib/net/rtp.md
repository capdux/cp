# `cap:rtp` — 纯 CP RFC 3550 RTP/RTCP 报文编解码与序列号抖动缓冲库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:net`，导入请使用 `import { ... } from "cap:net"`。

> **模块路径**: `lib/rtp/`  
> **导入协议**: `import { RtpPacket, RtpExtension, RtcpSenderReport, RtcpBye, RtcpType, JitterBuffer, JitterItem, JitterStats, RtpError, seqDiff } from "cap:net";`  
> **动态挂载**: `import cap from "cap";` -> `cap.rtp.*` 编译期按需惰性挂载

---

## 一、 架构概述

`cap:rtp` 是 100% 纯 CP 实现、零禁用外部依赖（无 `std.posix` / `@import("bun")`）的 RFC 3550 实时传输协议（RTP/RTCP）编解码与乱序重排缓冲库：

1. **RTP v2 报文编解码 (`RtpPacket`, `RtpExtension`)**：
   - 完整支持 12 字节固定头（`V=2`, `P`, `X`, `CC`, `M`, `PT`, `sequence_number`, `timestamp`, `ssrc`）；
   - 支持最多 15 个 32-bit CSRC 贡献源列表、RFC 3550 Section 5.3.1 扩展头（`profile` + 32-bit 字长度）及尾部 Padding 自动对齐/剥离；
   - `decode()` 严格拦截非 `V=2`（`InvalidVersion`）、头/CSRC/扩展头截断（`TruncatedPacket`）以及 `padding_len == 0` 或越界（`InvalidPadding`）；`extension.data` 与 `payload` 为零拷贝切片，`deinit(allocator)` 释放解码分配的 `csrcs`。
2. **RTCP 控制报文编解码 (`RtcpSenderReport`, `RtcpBye`, `RtcpType`)**：
   - 支持 `PT=200`（Sender Report）与 `PT=203`（BYE，含多 SSRC 列表与可选 UTF-8 reason 及 32-bit 字边界补零）；
   - 按 RFC 3550 规范编码与校验 `(len_words + 1) * 4` 字节总长度与版本号/负载类型。
3. **16-bit 回绕序列号抖动缓冲器 (`JitterBuffer`, `seqDiff`)**：
   - 基于 `seqDiff(a, b)` 模 `65536` 有符号差值算术，支持 `65534 -> 65535 -> 0 -> 1` 跨边界乱序重排；
   - 自动去重相同序列号（`duplicates`）、丢弃迟到过期包（`late_drops`）、支持容量上限拦截（`BufferFull`）及 `skipMissing()` 跳过丢包并累计 `lost_packets`。

---

## 二、 核心 API 速查表 (`lib/rtp/rtp.cp`)

| 符号 / 方法 | 签名与说明 |
| :--- | :--- |
| `RTP_HEADER_SIZE` | `usize = 12` |
| `RTP_VERSION` | `u8 = 2` |
| `RtpError` | `InvalidVersion`, `TruncatedPacket`, `InvalidPadding`, `InvalidPayloadType`, `BufferFull`, `OutOfMemory` |
| `RtpExtension` | `{ profile: u16, data: []const u8 }` |
| `RtpPacket` | `{ version: u8, padding: bool, marker: bool, payload_type: u7, sequence_number: u16, timestamp: u32, ssrc: u32, csrcs: []const u32, extension: RtpExtension?, payload: []const u8 }` |
| `RtpPacket.encode(alloc)` | 编码为大端序 RTP 字节切片（`([]u8) ! RtpError`） |
| `RtpPacket.decode(alloc, bytes)` | 解码并严格校验 RTP 报文（`RtpPacket ! RtpError`） |
| `RtpPacket.deinit(*_, alloc)` | 释放 `decode()` 分配的 `csrcs` 切片 |
| `RtcpType` | `SenderReport = 200`, `ReceiverReport = 201`, `Sdes = 202`, `Bye = 203` |
| `RtcpSenderReport` | `{ ssrc: u32, ntp_msw: u32, ntp_lsw: u32, rtp_timestamp: u32, packet_count: u32, octet_count: u32 }`；提供 `encode(alloc)` / `decode(bytes)` |
| `RtcpBye` | `{ ssrc_list: []u32, reason: ([]const u8)? }`；提供 `encode(alloc)` / `decode(alloc, bytes)` / `deinit(*_, alloc)` |
| `seqDiff(a, b)` | 16-bit 模 `65536` 有符号序列号差值（`i32`） |
| `JitterItem` | `{ seq: u16, timestamp: u32, payload: []const u8 }`；提供 `deinit(*_, alloc)` |
| `JitterStats` | `{ buffered: usize, emitted: u64, duplicates: u64, late_drops: u64, lost_packets: u64 }` |
| `JitterBuffer.init(alloc, max_cap)` | 初始化抖动缓冲器 |
| `JitterBuffer.deinit(*_)` | 释放所有未出队帧及内部存储 |
| `JitterBuffer.push(*_, seq, ts, payload)` | 入队数据包（自动去重与过旧丢弃，`void ! RtpError`） |
| `JitterBuffer.popNext(*_)` | 按序弹出期望序列号对应的 `JitterItem?` |
| `JitterBuffer.skipMissing(*_)` | 跳过丢失序列号并推进至下一个已缓冲帧，更新 `lost_packets` |
| `JitterBuffer.stats()` | 返回当前 `JitterStats` 统计快照 |

---

## 三、 使用示例

```typescript
const std = @import("std");
import { RtpPacket, RtpExtension, JitterBuffer, seqDiff } from "cap:net";

fn demo(alloc: std.mem.Allocator): !void {
    const ext_bytes: [4]u8 = [0x11, 0x22, 0x33, 0x44];
    const pkt = RtpPacket {
        version: 2,
        padding: false,
        marker: true,
        payload_type: 96,
        sequence_number: 65535,
        timestamp: 48000,
        ssrc: 0x12345678,
        extension: RtpExtension { profile: 0xBEDE, data: &ext_bytes },
        payload: "opus-frame",
    };

    const wire = pkt.encode(alloc)?;
    defer alloc.free(wire);

    let decoded = RtpPacket.decode(alloc, wire)?;
    defer decoded.deinit(alloc);

    let jb = JitterBuffer.init(alloc, 16);
    defer jb.deinit();
    jb.push(0, 48960, "frame-0")?;
    jb.push(65535, 48000, "frame-65535")?;

    if (item := jb.popNext()) {
        let owned = item;
        defer owned.deinit(alloc);
        _ = seqDiff(0, 65535);
    }
}
```
