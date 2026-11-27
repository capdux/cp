[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# jsonpath-rust

> 版本：`0.7.1` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap RFC 9535 JSONPath expression parser and canonical serializer in pure CP

## 安装与导入

```bash
cap add jsonpath-rust
```

```cp
import { jprIsWs } from "jsonpath-rust";
```

## 核心导出 API

- `export enum JpSegKind`
- `export fn jprIsWs(c: u8): bool;`
- `export fn jprIsDigit(c: u8): bool;`
- `export fn jprIsIdentStart(c: u8): bool;`
- `export fn jprIsIdentCont(c: u8): bool;`
- `export fn jprIsSimpleIdent(s: string): bool;`
- `export fn jprMakeSeg(kind: JpSegKind, name: string): JpSegment;`
- `export fn jprParseInt(s: string, start_pos: usize): JprIntRes;`
- `export fn parseJsonPath(expr: string, out: []JpSegment): i64;`
- `export fn jprPushByte(out: []u8, pos: usize, b: u8): usize;`
- `export fn jprPushSlice(out: []u8, pos: usize, s: string): usize;`
- `export fn jprPushI64(out: []u8, pos: usize, val: i64): usize;`
- `export fn formatJsonPath(segs: &[]JpSegment, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`jsonpath-rust@0.7.1`)（https://github.com/besok/jsonpath-rust），许可证：MIT
