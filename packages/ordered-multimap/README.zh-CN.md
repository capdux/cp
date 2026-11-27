[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# ordered-multimap

> 版本：`0.7.3` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap insertion-ordered multimap supporting multiple values per key

## 安装与导入

```bash
cap add ordered-multimap
```

```cp
import { ommEmptyStrings } from "ordered-multimap";
```

## 核心导出 API

- `export fn ommEmptyStrings(): [OMM_CAP]string;`
- `export fn ommStrEq(a: string, b: string): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`ordered-multimap@0.7.3`)（https://github.com/sgodwinson/ordered-multimap），许可证：MIT
