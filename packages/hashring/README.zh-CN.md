[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# hashring

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation consistent hash ring with virtual nodes, O(log V) binary search routing, and deduplicated replica selection for CP

## 安装与导入

```bash
cap add hashring
```

```cp
import { mulWrap64 } from "hashring";
```

## 核心导出 API

- `export fn mulWrap64(a: u64, b: u64): u64;`
- `export fn avalanche64(x: u64): u64;`
- `export fn fnv1a64Feed(state: u64, b: u8): u64;`
- `export fn hashKey(key: string): u64;`
- `export fn hashVnode(name: string, v_idx: u16): u64;`
- `export fn eqStr(a: string, b: string): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`hashring@1.0.0`)，许可证：MIT
