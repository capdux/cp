# `cap:bitstream` — MSB/LSB 位流读写器与 Exp-Golomb / LEB128 编解码库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:binfmt`，导入请使用 `import { ... } from "cap:binfmt"`。

> **模块路径**: `lib/bitstream/`  
> **导入协议**: `import { BitWriter, BitReader, BitOrder, Leb128, BitStreamError } from "cap:binfmt";`  
> **动态挂载**: `import cap from "cap";` -> `cap.bitstream.*` 编译期按需惰性挂载

---

## 一、 架构概述

`cap:bitstream` 是纯 CP 实现、零 C 外部依赖（无 `std.posix` / `@import("bun")`）的位级与变长整数编解码引擎，提供三大核心能力：

1. **跨字节非对齐位流读写（`BitWriter` / `BitReader`）**：
   - 同时支持 `BitOrder.MsbFirst`（H.264/HEVC/JPEG/网络协议）与 `BitOrder.LsbFirst`（DEFLATE/WebP/Protobuf/硬件寄存器）两种位序。
   - 支持 `1..64` 位任意宽度读写（`writeBits` / `readBits`）、非破坏性前瞻（`peekBits`）、位跳过（`skipBits`）、字节对齐补零/丢弃（`alignToByte`）及对齐字节块写入（`writeAlignedBytes`）。
   - 严格校验位宽范围（`0` 或 `> 64` 返回 `BitStreamError.InvalidBitCount`）与流尾越界（返回 `BitStreamError.UnexpectedEof`）。
2. **H.264 / HEVC 0 阶指数哥伦布编码（`Exp-Golomb`）**：
   - 支持无符号 `ue(v)`（`writeExpGolombU` / `readExpGolombU`）与有符号 `se(v)`（`writeExpGolombS` / `readExpGolombS`，映射 `0, 1, -1, 2, -2, ...`，覆盖 `minInt(i32)` 与 `maxInt(i32)`）。
   - 解码时严格拦截超过 32 个前导零或超出 32 位表示范围的畸形码字（返回 `BitStreamError.InvalidExpGolomb`）。
3. **紧凑 LEB128 变长整数编解码（`Leb128`）**：
   - 提供 `encodeUleb128` / `decodeUleb128` 与带二补码符号扩展的 `encodeSleb128` / `decodeSleb128`。
   - 严格拦截超过 10 字节或超出 64 位整数范围的编码（`BitStreamError.IntegerOverflow`）以及截断输入（`BitStreamError.UnexpectedEof`）。

---

## 二、 核心 API 速查表 (`lib/bitstream/bitstream.cp`)

| 符号 / 方法 | 签名与说明 |
| :--- | :--- |
| `BitStreamError` | `UnexpectedEof`, `InvalidBitCount`, `IntegerOverflow`, `InvalidExpGolomb`, `OutOfMemory` |
| `BitOrder` | `MsbFirst`, `LsbFirst` |
| `BitWriter.init(allocator, order)` | 初始化指定位序的位流写入器 |
| `BitWriter.deinit()` | 释放内部缓冲内存 |
| `BitWriter.writeBit(bit: u1)` | 写入单个比特（`0` 或 `1`） |
| `BitWriter.writeBits(value: u64, num_bits: u7)` | 写入 `1..64` 位（越界返回 `InvalidBitCount`） |
| `BitWriter.writeExpGolombU(v: u32)` | 写入 H.264/HEVC 0 阶无符号指数哥伦布码 `ue(v)` |
| `BitWriter.writeExpGolombS(v: i32)` | 写入 H.264/HEVC 0 阶有符号指数哥伦布码 `se(v)` |
| `BitWriter.alignToByte()` | 当前未满字节补零并对齐到字节边界 |
| `BitWriter.writeAlignedBytes(data)` | 对齐到字节边界后直接追加字节切片 |
| `BitWriter.finish()` | 对齐到字节边界并移交所有权返回 `[]u8`（调用方负责 `allocator.free`） |
| `BitWriter.bytes()` | 返回当前已刷入缓冲区的完整字节只读视图 |
| `BitReader.init(bytes, order)` | 基于只读字节切片与位序初始化位流读取器 |
| `BitReader.readBit()` | 读取 1 比特，耗尽时返回 `UnexpectedEof` |
| `BitReader.readBits(num_bits: u7)` | 读取 `1..64` 位，越界返回 `InvalidBitCount` |
| `BitReader.peekBits(num_bits: u7)` | 非破坏性前瞻 `1..64` 位，不推进读取游标 |
| `BitReader.readExpGolombU()` | 解码 0 阶无符号指数哥伦布码 `ue(v)` |
| `BitReader.readExpGolombS()` | 解码 0 阶有符号指数哥伦布码 `se(v)` |
| `BitReader.alignToByte()` | 丢弃当前字节剩余比特并对齐到下一字节边界 |
| `BitReader.skipBits(n: usize)` | 跳过 `n` 比特，不足时返回 `UnexpectedEof` |
| `BitReader.bitsRemaining()` | 返回剩余可读比特数 |
| `Leb128.encodeUleb128(allocator, v: u64)` | 编码 `u64` 为 ULEB128 字节切片 |
| `Leb128.decodeUleb128(bytes)` | 解码 ULEB128，返回 `Uleb128Result { value: u64, bytes_read: usize }` |
| `Leb128.encodeSleb128(allocator, v: i64)` | 编码 `i64` 为 SLEB128 字节切片 |
| `Leb128.decodeSleb128(bytes)` | 解码 SLEB128（含符号扩展），返回 `Sleb128Result { value: i64, bytes_read: usize }` |

---

## 三、 使用示例

```typescript
const std = @import("std");
import { BitWriter, BitReader, BitOrder, Leb128 } from "cap:binfmt";

fn demo(alloc: std.mem.Allocator): !void {
    // 1. MSB 位流写入与非对齐读取
    let bw = BitWriter.init(alloc, BitOrder.MsbFirst);
    defer bw.deinit();

    bw.writeBits(0b101, 3)?;
    bw.writeExpGolombU(5)?;
    bw.writeExpGolombS(-2)?;
    bw.writeAlignedBytes("OK")?;

    const raw = bw.finish()?;
    defer alloc.free(raw);

    let br = BitReader.init(raw, BitOrder.MsbFirst);
    const hdr = br.readBits(3)?;          // 0b101
    const ue = br.readExpGolombU()?;      // 5
    const se = br.readExpGolombS()?;      // -2
    br.alignToByte();
    const c1 = br.readBits(8)?;           // 'O'
    const c2 = br.readBits(8)?;           // 'K'
    _ = hdr;
    _ = ue;
    _ = se;
    _ = c1;
    _ = c2;

    // 2. ULEB128 / SLEB128 编解码
    const enc = Leb128.encodeSleb128(alloc, -123456)?;
    defer alloc.free(enc);
    const decoded = Leb128.decodeSleb128(enc)?; // decoded.value == -123456
    _ = decoded;
}
```
