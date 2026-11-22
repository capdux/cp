# `cap:btf` — Linux BPF Type Format (`.BTF`) Zero-Allocation Parser

> [!NOTE]
> 本子模块已归并至统一主包 `cap:obs`，导入请使用 `import { ... } from "cap:obs"`。

Pure CP, zero-allocation binary parser for Linux `.BTF` kernel and eBPF type graphs: 24-byte `btf_header`, NUL-terminated string table lookup, variable-length type iteration across all 19 standard BTF kinds, and typed helpers for `INT`, `ARRAY`, and `STRUCT`/`UNION` (including `kflag` bitfields).

## Protocol Constants

- Header & Magic (Little-Endian):
  - `BTF_MAGIC = 0xEB9F`, `BTF_VERSION = 1`, `BTF_HEADER_SIZE = 24`, `BTF_TYPE_HEADER_SIZE = 12`
- BTF Kinds (`(info >> 24) & 0x1F`):
  - `KIND_UNKN (0)`, `KIND_INT (1)`, `KIND_PTR (2)`, `KIND_ARRAY (3)`, `KIND_STRUCT (4)`, `KIND_UNION (5)`, `KIND_ENUM (6)`, `KIND_FWD (7)`, `KIND_TYPEDEF (8)`, `KIND_VOLATILE (9)`, `KIND_CONST (10)`, `KIND_RESTRICT (11)`, `KIND_FUNC (12)`, `KIND_FUNC_PROTO (13)`, `KIND_VAR (14)`, `KIND_DATASEC (15)`, `KIND_FLOAT (16)`, `KIND_DECL_TAG (17)`, `KIND_TYPE_TAG (18)`, `KIND_ENUM64 (19)`
- `KIND_INT` Encoding Flags:
  - `INT_SIGNED (1)`, `INT_CHAR (2)`, `INT_BOOL (4)`

## Core Types & Functions

| Symbol | Description |
| :--- | :--- |
| `isBtf(data: []const u8): bool` | Fast length (`>= 24`) and `0xEB9F` magic check. |
| `parseBtf(data: []const u8): BtfSection?` | Validates magic, version `1`, `hdr_len >= 24`, and type/string section bounds. |
| `BtfSection.stringAt(off: u32): ([]const u8)?` | Zero-copy NUL-terminated string lookup in the `.BTF` string table. |
| `BtfSection.types(): BtfTypeIter` | Zero-copy iterator stepping over all variable-length BTF types (1-based `type_id`, `hasError()` detects unknown kind or truncation). |
| `parseBtfInt(t: BtfType): BtfIntInfo?` | Decodes `KIND_INT` encoding (`isSigned()`, `isChar()`, `isBool()`), `bit_offset`, and `bits`. |
| `parseBtfArray(t: BtfType): BtfArray?` | Decodes `KIND_ARRAY` (`elem_type`, `index_type`, `nelems`). |
| `btfMemberAt(t: BtfType, idx: usize): BtfMember?` | Extracts `KIND_STRUCT`/`KIND_UNION` member with `bitOffset(kflag)` and `bitfieldSize(kflag)`. |

## Example

```cp
import {
    parseBtf,
    parseBtfInt,
    btfMemberAt,
    KIND_INT,
    KIND_STRUCT,
} from "cap:obs";

fn inspectBtf(section_bytes: []const u8) {
    if (btf := parseBtf(section_bytes)) {
        let it = btf.types();
        while (t := it.next()) {
            if (name := btf.stringAt(t.name_off)) {
                * = name;
            }
            if (t.kind == KIND_INT) {
                * = parseBtfInt(t);
            } else if (t.kind == KIND_STRUCT) {
                * = btfMemberAt(t, 0);
            }
        }
    }
}
```
