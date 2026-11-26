# @cap/toml-edit

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:toml_edit` ([https://github.com/toml-rs/toml](https://github.com/toml-rs/toml))（原库名 `toml_edit` → CP 包名 `@cap/toml-edit`）

Zero-allocation TOML v1.0 document parser, typed query/editor, and canonical formatter (`TomlDoc`, `TomlEntry`) for CP — clean-room MIT parity with Rust `toml_edit` v0.22 (900M+ downloads).

纯 CP 零堆分配 TOML v1.0 文档解析、修改与规范化输出引擎，支持 `[table]` / `[a.b]` 节头、点分键（`a.b = 1`）、`#` 注释、基础/字面字符串、带下划线整数（`1_000`）/负整数、布尔值与无损格式化往返。

## Install / 安装

```bash
cap add @cap/toml-edit
```

## API

### Constants & Entry

- `TOML_STR: u8 = 1`, `TOML_INT: u8 = 2`, `TOML_BOOL: u8 = 3`, `TOML_MAX_ENTRIES: usize = 64`
- `TomlEntry { section: string, key: string, kind: u8, str_val: string, int_val: i64, bool_val: bool }`

### `TomlDoc`

| Method | Signature | Description |
| --- | --- | --- |
| `new` | `static fn new() -> TomlDoc` | Creates an empty document (64-entry fixed pool) |
| `parse` | `static fn parse(src: string) -> TomlDoc` | Parses TOML v1.0 source into a `TomlDoc` |
| `len` | `() -> usize` | Returns number of stored entries |
| `hasKey` | `(section: string, key: string) -> bool` | Checks whether `(section, key)` exists |
| `getStr` / `getInt` / `getBool` | `(section, key) -> T?` | Typed lookups |
| `setStr` / `setInt` / `setBool` | `(*_, section, key, val) -> bool` | In-place upsert of typed entries |
| `format` | `(out: []u8) -> usize` | Serializes document into canonical TOML text |

## Usage / 使用示例

```cp
import { TomlDoc } from "@cap/toml-edit";

let doc = TomlDoc.parse("[package]\nname = \"demo\"\nport = 8_080\n");
const port = doc.getInt("package", "port"); // 8080
* = doc.setInt("package", "port", 9090 as i64);

let out: [256]u8 = undefined;
let n = doc.format(out[0..256]);
```
