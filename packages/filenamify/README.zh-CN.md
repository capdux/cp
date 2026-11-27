[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# filenamify

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation cross-platform safe filename sanitizer (npm filenamify v6 parity) for CP

## 安装与导入

```bash
cap add filenamify
```

```cp
import { fnmIsInvalidChar } from "filenamify";
```

## 核心导出 API

- `export fn fnmIsInvalidChar(ch: u8): bool;`
- `export fn fnmHasInvalidChar(s: string): bool;`
- `export fn fnmToLowerAscii(ch: u8): u8;`
- `export fn fnmEq3IgnoreCase(s: string, a: u8, b: u8, c: u8): bool;`
- `export fn fnmMatchAt(buf: string, pos: usize, pat: string): bool;`
- `export fn fnmEndsWith(buf: string, len: usize, pat: string): bool;`
- `export fn fnmAppendStr(dst: []u8, pos: usize, src: string): usize;`
- `export fn fnmUtf8SafeLen(s: string, max_len: usize): usize;`
- `export fn isReservedFilename(name: string): bool;`
- `export fn filenamifyWithOptions(input: string, replacement: string, max_len: usize, out: []u8): usize;`
- `export fn filenamify(input: string, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`filenamify@1.0.0`)，许可证：MIT
