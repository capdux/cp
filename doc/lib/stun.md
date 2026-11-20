# `cap:stun` — 纯 CP RFC 5389 / RFC 8489 STUN 线协议编解码与完整性校验库

> **模块路径**: `lib/stun/`  
> **导入协议**: `import { StunBuilder, StunMessage, StunClass, StunMethod, AttrType, StunAddr, XorMappedAddress, StunErrorCode, StunError, parseMessage, verifyMessageIntegrity, verifyFingerprint, decodeXorMappedAddress, decodeErrorCode, MAGIC_COOKIE } from "cap:stun";`  
> **动态挂载**: `import cap from "cap";` -> `cap.stun.*` 编译期按需惰性挂载

---

## 一、 架构概述

`cap:stun` 是 100% 纯 CP 实现、零禁用外部依赖（无 `std.posix` / `@import("bun")`）的 RFC 5389 / RFC 8489 STUN（Session Traversal Utilities for NAT）线协议编解码与完整性校验库：

1. **标准 STUN 头部与属性构造 (`StunBuilder`)**：
   - 严格按 RFC 5389 Section 6 交错编码 14-bit Message Type（2-bit `StunClass` + 12-bit `method`），前 2 位恒为 `00`，Magic Cookie 固定为 `0x2112A442`；
   - `addAttribute` 自动完成 4 字节对齐与零字节填充；
   - `addXorMappedAddress` 支持 IPv4（XOR `MAGIC_COOKIE`）与 IPv6（XOR `MAGIC_COOKIE || transaction_id`）及端口（XOR `MAGIC_COOKIE >> 16`）；
   - `addErrorCode` 支持 `300..699` 状态码与 UTF-8 reason 短语编码；
   - `finishWithIntegrity` 按 RFC 5389 Section 15.4 / 15.5 规范顺序生成 `MESSAGE-INTEGRITY`（HMAC-SHA1）与 `FINGERPRINT`（IEEE CRC-32 `^ 0x5354554e`）。
2. **安全解析与验签 (`parseMessage`, `verifyMessageIntegrity`, `verifyFingerprint`)**：
   - 严格拦截前 2 位非 `00`（`InvalidHeader`）、非法 Magic Cookie（`InvalidMagicCookie`）、长度非 4 的倍数或与负载不匹配（`InvalidMessageLength`）及属性截断越界（`TruncatedAttribute`）；
   - `verifyMessageIntegrity` 使用流式 HMAC-SHA1 与常量时间比对，避免额外堆分配与时序侧信道；
   - `verifyFingerprint` 校验 `FINGERPRINT` 必须为末尾属性并验证 CRC-32 `^ 0x5354554e`。

---

## 二、 核心 API 速查表 (`lib/stun/stun.cp`)

| 符号 / 方法 | 签名与说明 |
| :--- | :--- |
| `MAGIC_COOKIE` | `u32 = 0x2112A442` |
| `FINGERPRINT_XOR` | `u32 = 0x5354554e` |
| `StunError` | `InvalidHeader`, `InvalidMagicCookie`, `InvalidMessageLength`, `TruncatedAttribute`, `InvalidAttributeValue`, `IntegrityMismatch`, `FingerprintMismatch`, `OutOfMemory` |
| `StunClass` | `Request = 0`, `Indication = 1`, `SuccessResponse = 2`, `ErrorResponse = 3` |
| `StunMethod` | `Binding = 0x0001`, `Allocate = 0x0003`, `Refresh = 0x0004`, `Send = 0x0006`, `Data = 0x0007`, `CreatePermission = 0x0008`, `ChannelBind = 0x0009` |
| `AttrType` | `MappedAddress = 0x0001`, `Username = 0x0006`, `MessageIntegrity = 0x0008`, `ErrorCode = 0x0009`, `UnknownAttributes = 0x000A`, `Realm = 0x0014`, `Nonce = 0x0015`, `XorMappedAddress = 0x0020`, `Software = 0x8022`, `AlternateServer = 0x8023`, `Fingerprint = 0x8028` |
| `StunAddr` | tagged union：`ipv4: [4]u8`, `ipv6: [16]u8` |
| `XorMappedAddress` | `{ addr: StunAddr, port: u16 }` |
| `StunErrorCode` | `{ code: u16, reason: []const u8 }` |
| `StunAttribute` | `{ attr_type: u16, value: []const u8 }` |
| `StunMessage` | `{ msg_class: StunClass, method: u16, transaction_id: [12]u8, attributes: []StunAttribute }`；提供 `findAttr(attr_type)` 与 `deinit(*_, allocator)` |
| `StunBuilder.init(alloc, msg_class, method, tid)` | 初始化 STUN 消息构建器 |
| `StunBuilder.deinit(*_)` | 释放构建器内部缓冲 |
| `StunBuilder.addAttribute(*_, attr_type, value)` | 添加任意属性并自动 4 字节补零对齐 |
| `StunBuilder.addXorMappedAddress(*_, addr, port)` | 添加 `XOR-MAPPED-ADDRESS`（IPv4 / IPv6） |
| `StunBuilder.addErrorCode(*_, code, reason)` | 添加 `ERROR-CODE`（300..699） |
| `StunBuilder.finish(*_, alloc)` | 生成完整 STUN 字节切片 |
| `StunBuilder.finishWithIntegrity(*_, alloc, key, add_fp)` | 生成带 `MESSAGE-INTEGRITY` 与可选 `FINGERPRINT` 的 STUN 字节切片 |
| `parseMessage(alloc, bytes)` | 解析并校验 STUN 线格式消息 |
| `verifyMessageIntegrity(bytes, key)` | 校验 `MESSAGE-INTEGRITY`（HMAC-SHA1） |
| `verifyFingerprint(bytes)` | 校验末尾 `FINGERPRINT`（CRC-32 `^ 0x5354554e`） |
| `decodeXorMappedAddress(attr_val, tid)` | 解码 `XOR-MAPPED-ADDRESS` |
| `decodeErrorCode(attr_val)` | 解码 `ERROR-CODE` |

---

## 三、 使用示例

```typescript
const std = @import("std");
import {
    StunBuilder,
    StunClass,
    StunMethod,
    AttrType,
    StunAddr,
    parseMessage,
    verifyMessageIntegrity,
    verifyFingerprint,
    decodeXorMappedAddress,
} from "cap:stun";

fn demo(alloc: std.mem.Allocator): !void {
    const tid: [12]u8 = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12];
    let builder = StunBuilder.init(alloc, StunClass.SuccessResponse, StunMethod.Binding as u16, tid);
    defer builder.deinit();

    builder.addXorMappedAddress(StunAddr{ ipv4: [203, 0, 113, 42] }, 54321)?;
    const wire = builder.finishWithIntegrity(alloc, "secret-ice-pwd", true)?;
    defer alloc.free(wire);

    * = verifyMessageIntegrity(wire, "secret-ice-pwd")?;
    * = verifyFingerprint(wire)?;

    let msg = parseMessage(alloc, wire)?;
    defer msg.deinit(alloc);

    if (msg.findAttr(AttrType.XorMappedAddress as u16)) |attr| {
        const mapped = decodeXorMappedAddress(attr.value, msg.transaction_id)?;
        _ = mapped.port;
    }
}
```
