[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# fletcher

零堆分配的 Fletcher-16、Fletcher-32 与 Fletcher-64 位置敏感校验和库，同时提供一次性快捷函数与流式增量哈希器（`Fletcher16`、`Fletcher32`、`Fletcher64`）。移植自 Rust `fletcher`。

## 源包溯源与零成本迁移

| 项目 | 说明 |
| :--- | :--- |
| **源平台** | crates.io |
| **上游原包** | `fletcher` (`aaronriekenberg/rust-fletcher`) |
| **CP 包名** | `fletcher` (`v1.0.0`) |
| **许可证** | MIT |

CP 生态保持原包名 `fletcher` 不变，无需 `@cap/*` 前缀或改名：

```rust
// Rust (crates.io: fletcher)
use fletcher::{calc_fletcher16, Fletcher16};
let sum = calc_fletcher16(b"abcde");
```

```typescript
// CP（保持原包名 "fletcher"）
import { fletcher16, calcFletcher16, Fletcher16 } from "fletcher";
let sum = fletcher16("abcde");
```

## 安装

```bash
cap add fletcher
```

## 快速上手与实战示例

### 1. 一次性校验和（字符串、字节切片与字切片）

```typescript
import {
    fletcher16,
    fletcher16Bytes,
    calcFletcher16,
    fletcher32,
    fletcher32Bytes,
    calcFletcher32,
    fletcher64,
    fletcher64Bytes,
    calcFletcher64,
} from "fletcher";

fn oneShotChecksums(): void {
    // Fletcher-16（模 255）
    let s16 = fletcher16("abcde");
    let raw: [5]u8 = [97, 98, 99, 100, 101];
    let b16 = fletcher16Bytes(raw[0..5]);
    let c16 = calcFletcher16(raw[0..5]); // 对齐 Rust calc_fletcher16

    // Fletcher-32（模 65535，支持 16 位字切片或字节流）
    let s32 = fletcher32("abcde");
    let b32 = fletcher32Bytes(raw[0..5]);
    let words16: [2]u16 = [0x6261, 0x6463];
    let c32 = calcFletcher32(words16[0..2]);

    // Fletcher-64（模 4294967295，支持 32 位字切片或字节流）
    let s64 = fletcher64("abcdefgh");
    let b64 = fletcher64Bytes(raw[0..5]);
    let words32: [2]u32 = [0x64636261, 0x68676665];
    let c64 = calcFletcher64(words32[0..2]);
}
```

### 2. 流式增量校验器（`Fletcher16`、`Fletcher32`、`Fletcher64`）

```typescript
import { Fletcher16, Fletcher32, Fletcher64 } from "fletcher";

fn streamingChecksums(): void {
    // Fletcher16 分块增量更新与自定义初始种子
    let mut h16 = Fletcher16.new(); // 别名：Fletcher16.init()
    h16.update("abc");
    h16.update("de");
    let d16 = h16.digest(); // 别名：h16.value()
    h16.reset();

    let mut seeded16 = Fletcher16.withInitialValues(1, 2);
    seeded16.update("hello");

    // Fletcher32 内部自动缓存奇数长度分块的尾部残留字节
    let mut h32 = Fletcher32.new();
    h32.update("abc");
    h32.update("de");
    let d32 = h32.digest();

    // Fletcher64 支持任意非 4 字节对齐分块或 u32 字切片
    let mut h64 = Fletcher64.new();
    h64.update("abcd");
    h64.update("efgh");
    let d64 = h64.value();
}
```

## API 速览

### 流式类
- `Fletcher16`：`new()` / `init()`、`withInitialValues(a: u8, b: u8)`、`update(string)`、`updateBytes(&[]u8)`、`digest(): u16` / `value(): u16`、`reset()`
- `Fletcher32`：`new()` / `init()`、`withInitialValues(a: u16, b: u16)`、`update(string)`、`updateBytes(&[]u8)`、`updateWords(&[]u16)`、`digest(): u32` / `value(): u32`、`reset()`
- `Fletcher64`：`new()` / `init()`、`withInitialValues(a: u32, b: u32)`、`update(string)`、`updateBytes(&[]u8)`、`updateWords(&[]u32)`、`digest(): u64` / `value(): u64`、`reset()`

### 一次性函数
- `fletcher16(string): u16`、`fletcher16Bytes(&[]u8): u16`、`calcFletcher16(&[]u8): u16`
- `fletcher32(string): u32`、`fletcher32Bytes(&[]u8): u32`、`calcFletcher32(&[]u16): u32`
- `fletcher64(string): u64`、`fletcher64Bytes(&[]u8): u64`、`calcFletcher64(&[]u32): u64`
