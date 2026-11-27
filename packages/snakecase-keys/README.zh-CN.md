[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# snakecase-keys

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap object key converter to snake_case with exclude list and fixed key buffer pool, composing map-obj and heck (clean-room port of npm snakecase-keys)

## 安装与导入

```bash
cap add snakecase-keys
```

```cp
import { sckStrEq } from "snakecase-keys";
```

## 核心导出 API

- `export fn sckStrEq(a: string, b: string): bool;`
- `export fn sckIsExcluded(k: string, exclude: []string): bool;`
- `export fn snakecaseKeyInto(key: string, out: []u8): usize;`
- `export fn snakecaseKeys(src: MapObjResult, exclude: []string): SnakecaseKeysResult;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`snakecase-keys@1.0.0`)，许可证：MIT
