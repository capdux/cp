[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# rustc-hash

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap pure-CP fast non-cryptographic FxHasher (64-bit & 32-bit, crates.io rustc-hash equivalent)

## 安装与导入

```bash
cap add rustc-hash
```

```cp
import { fxhMul64 } from "rustc-hash";
```

## 核心导出 API

- `export fn fxhMul64(a: u64, b: u64): u64;`
- `export fn fxhMul32(a: u32, b: u32): u32;`
- `export fn fxhRotl64By5(x: u64): u64;`
- `export fn fxhRotl32By5(x: u32): u32;`
- `export fn fxhAddWord64(hash: u64, word: u64): u64;`
- `export fn fxhAddWord32(hash: u32, word: u32): u32;`
- `export fn fxhReadU32LeBytes(buf: []u8, off: usize): u32;`
- `export fn fxhReadU64LeBytes(buf: []u8, off: usize): u64;`
- `export fn fxhReadU32LeStr(s: string, off: usize): u32;`
- `export fn fxhReadU64LeStr(s: string, off: usize): u64;`
- `export fn fxHash64(data: []u8): u64;`
- `export fn fxHash64Str(s: string): u64;`
- `export fn fxHash32(data: []u8): u32;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`rustc-hash@1.0.0`)，许可证：Apache-2.0 OR MIT
