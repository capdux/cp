[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# fletcher

纯 CP 实现的零堆分配 Fletcher-16、Fletcher-32 与 Fletcher-64 位置敏感校验和库，同时支持一次性计算与流式增量更新。

- **包名**：`fletcher`（`v1.0.0`）
- **上游对齐**：洁净室移植自 `crates.io/fletcher`（`MIT`，`rust-fletcher`），支持字节切片、字切片、字符串输入及自定义初始种子。

## 安装与导入

```bash
cap add fletcher
```

```typescript
import {
    Fletcher16, Fletcher32, Fletcher64,
    fletcher16, fletcher16Bytes, calcFletcher16,
    fletcher32, fletcher32Bytes, calcFletcher32,
    fletcher64, fletcher64Bytes, calcFletcher64,
} from "fletcher";
```

## 核心 API

### 流式校验和类

| 类型 | 构造与方法 | 说明 |
| --- | --- | --- |
| `Fletcher16` | `new()`, `init()`, `withInitialValues(a: u8, b: u8)`, `update(data: string)`, `updateBytes(data: &[]u8)`, `digest(): u16`, `value(): u16`, `reset()` | 16 位流式 Fletcher 校验和（按字节，`mod 255`）。 |
| `Fletcher32` | `new()`, `init()`, `withInitialValues(a: u16, b: u16)`, `updateWords(words: &[]u16)`, `updateBytes(data: &[]u8)`, `update(data: string)`, `digest(): u32`, `value(): u32`, `reset()` | 32 位流式 Fletcher 校验和（按 16 位字，`mod 65535`），自动缓冲奇数末尾字节。 |
| `Fletcher64` | `new()`, `init()`, `withInitialValues(a: u32, b: u32)`, `updateWords(words: &[]u32)`, `updateBytes(data: &[]u8)`, `update(data: string)`, `digest(): u64`, `value(): u64`, `reset()` | 64 位流式 Fletcher 校验和（按 32 位字，`mod 0xFFFFFFFF`），自动缓冲不足 4 字节的尾部。 |

### 一次性校验和函数

| 函数 | 签名 | 说明 |
| --- | --- | --- |
| `fletcher16` | `(data: string): u16` | 对字符串一次性计算 Fletcher-16。 |
| `fletcher16Bytes` / `calcFletcher16` | `(data: &[]u8): u16` | 对字节切片一次性计算 Fletcher-16。 |
| `fletcher32` | `(data: string): u32` | 对字符串一次性计算 Fletcher-32。 |
| `fletcher32Bytes` | `(data: &[]u8): u32` | 对字节切片一次性计算 Fletcher-32。 |
| `calcFletcher32` | `(words: &[]u16): u32` | 对 `u16` 字切片计算 Fletcher-32（对齐 Rust `calc_fletcher32`）。 |
| `fletcher64` | `(data: string): u64` | 对字符串一次性计算 Fletcher-64。 |
| `fletcher64Bytes` | `(data: &[]u8): u64` | 对字节切片一次性计算 Fletcher-64。 |
| `calcFletcher64` | `(words: &[]u32): u64` | 对 `u32` 字切片计算 Fletcher-64（对齐 Rust `calc_fletcher64`）。 |

## 使用示例

### 1. 一次性计算

```typescript
import { fletcher16, fletcher32, fletcher64 } from "fletcher";

fn main() {
    let c16 = fletcher16("abcde");   // 0xC8F0
    let c32 = fletcher32("abcdef");  // 32 位校验和
    let c64 = fletcher64("abcdefgh");
}
```

### 2. 流式分块计算

```typescript
import { Fletcher32 } from "fletcher";

fn main() {
    let h = Fletcher32.new();
    h.update("abc");
    h.update("def");
    let sum = h.digest();
    h.reset();
}
```

### 3. 自定义初始值与字切片

```typescript
import { Fletcher16, calcFletcher32 } from "fletcher";

fn main() {
    let h = Fletcher16.withInitialValues(0xFF, 0xFF);
    h.update("hello");
    let seeded = h.value();

    let words: [3]u16 = [0x0102, 0x0304, 0x0506];
    let wsum = calcFletcher32(words[0..]);
}
```

## 与源包差异及零拷贝安全亮点

- **零堆分配与尾字节缓冲**：`Fletcher32` 与 `Fletcher64` 在栈对象内部缓存未满字长的尾字节，任意分块喂入与整块输入结果完全一致。
- **字/字节双接口**：同时提供对齐 Rust 上游的 `calcFletcher32(&[]u16)` / `calcFletcher64(&[]u32)` 与面向原始字节流的 `fletcher32Bytes` / `fletcher64Bytes`。
- **非破坏性读取**：`digest()` / `value()` 不会清空累加状态，复用实例需显式调用 `reset()`。
