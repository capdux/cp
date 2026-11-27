[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# toml_writer

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap TOML 1.0 serializer and formatter (bare/quoted keys, escaped strings, tables, array-of-tables, inline arrays) inspired by Rust toml_writer

## 安装与导入

```bash
cap add toml_writer
```

```cp
import { isBareKey } from "toml_writer";
```

## 核心导出 API

- `export fn isBareKey(k: string): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`toml_writer@1.0.0`)，许可证：MIT
