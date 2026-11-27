[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# sieve-cache

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation NSDI'24 SIEVE cache eviction engine with static doubly-linked pool and open-addressing index for CP

## 安装与导入

```bash
cap add sieve-cache
```

```cp
import { emptyNode } from "sieve-cache";
```

## 核心导出 API

- `export fn emptyNode(): SieveNode;`
- `export fn mulWrap64(a: u64, b: u64): u64;`
- `export fn hashSlot(key: u64): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`sieve-cache@1.0.0`)，许可证：MIT
