[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# camino

> **Source Attribution**: Clean-room MIT CP port of **crates.io** [`camino`](https://github.com/camino-rs/camino) (`v1` / `MIT OR Apache-2.0`), CP package `camino@1.0.0`.

Zero-heap, strictly validated UTF-8 path engine (`Utf8PathBuf`) in pure CP. Reuses `utf8parse` for strict UTF-8 byte validation on raw byte inputs.

## Why CP `camino`?

- **Zero Heap Allocation**: `Utf8PathBuf` stores up to 512 bytes (`CAMINO_MAX_PATH = 512`) inline on stack or inside structs with zero dynamic allocation.
- **Zero-Overhead Stack Initialization**: All `Utf8PathBuf` constructors (`fromStr`, `fromBytes`, `parent`, `stripPrefix`) initialize `buf` with `undefined`, avoiding 512-byte stack zeroing on every path operation.
- **Strict UTF-8 Guarantee**: `Utf8PathBuf.fromBytes(b)` validates UTF-8 via `utf8parse` and returns `null` if invalid, preventing malformed OS byte sequences from entering UTF-8 APIs.
- **Rich Rust `camino` Ergonomics**: Supports `push`, `pop`, `parent`, `fileNameInto`, `fileStemInto`, `extensionInto`, `setFileName`, `setExtension`, `componentCount`, `hasPrefixPath`, and `stripPrefix`.

## Zero-Cost Migration (Rust → CP)

Keep the exact same package name (`camino`) — no renaming required.

**Rust (`Cargo.toml` / `main.rs`):**
```rust
use camino::Utf8PathBuf;
let mut p = Utf8PathBuf::from("/usr/local");
p.push("bin/cap");
```

**CP (`cap.json` / `main.cp`):**
```typescript
import { Utf8PathBuf, CAMINO_MAX_PATH } from "camino";
```

## Quick Start

### Build, Inspect & Mutate UTF-8 Paths

```typescript
import { Utf8PathBuf } from "camino";

export fn demoPath(): bool {
    let mut p = Utf8PathBuf.fromStr("/var/log");
    p.push("app/server.log");
    // p == "/var/log/app/server.log"

    let mut ext_buf: [32]u8 = undefined;
    let ext_len = p.extensionInto(ext_buf[0..32]); // Some(3) -> "log"

    _ = p.setExtension("txt"); // "/var/log/app/server.txt"

    if (let rel = p.stripPrefix("/var/log")) {
        return rel.equals("app/server.txt") && ext_len != null;
    }
    return false;
}
```

### Validate Raw Bytes into `Utf8PathBuf`

```typescript
import { Utf8PathBuf } from "camino";

export fn fromRawPath(raw: []u8): bool {
    if (let p = Utf8PathBuf.fromBytes(raw)) {
        return p.isAbsolute() && p.componentCount() > 0;
    }
    return false; // invalid UTF-8 or length > CAMINO_MAX_PATH
}
```

## API Reference

- **Constant**: `CAMINO_MAX_PATH: usize = 512`
- **`Utf8PathBuf` Constructors**:
  - `Utf8PathBuf.fromStr(s: string): Utf8PathBuf`
  - `Utf8PathBuf.fromBytes(b: []u8): Utf8PathBuf?` — validates UTF-8 and max length (512)
- **Inspection**: `isAbsolute(): bool`, `isRelative(): bool`, `len(): usize`, `isEmpty(): bool`, `equals(other: string): bool`, `componentCount(): usize`, `hasPrefixPath(base: string): bool`
- **Extraction (Zero-Alloc)**: `parent(): Utf8PathBuf?`, `stripPrefix(base: string): Utf8PathBuf?`, `fileNameInto(dst: []u8): usize?`, `fileStemInto(dst: []u8): usize?`, `extensionInto(dst: []u8): usize?`
- **Mutation**: `push(path: string)`, `pop(): bool`, `setFileName(name: string)`, `setExtension(ext: string): bool`

## License

MIT
