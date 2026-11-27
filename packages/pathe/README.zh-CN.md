[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# pathe

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation universal POSIX-normalized path utilities preserving Windows drive letters and URL protocols (clean-room MIT implementation of npm pathe, #219 / 207M wk)

## 安装与导入

```bash
cap add pathe
```

```cp
import { pthIsSep } from "pathe";
```

## 核心导出 API

- `export fn pthIsSep(b: u8): bool;`
- `export fn pthIsAlpha(b: u8): bool;`
- `export fn pthIsSchemeChar(b: u8): bool;`
- `export fn pthHasDrivePrefix(s: string): bool;`
- `export fn pthProtocolPrefixLen(s: string): usize;`
- `export fn pthPush(out: []u8, pos: usize, b: u8): usize;`
- `export fn pthPushSlice(out: []u8, pos: usize, s: string): usize;`
- `export fn pthStrEq(a: string, b: string): bool;`
- `export fn isAbsolute(path: string): bool;`
- `export fn normalize(path: string, out: []u8): usize;`
- `export fn join(a: string, b: string, out: []u8): usize;`
- `export fn joinPath(a: string, b: string, out: []u8): usize;`
- `export fn pthStripTrailingSlash(out: []u8, len: usize): usize;`
- `export fn resolve(base: string, rel: string, out: []u8): usize;`
- `export fn dirname(path: string): string;`
- `export fn basename(path: string): string;`
- `export fn extname(path: string): string;`
- `export fn pthSplitSegments(s: string, starts: []usize, lens: []usize): usize;`
- `export fn relative(from_path: string, to_path: string, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`pathe@1.0.0`)，许可证：MIT
