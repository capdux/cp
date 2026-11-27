[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# resp

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap Redis RESP2 wire protocol streaming frame parser and command encoder.

## 安装与导入

```bash
cap add resp
```

```cp
import { makeFrame } from "resp";
```

## 核心导出 API

- `export fn makeFrame(kind: u8): RespFrame;`
- `export fn parseI64(s: &[]u8): IntResult;`
- `export fn findCrlf(buf: &[]u8, start: usize): i64;`
- `export fn parseFrame(buf: &[]u8): RespFrame;`
- `export fn appendByte(dst: []u8, pos: usize, b: u8): i64;`
- `export fn appendSlice(dst: []u8, pos: usize, s: &[]u8): i64;`
- `export fn appendI64(dst: []u8, pos: usize, v: i64): i64;`
- `export fn encodeSimpleString(s: &[]u8, dst: []u8): i64;`
- `export fn encodeInteger(v: i64, dst: []u8): i64;`
- `export fn encodeNull(dst: []u8): i64;`
- `export fn encodeBulkString(s: &[]u8, dst: []u8): i64;`
- `export fn encodeCommand(args: &[]&[]u8, dst: []u8): i64;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`resp@1.0.0`)，许可证：MIT
