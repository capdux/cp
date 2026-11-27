[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# logfmt

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation logfmt structured key=value log parser and writer (Heroku/Grafana Loki/Kubernetes) in pure CP

## 安装与导入

```bash
cap add logfmt
```

```cp
import { isSpace } from "logfmt";
```

## 核心导出 API

- `export fn isSpace(b: u8): bool;`
- `export fn bytesEq(a: &[]u8, b: &[]u8): bool;`
- `export fn needsQuotes(val: &[]u8): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`logfmt@1.0.0`)，许可证：MIT
