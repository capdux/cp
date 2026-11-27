[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# y18n

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation internationalization (i18n) catalog and printf-style formatter (y18n parity) in pure CP

## 安装与导入

```bash
cap add y18n
```

```cp
import { formatPrintf } from "y18n";
```

## 核心导出 API

- `export fn formatPrintf(tmpl: string, count: i64, has_count: bool, args: &[]string, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`y18n@1.0.0`)，许可证：MIT
