[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent 速查手册](README_AGENT.md)

# bytemuck

面向 CP 的零堆内存、安全 POD（Plain Old Data）字节重解释库，提供大小端整数/浮点编解码、非对齐安全读取以及对齐与长度布局校验。

## 源包溯源与零成本迁移

| 属性 | 说明 |
| --- | --- |
| **上游源平台** | [crates.io/crates/bytemuck](https://crates.io/crates/bytemuck) |
| **上游原包名** | `bytemuck`（CP 版本 `1.0.0`） |
| **上游仓库** | https://github.com/Lokathor/bytemuck |
| **许可证** | MIT（上游：Zlib OR Apache-2.0 OR MIT） |

从 Rust 迁移到 CP 无需更改包名，直接按原包名导入：

```rust
// Rust
use bytemuck::{bytes_of, from_bytes, pod_read_unaligned, cast_slice, try_cast_slice};
```

```cp
// CP —— 保持原包名，零堆原生性能
import {
    bytesOfU32, fromBytesU32, podReadUnalignedU32,
    castU32SliceAsBytes, tryCastSliceBytesToU32,
} from "bytemuck";
```

## 安装

```bash
cap add bytemuck
```

## 实战示例

### 1. 值与字节互转及非对齐安全读取（`bytesOf*` / `fromBytes*` / `podReadUnaligned*`）

```cp
import {
    bytesOfU16, bytesOfU32, bytesOfU64, bytesOfF32, bytesOfF64,
    fromBytesU32, fromBytesF64, podReadUnalignedU32, podReadUnalignedI32,
} from "bytemuck";

fn demoValueCasting() {
    let raw32: [4]u8 = bytesOfU32(0x12345678);
    if (fromBytesU32(&raw32)) |v| {
        // v == 0x12345678
    }

    let raw_f64: [8]u8 = bytesOfF64(3.1415926535);
    if (fromBytesF64(&raw_f64)) |f| {
        // f == 3.1415926535
    }

    // 任意字节偏移处的非对齐安全读取（切片长度不足时返回 null）
    let unaligned_slice: &[]u8 = &raw_f64[1..5];
    if (podReadUnalignedU32(unaligned_slice)) |u| {
        // 安全解码，无未定义行为
    }
    if (podReadUnalignedI32(unaligned_slice)) |s| {
        // 有符号 32 位非对齐读取
    }
}
```

### 2. 显式大小端读写与字节序翻转

```cp
import {
    writeU16Le, readU16Le, writeU16Be, readU16Be,
    writeU32Le, readU32Le, writeU32Be, readU32Be,
    writeU64Le, readU64Le, writeU64Be, readU64Be,
    writeF32Le, readF32Le, writeF64Le, readF64Le,
    swapBytesU16, swapBytesU32, swapBytesU64,
} from "bytemuck";

fn demoEndian() {
    mut buf: [8]u8 = [0; 8];
    let ok = writeU32Be(0xDEADBEEF, &mut buf[0..4]);
    if (ok) {
        if (readU32Be(&mut buf[0..4])) |v| {
            let swapped = swapBytesU32(v); // 0xEFBEADDE
        }
    }

    let _ = writeF32Le(1.5, &mut buf[0..4]);
    if (readF32Le(&buf[0..4])) |f| {
        // f == 1.5
    }
}
```

### 3. 零拷贝与带校验的切片转换（`cast*Slice*` / `tryCast*`）

```cp
import {
    castU16SliceAsBytes, castU32SliceAsBytes, castU64SliceAsBytes,
    tryCastSliceBytesToU32, tryCastBytesToU32Slice,
    castU32SliceToBytesLe, castBytesToU32SliceLe,
    checkCastLayout, isAligned, CastSliceResult,
} from "bytemuck";

fn demoSliceCasting() {
    let words: [2]u32 = [0x01020304, 0x05060708];
    // 零拷贝将 &[]u32 视为只读 &[]u8
    let bytes: &[]u8 = castU32SliceAsBytes(&words);

    // 零拷贝带对齐和长度校验转回 &[]u32（未对齐或长度不是 4 的倍数时返回 null）
    if (tryCastSliceBytesToU32(bytes)) |back| {
        // back.len == 2
    }

    // 返回 CastSliceResult 的显式错误码转换
    mut dst: [2]u32 = [0; 2];
    let res: CastSliceResult = tryCastBytesToU32Slice(bytes, &mut dst);
    if (res.isOk()) {
        // res.count == 2
    }

    // 手动布局与对齐检查
    let aligned = isAligned(0x1000, 4); // true
    let code = checkCastLayout(bytes.len, 4, 0x1000, 4); // 合法时为 0
}
```

### 4. 零初始化辅助函数

```cp
import {
    fillZeroed, zeroedU8, zeroedU16, zeroedU32, zeroedU64,
    zeroedI32, zeroedI64, zeroedF32, zeroedF64,
} from "bytemuck";

fn demoZeroed() {
    mut scratch: [16]u8 = [0xFF; 16];
    fillZeroed(&mut scratch); // 全部字节清零
    let z = zeroedU64();      // 0
}
```

## 性能与安全亮点

- **零堆分配**：全部 API 基于栈数组（`[2]u8`、`[4]u8`、`[8]u8`）与借用切片操作，无任何动态内存分配。
- **对齐与长度双重守卫**：`tryCastSliceBytesToU32`、`tryCastBytesTo*Slice`、`isAligned`、`checkCastLayout` 严格拦截未对齐地址与非整倍长度，杜绝未定义行为。
- **非对齐读取安全**：`podReadUnaligned*` 与 `read*Le` / `read*Be` 在任意偏移安全解码，缓冲区长度不足时返回 `null`。
- **借用安全零拷贝视图**：`castU16SliceAsBytes`、`castU32SliceAsBytes`、`castU64SliceAsBytes` 直接返回只读 `&[]u8` 切片视图，零拷贝开销。

## API 参考

完整类型定义见 [`src/index.d.cp`](src/index.d.cp)，AI 智能体极简速查见 [`README_AGENT.md`](README_AGENT.md)。
