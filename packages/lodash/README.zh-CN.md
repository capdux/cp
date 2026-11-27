[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# lodash

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation dot-path parser and deep JSON path reader (dot-prop / lodash.get / dlv parity) for CP

## 安装与导入

```bash
cap add lodash
```

```cp
import { parseDotPath } from "lodash";
```

## 核心导出 API

- `export fn parseDotPath(path: string, key_buf: []u8): ParsedDotPath;`
- `export fn getJsonPath(json: string, path: string): string?;`
- `export fn getJsonString(json: string, path: string): string?;`
- `export fn hasJsonPath(json: string, path: string): bool;`
- `export fn escapePathSegment(seg: string, out: []u8): string;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`lodash@1.0.0`)，许可证：MIT
- **上游生态**：`npm` (`dot-prop@1.0.0`)，许可证：MIT
