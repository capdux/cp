[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# fnv

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation FNV-1a 32/64-bit and Rust rustc-hash FxHash 64-bit streaming non-cryptographic hashers for CP

## 安装与导入

```bash
cap add fnv
```

```cp
import { mulWrap32 } from "fnv";
```

## 核心导出 API

- `export fn mulWrap32(a: u32, b: u32): u32;`
- `export fn mulWrap64(a: u64, b: u64): u64;`
- `export fn rotl64By5(x: u64): u64;`
- `export fn fnv1a32(data: string): u32;`
- `export fn fnv1a64(data: string): u64;`
- `export fn fxHash64(data: string): u64;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`rustc-hash@1.0.0`)，许可证：MIT
- **上游生态**：`crates.io` (`fnv@1.0.0`)，许可证：MIT
