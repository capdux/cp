# `cap:base58` — Base58, Base58Check 与 BIP-173/350 Bech32/Bech32m 编解码库

> **模块路径**: `lib/base58/`  
> **导入协议**: `import { Base58, Bech32, Bech32Encoding, Bech32Decoded, Base58Error } from "cap:base58";`  
> **动态挂载**: `import cap from "cap";` -> `cap.base58.*` 编译期按需惰性挂载

---

## 一、 架构概述

`cap:base58` 是纯 CP 实现、零 C 外部依赖（无 `std.posix` / `@import("bun")`）的密码学地址与二进制文本编解码库，包含：

1. **Base58 & Base58Check (`Base58`)**：
   - 使用 Bitcoin 标准字母表 `123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz`（剔除易混淆字符 `0`, `O`, `I`, `l`）。
   - 严格保留前导零字节映射（每个前导 `0x00` 字节 <-> `'1'` 字符）与大数进位转换。
   - `encodeCheck` / `decodeCheck` 内置 4 字节 Double-SHA256（复用 `cap:crypto` 的 `sha256Raw`）校验和，常用于 Bitcoin P2PKH/P2SH、WIF 等格式。
2. **BIP-173 Bech32 & BIP-350 Bech32m (`Bech32`)**：
   - 支持 HRP（Human-Readable Part，ASCII `33..126`）与 5-bit 数据字编码，自动识别校验常数 `1`（`Bech32`）与 `0x2bc830a3`（`Bech32m`）。
   - 严格拦截大小写混用（`MixedCase`）、缺失分隔符 `'1'`（`MissingSeparator`）、非法 HRP（`InvalidHrp`）、长度越界（`< 8` 或 `> 90`，`InvalidLength`）、非法字符（`InvalidCharacter`）及校验失败（`InvalidChecksum`）。
   - 提供通用位宽转换工具 `Bech32.convertBits(allocator, data, from_bits, to_bits, pad)`，在 `pad == false` 时严格校验无多余或非零填充位（否则返回 `InvalidPadding`）。

> **重要区分**：`Bech32.decode` 返回的 `Bech32Decoded.data` 为 **5-bit 数据字切片**（每个字节取值 `0..31`，不含 6 字符校验和）；若需还原原始 8-bit 字节流（如 SegWit witness program），请对数据部分调用 `Bech32.convertBits(allocator, words5, 5, 8, false)`。

---

## 二、 核心 API 速查表 (`lib/base58/base58.cp`)

| 符号 / 方法 | 签名与说明 |
| :--- | :--- |
| `Base58Error` | `InvalidCharacter`, `InvalidChecksum`, `InvalidLength`, `InvalidHrp`, `MixedCase`, `MissingSeparator`, `InvalidPadding`, `OutOfMemory` |
| `Bech32Encoding` | 枚举（`u32`）：`Bech32 = 1`, `Bech32m = 0x2bc830a3` |
| `Bech32Decoded` | 结构体 `{ hrp: []u8, data: []u8, encoding: Bech32Encoding }`；`hrp` 已归一化为小写，`data` 为 5-bit 字数组；提供 `deinit(*_, allocator)` |
| `Base58.encode(allocator, bytes)` | 将字节切片编码为 Base58 字符串（调用方负责 `allocator.free`） |
| `Base58.decode(allocator, str)` | 将 Base58 字符串解码为原始字节（调用方负责 `allocator.free`） |
| `Base58.encodeCheck(allocator, version_and_payload)` | 附加 4 字节 Double-SHA256 校验和后进行 Base58 编码 |
| `Base58.decodeCheck(allocator, str)` | 解码 Base58Check 并验证 4 字节 Double-SHA256 校验和 |
| `Bech32.convertBits(allocator, data, from_bits, to_bits, pad)` | 通用位宽转换（`1..8` bit）；`pad = false` 时严格拒绝非法填充（`InvalidPadding`） |
| `Bech32.encode(allocator, hrp, data5, encoding)` | 编码 `hrp` 与 5-bit `data5` 为 Bech32 或 Bech32m 字符串 |
| `Bech32.decode(allocator, str)` | 解码 Bech32/Bech32m 字符串并自动识别 `Bech32Encoding` |

---

## 三、 使用示例

```typescript
const std = @import("std");
import { Base58, Bech32, Bech32Encoding } from "cap:base58";

fn demo(alloc: std.mem.Allocator): !void {
    // 1. Base58Check 编解码
    const payload = [_]u8{ 0x00, 0x01, 0x02, 0x03 };
    const addr = Base58.encodeCheck(alloc, &payload)?;
    defer alloc.free(addr);

    const decoded = Base58.decodeCheck(alloc, addr)?;
    defer alloc.free(decoded);

    // 2. Bech32m (BIP-350) 编解码（含 8-bit -> 5-bit 转换）
    const raw = "hello";
    const data5 = Bech32.convertBits(alloc, raw, 8, 5, true)?;
    defer alloc.free(data5);

    const b32m = Bech32.encode(alloc, "cp", data5, Bech32Encoding.Bech32m)?;
    defer alloc.free(b32m);

    let dec = Bech32.decode(alloc, b32m)?;
    defer dec.deinit(alloc);

    const restored = Bech32.convertBits(alloc, dec.data, 5, 8, false)?;
    defer alloc.free(restored);
}
```
