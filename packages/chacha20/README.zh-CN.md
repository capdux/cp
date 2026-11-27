[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# chacha20

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation RFC 8439 ChaCha20 stream cipher (Rust chacha20 crate parity) for CP

## 安装与导入

```bash
cap add chacha20
```

```cp
import { cc20Add32 } from "chacha20";
```

## 核心导出 API

- `export fn cc20Add32(a: u32, b: u32): u32;`
- `export fn cc20Rotl16(x: u32): u32;`
- `export fn cc20Rotl12(x: u32): u32;`
- `export fn cc20Rotl8(x: u32): u32;`
- `export fn cc20Rotl7(x: u32): u32;`
- `export fn cc20ReadLe32(b: []u8, off: usize): u32;`
- `export fn cc20WriteLe32(out: []u8, off: usize, v: u32);`
- `export fn cc20QuarterRound(st: []u32, ai: usize, bi: usize, ci: usize, di: usize);`
- `export fn chacha20Block(key: []u8, counter: u32, nonce: []u8, out64: []u8): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`chacha20@1.0.0`)，许可证：MIT OR Apache-2.0
