[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# siphash

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation SipHash-2-4 and SipHash-1-3 keyed 64-bit HashDoS-resistant PRF for CP.

## 安装与导入

```bash
cap add siphash
```

```cp
import { siphash24 } from "siphash";
```

## 核心导出 API

- `export fn siphash24(k0: u64, k1: u64, data: &[]u8): u64;`
- `export fn siphash13(k0: u64, k1: u64, data: &[]u8): u64;`
- `export fn siphash24Key16(key: [16]u8, data: &[]u8): u64;`
- `export fn siphash13Key16(key: [16]u8, data: &[]u8): u64;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`siphash@1.0.0`)，许可证：MIT
