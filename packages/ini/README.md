[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# ini

> **Source Attribution**: Clean-room MIT CP implementation of [`npm:ini`](https://github.com/npm/ini) (ISC) | CP Package Version: `1.0.0` | Zero external dependencies

Zero-heap INI / `.gitconfig` / `.npmrc` / `.editorconfig` parser and writer for CP. Supports `[section]` headers, `#` and `;` comments, quoted values, typed boolean/integer getters, and auto-quoting serialization directly into caller-owned buffers.

## Zero-Cost Migration (TypeScript / JS → CP)

Keep the exact same package name (`"ini"`) — no renaming required.

| TypeScript / Node.js (`npm:ini`) | CP (`ini`) |
| :--- | :--- |
| `import ini from "ini";` | `import { IniDoc, IniWriter } from "ini";` |
| `const cfg = ini.parse(text);` | `const doc = IniDoc.parse(text);` |
| `cfg.server.port` | `doc.getInt(text, "server", "port", 8080)` |
| `cfg.server.tls` | `doc.getBool(text, "server", "tls", false)` |
| `ini.stringify(obj)` | `IniWriter.init(buf)` + `writeSection` / `writeEntry` |

## API Reference

### Constants

- `MAX_ENTRIES: usize = 128`

### Types & Methods

| Symbol / Signature | Description |
| :--- | :--- |
| `IniEntry { section_start, section_len, key_start, key_len, val_start, val_len }` | Offset/length index entry referencing `src` without copying. |
| `IniDoc.parse(src: &[]u8): IniDoc` | Parse INI bytes into up to 128 offset-indexed entries. |
| `doc.len(): usize` | Number of parsed key-value entries. |
| `doc.findIndex(src: &[]u8, section: &[]u8, key: &[]u8): i64` | Return entry index (`>= 0`) or `-1` if missing. |
| `doc.has(src: &[]u8, section: &[]u8, key: &[]u8): bool` | Return `true` if `(section, key)` exists. |
| `doc.get(src: &[]u8, section: &[]u8, key: &[]u8): &[]u8` | Return borrowed value slice (empty slice if missing). |
| `doc.getBool(src, section, key, default_val: bool): bool` | Parse `true`/`false`/`yes`/`no`/`on`/`off`/`1`/`0` (case-insensitive). |
| `doc.getInt(src, section, key, default_val: i64): i64` | Parse decimal `i64` with fallback to `default_val`. |
| `IniWriter.init(buf: []u8): IniWriter` | Create a zero-heap INI writer backed by `buf`. |
| `w.writeSection(*_, name: &[]u8): bool` | Emit `[name]\n` header; returns `false` on overflow. |
| `w.writeEntry(*_, key: &[]u8, value: &[]u8): bool` | Emit `key = value\n` (auto-quotes values with `;`, `#`, or edge spaces). |
| `w.written(): &[]u8` / `w.len(): usize` | Return written slice or byte length. |

## Usage Examples

### Parsing & Typed Getters

```cp
import { IniDoc } from "ini";

fn demoParse() {
    const src = "; app config\nname = demo\n[server]\nhost = 127.0.0.1\nport = 9000\ntls = true\n";
    const doc = IniDoc.parse(src);

    // Root (global) key uses empty section ""
    const name = doc.get(src, "", "name");          // "demo"
    const port = doc.getInt(src, "server", "port", 8080); // 9000
    const tls = doc.getBool(src, "server", "tls", false); // true
    const timeout = doc.getInt(src, "server", "timeout", 30); // 30 (default)
    * = name; * = port; * = tls; * = timeout;
}
```

### Writing INI with Auto-Quoting & Overflow Detection

```cp
import { IniWriter } from "ini";

fn demoWrite() {
    let buf: [256]u8 = undefined;
    let w = IniWriter.init(buf[0..]);

    * = w.writeSection("database");
    * = w.writeEntry("host", "localhost");
    // Contains '#' -> automatically wrapped in double quotes
    * = w.writeEntry("note", "primary #1");

    if (!w.overflow) {
        const out = w.written();
        * = out;
    }
}
```

## Performance & Safety Highlights

- **Zero-Copy Offset Indexing**: `IniDoc` stores `[128]IniEntry = undefined` (byte offsets into `src`) instead of copying strings — zero heap allocation and zero startup zeroing.
- **Fast `@memcpy` Writer**: `IniWriter.writeSlice` uses `@memcpy` with explicit bounds checks and sticky `overflow` flag.
- **Borrow Safety**: `doc.get(src, ...)` returns a borrowed `&[]u8` slice directly into `src`; caller owns `src` lifetime cleanly.

## License

MIT
