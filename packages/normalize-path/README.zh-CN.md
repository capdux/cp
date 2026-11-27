[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# normalize-path

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

CP 官方生态包 normalize-path

## 安装与导入

```bash
cap add normalize-path
```

```cp
import { isSlash } from "normalize-path";
```

## 核心导出 API

- `export fn isSlash(ch: u8): bool;`
- `export fn isAsciiAlpha(ch: u8): bool;`
- `export fn hasBackslash(path: string): bool;`
- `export fn isUncPath(path: string): bool;`
- `export fn normalizePath(path: string, strip_trailing: bool, out: []u8): usize;`
- `export fn normalizePathDefault(path: string, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`normalize-path@1.0.0`)，许可证：MIT
