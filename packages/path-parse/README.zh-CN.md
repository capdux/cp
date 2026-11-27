[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# path-parse

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation POSIX and Win32 path.parse() splitter into { root, dir, base, ext, name } (clean-room MIT implementation of npm path-parse, #238 / 196M wk)

## 安装与导入

```bash
cap add path-parse
```

```cp
import { parsePosix } from "path-parse";
```

## 核心导出 API

- `export fn parsePosix(path: string): ParsedPath;`
- `export fn parseWin32(path: string): ParsedPath;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`path-parse@1.0.0`)，许可证：MIT
