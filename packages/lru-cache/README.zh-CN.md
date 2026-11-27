[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# lru-cache

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation O(1) LRU cache with intrusive doubly-linked list, open-addressing FNV-1a hash index, TTL expiration, and byte-weight eviction for CP

## 安装与导入

```bash
cap add lru-cache
```

```cp
import { fnv1a64 } from "lru-cache";
```

## 核心导出 API

- `export fn fnv1a64(data: &[]u8): u64;`
- `export fn copyBytes(dst: []u8, src: &[]u8);`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`lru-cache@1.0.0`)，许可证：MIT
- **上游生态**：`npm` (`lru-cache@1.0.0`)，许可证：ISC
