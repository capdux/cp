[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# map-obj

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap fixed-pool object key-value map/filter/transform utility with skip sentinel and deep key path support (npm map-obj equivalent)

## 安装与导入

```bash
cap add map-obj
```

```cp
import { mobjStrEq } from "map-obj";
```

## 核心导出 API

- `export fn mobjStrEq(a: string, b: string): bool;`
- `export fn swapKeysAndValues(src: MapObjResult): MapObjResult;`
- `export fn pickKeys(src: MapObjResult, allow_keys: []string): MapObjResult;`
- `export fn omitKeys(src: MapObjResult, skip_keys: []string): MapObjResult;`
- `export fn filterEmptyValues(src: MapObjResult): MapObjResult;`
- `export fn mergeObjects(base: MapObjResult, overrides: MapObjResult): MapObjResult;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`map-obj@1.0.0`)，许可证：MIT
