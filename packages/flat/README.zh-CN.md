[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# flat

> 版本：`1.0.1` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation JSON flattening (a.b.0.c), dot-prop path lookup & flat-to-JSON reconstruction engine (flat / dot-prop parity) for CP

## 安装与导入

```bash
cap add flat
```

```cp
import { flattenJsonWith } from "flat";
```

## 核心导出 API

- `export fn flattenJsonWith(json_str: string, sep: u8): FlatResult;`
- `export fn flattenJson(json_str: string): FlatResult;`
- `export fn getByPath(json_str: string, dot_path: string, out: []u8): string?;`
- `export fn hasPath(json_str: string, dot_path: string): bool;`
- `export fn formatFlatJsonObject(flat: FlatResult, out: []u8): string;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`flat@1.0.1`)，许可证：BSD-3-Clause
