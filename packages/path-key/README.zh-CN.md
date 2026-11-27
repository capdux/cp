[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# path-key

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap cross-platform PATH environment variable key lookup and PATH list splitting matching npm path-key in pure CP

## 安装与导入

```bash
cap add path-key
```

```cp
import { isPathKeyName } from "path-key";
```

## 核心导出 API

- `export fn isPathKeyName(key: string): bool;`
- `export fn findPathKeyIndex(keys: []string, is_windows: bool): i64;`
- `export fn defaultPathKey(is_windows: bool): string;`
- `export fn pathDelimiter(is_windows: bool): u8;`
- `export fn splitPathEntries(path_val: string, is_windows: bool, out: []PathSpan): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`path-key@1.0.0`)，许可证：MIT
