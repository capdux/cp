[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# jsonrpc-core

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap JSON-RPC 2.0 request/notification/result/error encoder, envelope validator, batch detector, and standard error codes.

## 安装与导入

```bash
cap add jsonrpc-core
```

```cp
import { isServerError } from "jsonrpc-core";
```

## 核心导出 API

- `export fn isServerError(code: i32): bool;`
- `export fn errorMessage(code: i32): string;`
- `export fn isWs(c: u8): bool;`
- `export fn hexLower(nib: u8): u8;`
- `export fn appendByte(out: []u8, pos: usize, b: u8): i64;`
- `export fn appendSlice(out: []u8, pos: usize, s: string): i64;`
- `export fn appendI32(out: []u8, pos: usize, v: i32): i64;`
- `export fn appendJsonString(out: []u8, pos: usize, s: string): i64;`
- `export fn encodeRequest(id_json: string, method: string, params_json: string, out: []u8): usize;`
- `export fn encodeResult(id_json: string, result_json: string, out: []u8): usize;`
- `export fn encodeError(id_json: string, code: i32, message: string, data_json: string, out: []u8): usize;`
- `export fn bytesEq(a: string, b: string): bool;`
- `export fn isBatch(json: string): bool;`
- `export fn isValidEnvelope(json: string): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`jsonrpc-core@1.0.0`)，许可证：MIT
- **上游生态**：`npm` (`json-rpc-2.0@1.0.0`)，许可证：MIT
- **上游生态**：`crates.io` (`jsonrpsee@1.0.0`)，许可证：MIT
