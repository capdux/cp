# `cap:wasm_fmt` — WebAssembly `.wasm` 二进制解析与构建标准库

## 概述

`cap:wasm_fmt`（同时支持 `cap.wasm_fmt` 动态挂载）是纯 CP 实现的工业级 WebAssembly 二进制格式（`.wasm` v1）解析、LEB128 编解码与模块构建标准库。库内部严格执行魔数/版本头校验、标准 Section ID 单调递增与去重检查、Payload 长度精确对齐与防越界保护，以及 32 位 LEB128 最大 5 字节防溢出校验。

## 核心类型

| 类型 / 枚举 | 说明 |
| --- | --- |
| `WasmError` | 错误集合：`InvalidMagic`、`UnsupportedVersion`、`UnexpectedEof`、`Leb128Overflow`、`InvalidSectionId`、`OutOfOrderSection`、`DuplicateSection`、`SectionLengthMismatch`、`InvalidValType`、`InvalidExternalKind`、`InvalidLimitFlags`、`InvalidFuncTypeTag`、`InvalidMutFlag`、`InvalidTypeIndex`、`OutOfMemory` |
| `ValType` | 值类型枚举：`I32 (0x7F)`、`I64 (0x7E)`、`F32 (0x7D)`、`F64 (0x7C)`、`FuncRef (0x70)`、`ExternRef (0x6F)` |
| `SectionId` | 段 ID 枚举：`Custom (0)` .. `Data (11)` |
| `ExternalKind` | 导入/导出类型枚举：`Func (0)`、`Table (1)`、`Memory (2)`、`Global (3)` |
| `FuncType` | 函数签名 `{ params: []const ValType, results: []const ValType }` |
| `WasmLimits` | 内存/表边界 `{ min: u32, max: u32? }` |
| `WasmImport` | 导入项描述（module、name、kind、type_index / memory_limits） |
| `WasmExport` | 导出项描述（name、kind、index） |
| `WasmMemory` | 线性内存段条目 `{ limits: WasmLimits }` |
| `WasmGlobal` | 全局变量条目 `{ val_type, mutable, init_expr }` |
| `CustomSection` | 自定义段条目 `{ name, data }` |
| `WasmModule` | 完整解析/构建的 WASM 模块容器，提供 `deinit(*_, alloc)` 确定性释放内部分配内存 |

## API 速查

- **LEB128 编解码**
  - `readUleb128(data: []const u8, offset: *usize): WasmError!u32`
  - `writeUleb128(out: *std.ArrayList(u8), alloc: std.mem.Allocator, value: u32): WasmError!void`
  - `readSleb128(data: []const u8, offset: *usize): WasmError!i32`
  - `writeSleb128(out: *std.ArrayList(u8), alloc: std.mem.Allocator, value: i32): WasmError!void`
- **模块解析与编码**
  - `parseWasm(alloc: std.mem.Allocator, bytes: []const u8): WasmError!WasmModule`
  - `encodeWasm(alloc: std.mem.Allocator, mod: WasmModule): WasmError![]u8`
- **辅助查询与增补**
  - `findExport(mod: WasmModule, name: []const u8): WasmExport?`
  - `findExportFuncType(mod: WasmModule, name: []const u8): FuncType?`
  - `findCustomSection(mod: WasmModule, name: []const u8): CustomSection?`
  - `addCustomSection(alloc: std.mem.Allocator, wasm_bytes: []const u8, name: []const u8, data: []const u8): WasmError![]u8`

## 示例

```cp
import {
    WasmModule,
    FuncType,
    ValType,
    WasmExport,
    encodeWasm,
    parseWasm,
    findExportFuncType,
} from "cap:wasm_fmt";
const std = @import("std");

pub fn roundtripExample(alloc: std.mem.Allocator): !void {
    const params = [_]ValType{ .I32, .I32 };
    const results = [_]ValType{.I32};
    const types = [_]FuncType{.{ .params = &params, .results = &results }};
    const funcs = [_]u32{0};
    const exports = [_]WasmExport{.{ .name = "add", .kind = .Func, .index = 0 }};
    const body = [_]u8{ 0x00, 0x20, 0x00, 0x20, 0x01, 0x6A, 0x0B };
    const codes = [_][]const u8{&body};

    const mod = WasmModule{
        .types = &types,
        .functions = &funcs,
        .exports = &exports,
        .codes = &codes,
    };

    const bytes = try encodeWasm(alloc, mod);
    defer alloc.free(bytes);

    let parsed = try parseWasm(alloc, bytes);
    defer parsed.deinit(alloc);

    if (sig := findExportFuncType(parsed, "add")) {
        _ = sig;
    }
}
```
