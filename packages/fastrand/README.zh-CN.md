[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# fastrand

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation WyRand 64-bit PRNG with unbiased range sampling, 53-bit unit floats, Fisher-Yates shuffle, and alphanumeric generation for CP

## 安装与导入

```bash
cap add fastrand
```

```cp
import { addWrap64 } from "fastrand";
```

## 核心导出 API

- `export fn addWrap64(a: u64, b: u64): u64;`
- `export fn mulWide64Xor(a: u64, b: u64): u64;`
- `export fn i64ToOrderedU64(v: i64): u64;`
- `export fn orderedU64ToI64(u: u64): i64;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`fastrand@1.0.0`)，许可证：MIT
