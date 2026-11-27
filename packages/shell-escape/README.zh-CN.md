[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# shell-escape

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation POSIX sh and Windows CommandLineToArgvW argument escaper, command joiner, and env var name validator in pure CP

## 安装与导入

```bash
cap add shell-escape
```

```cp
import { isSafePosixByte } from "shell-escape";
```

## 核心导出 API

- `export fn isSafePosixByte(b: u8): bool;`
- `export fn isSafePosixWord(arg: string): bool;`
- `export fn escapePosix(arg: string, out: []u8): usize;`
- `export fn needsWindowsQuotes(arg: string): bool;`
- `export fn escapeWindows(arg: string, out: []u8): usize;`
- `export fn joinPosixCommand(args: &[]string, out: []u8): usize;`
- `export fn isValidEnvName(name: string): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`shell-escape@1.0.0`)，许可证：MIT
- **上游生态**：`crates.io` (`shell-escape@1.0.0`)，许可证：MIT
