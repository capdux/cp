[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# path-key

Zero-heap cross-platform `PATH` environment variable key lookup, value retrieval, and `PATH` list splitting/joining matching `npm` `path-key` in pure CP.

- **Upstream Alignment**: `npm` `path-key` (MIT, sindresorhus/path-key)
- **Package Version**: `1.0.0`
- **Memory Model**: Zero heap allocation — returns byte spans (`PathSpan`) or writes joined paths into caller-owned `[]u8` buffers.

## Installation & Import

```bash
cap add path-key
```

```cp
import {
    PathSpan,
    EnvPair,
    PathKeyOptions,
    isWindowsPlatform,
    isPathKeyName,
    findPathKeyIndex,
    defaultPathKey,
    pathDelimiter,
    pathKey,
    pathKeyFromPairs,
    findPathValue,
    splitPathEntries,
    joinPathEntries,
    prependPathEntry,
} from "path-key";
```

## Core API

| Symbol | Signature | Description |
| --- | --- | --- |
| `PathSpan` | `class PathSpan { start: usize, len: usize }` | Zero-copy byte range referencing an entry inside a `PATH` string. |
| `EnvPair` | `class EnvPair { key: string, value: string }` | Key-value environment entry pair. |
| `PathKeyOptions` | `class PathKeyOptions { platform: string }` | Platform selector with `PathKeyOptions.posix()` and `PathKeyOptions.win32()`. |
| `isWindowsPlatform` | `fn isWindowsPlatform(platform: string): bool` | Returns `true` for Windows platform strings (e.g. `"win32"`, `"windows"`). |
| `isPathKeyName` | `fn isPathKeyName(key: string): bool` | Case-insensitive check for `"PATH"` (matches `"Path"`, `"path"`, `"PATH"`). |
| `findPathKeyIndex` | `fn findPathKeyIndex(keys: []string, is_windows: bool): i64` | Finds matching `PATH` key index (`-1` if not found); uses case-insensitive last/first match on Windows and exact `"PATH"` on POSIX. |
| `defaultPathKey` | `fn defaultPathKey(is_windows: bool): string` | Returns `"Path"` on Windows and `"PATH"` on POSIX. |
| `pathDelimiter` | `fn pathDelimiter(is_windows: bool): u8` | Returns `';'` on Windows and `':'` on POSIX. |
| `pathKey` | `fn pathKey(keys: []string, platform: string): string` | Resolves the active `PATH` key name from `keys` for `platform`, or falls back to `defaultPathKey`. |
| `pathKeyFromPairs` | `fn pathKeyFromPairs(env: []EnvPair, platform: string): string` | Resolves the active `PATH` key name from `EnvPair` entries for `platform`. |
| `findPathValue` | `fn findPathValue(env: []EnvPair, platform: string): string?` | Returns the value of the resolved `PATH` entry, or `null` if absent. |
| `splitPathEntries` | `fn splitPathEntries(path_val: string, is_windows: bool, out: []PathSpan): usize` | Splits `path_val` by platform delimiter into `out` spans; returns count written. |
| `joinPathEntries` | `fn joinPathEntries(entries: []string, is_windows: bool, out: []u8): usize` | Joins `entries` with platform delimiter into `out`; returns bytes written. |
| `prependPathEntry` | `fn prependPathEntry(entry: string, existing_path: string, is_windows: bool, out: []u8): usize` | Prepends `entry` ahead of `existing_path` into `out`; returns bytes written. |

## Examples

### 1. Resolve `PATH` Key & Value on Windows vs POSIX

```cp
import { EnvPair, pathKeyFromPairs, findPathValue } from "path-key";

fn lookupDemo(): void {
    const env: [2]EnvPair = [
        EnvPair { key: "SystemRoot", value: "C:\\Windows" },
        EnvPair { key: "Path", value: "C:\\bin;C:\\tools" },
    ];
    const key = pathKeyFromPairs(env[0..], "win32"); // "Path"
    const val = findPathValue(env[0..], "win32");    // Some("C:\\bin;C:\\tools")
}
```

### 2. Zero-Copy Split & Stack-Buffer Prepend

```cp
import { PathSpan, splitPathEntries, prependPathEntry } from "path-key";

fn manipulatePath(): void {
    const raw = "/usr/local/bin:/usr/bin";
    let mut spans: [8]PathSpan = [PathSpan { start: 0, len: 0 }; 8];
    const count = splitPathEntries(raw, false, spans[0..]); // 2

    let mut out: [128]u8 = [0; 128];
    const n = prependPathEntry("/opt/cap/bin", raw, false, out[0..]);
    // out[0..n] == "/opt/cap/bin:/usr/local/bin:/usr/bin"
}
```

## Differences from Upstream & Highlights

- **Explicit Environment Passing**: Accepts `keys: []string` or `env: []EnvPair` and explicit `platform: string` instead of reading Node globals.
- **Zero-Allocation Path Utilities**: Extends upstream `path-key` with `splitPathEntries` (span-based, zero-copy), `joinPathEntries`, and `prependPathEntry` for safe `PATH` manipulation without heap allocations.
