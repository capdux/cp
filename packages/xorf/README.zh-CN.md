[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# xorf

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation Xor8 static fingerprint filter (Graf & Lemire ACM JEA 2020 / Rust xorf parity) with O(1) 3-lookup XOR queries and ~0.39% false-positive rate for CP

## 安装与导入

```bash
cap add xorf
```

```cp
import { makeU64 } from "xorf";
```

## 核心导出 API

- `export fn makeU64(hi: u32, lo: u32): u64;`
- `export fn hashKey64(key: u64, seed: u64): u64;`
- `export fn deriveSlots(h: u64, block_len: usize): Slots3;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`xorf@1.0.0`)，许可证：MIT
