[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# url

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Pure CP zero-allocation WHATWG URL Standard parser, normalizer, and relative URL resolver (Rust url v2 parity)

## 安装与导入

```bash
cap add url
```

```cp
import { defaultPortForScheme } from "url";
```

## 核心导出 API

- `export fn defaultPortForScheme(scheme: string): i32;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`url@1.0.0`)，许可证：MIT
