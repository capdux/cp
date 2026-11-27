[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# is-path-inside

> 版本：`4.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation path traversal guard checking whether child_path is strictly inside parent_path

## 安装与导入

```bash
cap add is-path-inside
```

```cp
import { isPathInside } from "is-path-inside";
```

## 核心导出 API

- `export fn isPathInside(child_path: string, parent_path: string): bool;`
- `export fn isPathInsideOrEqual(child_path: string, parent_path: string): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`is-path-inside@4.0.0`)（https://github.com/sindresorhus/is-path-inside），许可证：MIT
