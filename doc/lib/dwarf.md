# `cap:dwarf` — DWARF v4/v5 调试信息解析标准库

## 1. 概述

`cap:dwarf`（`lib/dwarf/`，支持 `import ... from "cap:dwarf"` 与 `cap.dwarf` 动态挂载）是纯 CP 实现的工业级 DWARF v4/v5 调试信息解析库，覆盖 `.debug_line` 行号表状态机解码与二分地址映射、`encodeDebugLineV4` 编码往返辅助，以及 `.debug_abbrev` / `.debug_info` / `.debug_str` 编译单元 DIE 树解析。

## 2. 核心类型

- **`DwarfError`**：错误枚举（`TruncatedData`, `UnsupportedVersion`, `InvalidHeader`, `InvalidOpcode`, `InvalidFileIndex`, `MissingAbbrev`, `UnsupportedForm`, `InvalidUtf8`, `LoopLimitExceeded`, `InvalidLeb128`, `OutOfMemory`）。
- **`.debug_line` 相关**：`FileEntry`, `LineProgramHeader`, `LineRow`, `LineLocation`, `LineEncodeEntry`, `LineTable`。
- **`.debug_abbrev` / `.debug_info` 相关**：`DwTag`, `DwAt`, `DwForm`, `AttrSpec`, `AbbrevEntry`, `AbbrevTable`, `AttrValueKind`, `AttrValue`, `DieAttr`, `DieNode`, `CompileUnitHeader`, `CompileUnit`。

## 3. API 速查

| 函数 / 方法 | 签名 | 说明 |
| --- | --- | --- |
| `parseDebugLine` | `(alloc, data: []const u8, default_addr_size: u8): !LineTable` | 解析 DWARF v4/v5 `.debug_line` 单元并执行状态机 |
| `lookupAddress` | `(table: *const LineTable, pc: u64): LineLocation?` | 二分查找 PC 对应的 `file:line:column` |
| `encodeDebugLineV4` | `(alloc, dirs, files, entries, addr_size): ![]u8` | 编码生成 DWARF v4 `.debug_line` 单元 |
| `parseAbbrevTable` | `(alloc, debug_abbrev, offset: u64): !AbbrevTable` | 解析 `.debug_abbrev` 缩写表 |
| `parseDebugInfoCu` | `(alloc, debug_info, debug_abbrev, debug_str, cu_offset: u64): !CompileUnit` | 解析 `.debug_info` 编译单元 DIE 树 |

## 4. 示例

```cp
const std = @import("std");
import { parseDebugLine, lookupAddress } from "cap:dwarf";

pub fn resolvePc(alloc: std.mem.Allocator, debug_line: []const u8, pc: u64): !void {
    let table = try parseDebugLine(alloc, debug_line, 8);
    defer table.deinit(alloc);
    if (lookupAddress(&table, pc)) |loc| {
        std.debug.print("{s}:{d}:{d}\n", .{ loc.file, loc.line, loc.column });
    }
}
```
