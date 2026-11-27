[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# toml_edit

> **Source Attribution**: Clean-room MIT CP implementation of [`crates.io:toml_edit`](https://github.com/toml-rs/toml) (MIT, v0.22 parity) | CP Package Version: `1.0.0` | Zero external dependencies

Zero-allocation TOML v1.0 document parser, typed query/mutation engine, and canonical section-grouped formatter for CP.

## Zero-Cost Migration (Rust → CP)

Keep the exact same package name (`"toml_edit"`) — no renaming required.

| Rust (`crates.io:toml_edit`) | CP (`toml_edit`) |
| :--- | :--- |
| `use toml_edit::DocumentMut;` | `import { TomlDoc } from "toml_edit";` |
| `let mut doc = src.parse::<DocumentMut>()?;` | `let doc = TomlDoc.parse(src);` |
| `doc["package"]["name"].as_str()` | `doc.getStr("package", "name")` |
| `doc["server"]["port"] = value(8080);` | `doc.setInt("server", "port", 8080)` |
| `doc.to_string()` | `doc.format(out)` |

## API Reference

### Constants

- `TOML_STR: u8 = 1`
- `TOML_INT: u8 = 2`
- `TOML_BOOL: u8 = 3`
- `TOML_MAX_ENTRIES: usize = 64`

### Types & Methods

| Symbol / Signature | Description |
| :--- | :--- |
| `TomlEntry { section, key, kind, str_val, int_val, bool_val }` | Typed entry record (`TomlEntry.empty()`). |
| `TomlDoc.new(): TomlDoc` | Create an empty 64-entry stack document. |
| `TomlDoc.parse(src: string): TomlDoc` | Parse TOML text (root keys, `[table]` headers, `#` comments, strings, ints, bools). |
| `doc.len(): usize` / `doc.hasKey(section, key): bool` | Entry count / key existence check. |
| `doc.getStr(section, key): string?` | Query string value (`null` if missing or wrong type). |
| `doc.getInt(section, key): i64?` | Query integer value (`null` if missing or wrong type). |
| `doc.getBool(section, key): bool?` | Query boolean value (`null` if missing or wrong type). |
| `doc.setStr(*_, section, key, val: string): bool` | Upsert string value (`false` if full at 64 entries). |
| `doc.setInt(*_, section, key, val: i64): bool` | Upsert `i64` value. |
| `doc.setBool(*_, section, key, val: bool): bool` | Upsert `bool` value. |
| `doc.format(out: []u8): usize` | Serialize canonical TOML (root keys first, then grouped `[section]` tables) into `out`. |

## Usage Examples

### Parse, Mutate & Format TOML

```cp
import { TomlDoc } from "toml_edit";

fn demoEdit() {
    const src = "title = \"cp-app\"\n[server]\nhost = \"127.0.0.1\"\nport = 8080\ntls = false\n";
    let doc = TomlDoc.parse(src);

    const title = doc.getStr("", "title");           // "cp-app"
    const port = doc.getInt("server", "port");       // 8080

    // Mutate existing key and add a new section entry
    * = doc.setInt("server", "port", 9443);
    * = doc.setBool("server", "tls", true);
    * = doc.setStr("metrics", "bind", "0.0.0.0");

    let out: [512]u8 = undefined;
    const n = doc.format(out[0..]);
    * = title; * = port; * = n;
}
```

### Type-Safe Lookups & Underscore Numbers

```cp
import { TomlDoc } from "toml_edit";

fn demoTyped() {
    const src = "[limits]\nmax_bytes = 1_048_576\nstrict = true\n";
    const doc = TomlDoc.parse(src);

    // Underscore-separated integers are parsed cleanly
    const max_b = doc.getInt("limits", "max_bytes"); // 1048576
    // Type mismatch returns null safely
    const wrong = doc.getStr("limits", "max_bytes"); // null
    * = max_b; * = wrong;
}
```

## Performance & Safety Highlights

- **Zero Heap Allocation**: `TomlDoc` stores `[64]TomlEntry` inline on the stack; `format()` uses a stack `[64]bool = undefined` bitmap for deterministic section grouping without hash maps.
- **Fast `@memcpy` Formatting**: Writes escaped strings, keys, and section headers via `@memcpy` into caller-owned `out: []u8`.
- **Type-Safe Accessors**: Separate `getStr`/`getInt`/`getBool` return optional types (`T?`), preventing silent type confusion.

## License

MIT
