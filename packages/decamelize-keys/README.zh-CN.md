[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# decamelize-keys

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap object key decamelizer with custom separator, exclude list, and fixed key buffer pool, composing @cap/map-obj and @cap/decamelize (clean-room port of npm decamelize-keys)

## 安装与导入

```bash
cap add decamelize-keys
```

```cp
import { dckStrEq } from "decamelize-keys";
```

## 核心导出 API

- `export fn dckStrEq(a: string, b: string): bool;`
- `export fn dckIsExcluded(k: string, exclude: []string): bool;`
- `export fn decamelizeKeys(src: MapObjResult, sep: string, exclude: []string): DecamelizeKeysResult;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`decamelize-keys@1.0.0`)，许可证：MIT
