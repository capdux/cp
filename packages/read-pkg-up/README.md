[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# read-pkg-up

Zero-allocation upward `package.json` / `cap.json` finder, parser, and normalizer in pure CP, composing `find-up` and `read-pkg`.

- **Upstream Alignment**: `npm` `read-pkg-up` (MIT, sindresorhus/read-pkg-up)
- **Package Version**: `1.0.0`
- **Memory Model**: Zero heap allocation — walks virtual/in-memory `PkgFileEntry` slices and writes resolved paths into caller-supplied `[]u8` buffers.

## Installation & Import

```bash
cap add read-pkg-up
```

```cp
import {
    PkgFileEntry,
    ReadPkgUpOptions,
    PkgQuickMeta,
    ReadPkgUpResult,
    parentDir,
    joinPath,
    extractPkgStringField,
    parseQuickPkgMeta,
    readPackageUpNamed,
    readPackageUpInEntries,
    readPackageUpWithOptions,
} from "read-pkg-up";
```

## Core API

| Symbol | Signature | Description |
| --- | --- | --- |
| `PkgFileEntry` | `class PkgFileEntry { path: string, content: string }` | In-memory file path and JSON content entry for deterministic upward lookup. |
| `ReadPkgUpOptions` | `class ReadPkgUpOptions { cwd: string, stop_at: string, normalize: bool, prefer_cap_json: bool }` | Lookup options; `ReadPkgUpOptions.defaults(cwd)` initializes defaults with `cwd`. |
| `PkgQuickMeta` | `class PkgQuickMeta { name, version, description: string, has_name, has_version, has_description: bool }` | Lightweight zero-copy metadata extracted from raw package JSON. |
| `ReadPkgUpResult` | `class ReadPkgUpResult { found: bool, pkg_path_len: usize, pkg: ReadPkgResult }` | Result containing `found`, byte length written into `path_out`, and parsed `ReadPkgResult`. |
| `parentDir` | `fn parentDir(path: string): string?` | Returns parent directory slice of `path`, or `null` at filesystem root. |
| `joinPath` | `fn joinPath(dir: string, name: string, out: []u8): usize` | Joins `dir` and `name` into `out`; returns bytes written. |
| `extractPkgStringField` | `fn extractPkgStringField(json: string, key: string): string?` | Extracts a top-level string field value for `key` from raw JSON without heap allocation. |
| `parseQuickPkgMeta` | `fn parseQuickPkgMeta(json: string): PkgQuickMeta` | Extracts `name`, `version`, and `description` in a single zero-copy pass. |
| `readPackageUpNamed` | `fn readPackageUpNamed(cwd, stop_at, filename: string, normalize: bool, entries: []PkgFileEntry, path_out: []u8): ReadPkgUpResult` | Walks upward from `cwd` to `stop_at` looking for `filename` inside `entries` and parses it. |
| `readPackageUpInEntries` | `fn readPackageUpInEntries(cwd, stop_at: string, normalize: bool, entries: []PkgFileEntry, path_out: []u8): ReadPkgUpResult` | Walks upward looking for `"package.json"` in `entries`. |
| `readPackageUpWithOptions` | `fn readPackageUpWithOptions(opts: ReadPkgUpOptions, entries: []PkgFileEntry, path_out: []u8): ReadPkgUpResult` | Full options-driven upward search (supports `prefer_cap_json` to check `cap.json` before `package.json`). |

## Examples

### 1. Find & Parse Nearest `package.json`

```cp
import { PkgFileEntry, readPackageUpInEntries } from "read-pkg-up";

fn findPkg(): void {
    const entries: [1]PkgFileEntry = [
        PkgFileEntry {
            path: "/repo/package.json",
            content: "{\"name\":\"demo\",\"version\":\"1.0.0\"}",
        },
    ];
    let mut path_buf: [256]u8 = [0; 256];
    const res = readPackageUpInEntries("/repo/src/app", "/repo", true, entries[0..], path_buf[0..]);
    if (res.found) {
        // path_buf[0..res.pkg_path_len] == "/repo/package.json"
    }
}
```

### 2. Prefer `cap.json` with `ReadPkgUpOptions`

```cp
import { PkgFileEntry, ReadPkgUpOptions, readPackageUpWithOptions } from "read-pkg-up";

fn findCapManifest(entries: []PkgFileEntry): void {
    let mut opts = ReadPkgUpOptions.defaults("/repo/packages/app/src");
    opts.stop_at = "/repo";
    opts.prefer_cap_json = true;
    let mut path_buf: [256]u8 = [0; 256];
    const res = readPackageUpWithOptions(opts, entries, path_buf[0..]);
}
```

### 3. Quick Zero-Copy Metadata Extraction

```cp
import { parseQuickPkgMeta, extractPkgStringField } from "read-pkg-up";

fn quickMeta(): void {
    const raw = "{\"name\":\"my-pkg\",\"version\":\"2.1.0\",\"description\":\"Fast\"}";
    const meta = parseQuickPkgMeta(raw);
    const ver = extractPkgStringField(raw, "version"); // Some("2.1.0")
}
```

## Differences from Upstream & Highlights

- **Pure & Deterministic**: Operates on caller-supplied `[]PkgFileEntry` slices instead of direct async filesystem I/O, making monorepo resolution and testing deterministic and zero-allocation.
- **CP Native Support**: `ReadPkgUpOptions.prefer_cap_json` natively supports resolving `cap.json` ahead of `package.json`.
- **Composed Architecture**: Reuses `find-up` upward directory walking and `read-pkg` manifest parsing/normalization.
