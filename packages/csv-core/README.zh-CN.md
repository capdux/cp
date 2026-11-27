[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# csv-core

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation streaming RFC 4180 CSV reader/writer state machine (Rust csv-core v0.2 parity)

## 安装与导入

```bash
cap add csv-core
```

```cp
import { countCsvRecords } from "csv-core";
```

## 核心导出 API

- `export fn countCsvRecords(input: &[]u8): usize;`
- `export fn countCsvFieldsInLine(input: &[]u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`csv-core@1.0.0`)，许可证：MIT
