[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# siphasher

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation SipHash-2-4 and SipHash-1-3 keyed streaming PRF/hashers (HashDoS-resistant) for CP

## 安装与导入

```bash
cap add siphasher
```

```cp
import { addWrap64 } from "siphasher";
```

## 核心导出 API

- `export fn addWrap64(a: u64, b: u64): u64;`
- `export fn rotl64(x: u64, r: u64): u64;`
- `export fn sipRound(s: SipState): SipState;`
- `export fn loadLe64(buf: []u8, offset: usize): u64;`
- `export fn loadLe64Str(s: string, offset: usize): u64;`
- `export fn siphash24(k0: u64, k1: u64, data: string): u64;`
- `export fn siphash13(k0: u64, k1: u64, data: string): u64;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`siphasher@1.0.0`)，许可证：MIT OR Apache-2.0
