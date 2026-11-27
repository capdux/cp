[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# multimap

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation insertion-ordered one-to-many string multimap (capacity 128) for HTTP headers, URL query params, tag indexes, and event routing (Rust multimap parity) for CP

## 安装与导入

```bash
cap add multimap
```

```cp
import { strEq } from "multimap";
```

## 核心导出 API

- `export fn strEq(a: string, b: string): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`multimap@1.0.0`)，许可证：MIT
