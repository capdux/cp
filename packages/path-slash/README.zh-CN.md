[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# path-slash

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

CP 官方生态包 path-slash

## 安装与导入

```bash
cap add path-slash
```

```cp
import { pshIsAsciiAlpha } from "path-slash";
```

## 核心导出 API

- `export fn pshIsAsciiAlpha(b: u8): bool;`
- `export fn pshIsSep(b: u8): bool;`
- `export fn pshWriteByte(out: []u8, pos: usize, b: u8): usize;`
- `export fn pshValidUtf8Len(s: string, offset: usize): usize;`
- `export fn pshIsAbs(path: string): bool;`
- `export fn toSlash(path: string, out: []u8): usize;`
- `export fn toSlashLossy(path: string, out: []u8): usize;`
- `export fn fromSlash(path: string, out: []u8): usize;`
- `export fn hasBackslash(path: string): bool;`
- `export fn hasForwardSlash(path: string): bool;`
- `export fn trimTrailingSlashes(path: string): string;`
- `export fn joinSlash(a: string, b: string, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`path-slash@1.0.0`)，许可证：MIT
