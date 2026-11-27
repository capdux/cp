[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# execa

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap subprocess command builder, POSIX/Windows argument escaper, preferLocal PATH resolver, and ExecaResult inspector (npm execa equivalent, reuses npm-run-path)

## 安装与导入

```bash
cap add execa
```

```cp
import { exaPush } from "execa";
```

## 核心导出 API

- `export fn exaPush(out: []u8, pos: usize, s: string): usize;`
- `export fn exaPushByte(out: []u8, pos: usize, b: u8): usize;`
- `export fn exaNeedsQuote(arg: string): bool;`
- `export fn escapePosixArg(arg: string, out: []u8): usize;`
- `export fn formatCommand(file: string, args: []string, out: []u8): usize;`
- `export fn stripFinalNewline(s: string): string;`
- `export fn resolveExecPath(opts: ExecaOptions, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`execa@1.0.0`)，许可证：MIT
