[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/fnv

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:rustc-hash` ([https://github.com/rust-lang/rustc-hash](https://github.com/rust-lang/rustc-hash)) / `crates.io:fnv` ([https://github.com/servo/rust-fnv](https://github.com/servo/rust-fnv))（原库名 `rustc-hash` → CP 包名 `@cap/fnv`）

Zero-allocation FNV-1a 32/64-bit and Rust `rustc-hash` FxHash 64-bit streaming non-cryptographic hashers for CP — clean-room MIT parity with Rust `fnv` and `rustc-hash`.

零堆分配的 CP 超高速非加密流式哈希引擎，合一实现标准 FNV-1a 32/64-bit（通过全部官方测试向量）与 Rust 编译器御用 FxHash 64-bit（左旋 5 位 + XOR + 乘黄金比例魔数 `0x517cc1b727220a95`），全程无溢出 64-bit 绕回算术（纯 CP 洁净室 MIT 实现）。

## Install / 安装

```bash
cap add @cap/fnv
```

## API Reference / 接口一览

| Symbol | Signature | Description |
| :--- | :--- | :--- |
| `Fnv32Hasher` | `init()`, `write(*_, data)`, `writeByte(*_, b)`, `finish() -> u32` | FNV-1a 32-bit 流式哈希器 |
| `fnv1a32` | `(data: string) -> u32` | FNV-1a 32-bit 单次快捷哈希 |
| `Fnv64Hasher` | `init()`, `write(*_, data)`, `writeByte(*_, b)`, `finish() -> u64` | FNV-1a 64-bit 流式哈希器 |
| `fnv1a64` | `(data: string) -> u64` | FNV-1a 64-bit 单次快捷哈希 |
| `FxHasher` | `init()`, `withSeed(seed)`, `writeU64(*_, v)`, `writeBytes(*_, data)`, `finish() -> u64` | `rustc-hash` FxHash 64-bit 流式哈希器 |
| `fxHash64` | `(data: string) -> u64` | FxHash 64-bit 单次快捷哈希 |

## Usage / 使用示例

```cp
import { fnv1a32, fnv1a64, fxHash64, Fnv64Hasher } from "@cap/fnv";

const h32 = fnv1a32("foobar"); // 0xbf9cf968
const h64 = fnv1a64("foobar"); // 0x85944171f73967e8
const fx = fxHash64("capdux:compiler:symbol");

let stream = Fnv64Hasher.init();
stream.write("foo");
stream.write("bar");
const same = stream.finish(); // == h64
```

## License

MIT
