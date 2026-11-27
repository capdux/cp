[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# jsonpointer

> 版本：`0.1.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation RFC 6901 JSON Pointer validator, escaper, segment navigator, array index parser, and iterator in pure CP

## 安装与导入

```bash
cap add jsonpointer
```

```cp
import { isValid } from "jsonpointer";
```

## 核心导出 API

- `export fn isValid(ptr: string): bool;`
- `export fn escapeToken(token: string, out: []u8): usize;`
- `export fn unescapeToken(token: string, out: []u8): usize;`
- `export fn countSegments(ptr: string): usize;`
- `export fn hasSegment(ptr: string, index: usize): bool;`
- `export fn getSegment(ptr: string, index: usize, out: []u8): usize;`
- `export fn parseArrayIndex(token: string): i64;`
- `export fn appendSegment(base: string, token: string, out: []u8): usize;`
- `export fn parentPointer(ptr: string): string;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`jsonpointer@0.1.0`)，许可证：MIT
