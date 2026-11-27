# path-key — AI Agent Quick Ref

Zero-heap cross-platform `PATH` env key resolver and `PATH` list splitter/joiner (npm `path-key` parity).

## Import

```cp
import {
    PathSpan, EnvPair, PathKeyOptions,
    isWindowsPlatform, isPathKeyName, findPathKeyIndex,
    defaultPathKey, pathDelimiter, pathKey, pathKeyFromPairs,
    findPathValue, splitPathEntries, joinPathEntries, prependPathEntry,
} from "path-key";
```

## API Signatures

```cp
export class PathSpan { start: usize, len: usize }
export class EnvPair { key: string, value: string }
export class PathKeyOptions {
    platform: string,
    static fn posix(): PathKeyOptions;
    static fn win32(): PathKeyOptions;
}
export fn isWindowsPlatform(platform: string): bool;
export fn isPathKeyName(key: string): bool;
export fn findPathKeyIndex(keys: []string, is_windows: bool): i64;
export fn defaultPathKey(is_windows: bool): string;
export fn pathDelimiter(is_windows: bool): u8;
export fn pathKey(keys: []string, platform: string): string;
export fn pathKeyFromPairs(env: []EnvPair, platform: string): string;
export fn findPathValue(env: []EnvPair, platform: string): string?;
export fn splitPathEntries(path_val: string, is_windows: bool, out: []PathSpan): usize;
export fn joinPathEntries(entries: []string, is_windows: bool, out: []u8): usize;
export fn prependPathEntry(entry: string, existing_path: string, is_windows: bool, out: []u8): usize;
```

## Minimal Example

```cp
import { EnvPair, pathKeyFromPairs, findPathValue } from "path-key";

fn demo(): void {
    const env: [1]EnvPair = [EnvPair { key: "Path", value: "C:\\bin" }];
    const k = pathKeyFromPairs(env[0..], "win32"); // "Path"
    const v = findPathValue(env[0..], "win32");    // "C:\\bin"
}
```

## Caveats & Borrowing Rules

- **Case sensitivity**: On Windows (`is_windows == true`), `PATH` lookup is ASCII case-insensitive (`"Path"`, `"path"`, `"PATH"`); on POSIX it matches exact `"PATH"`.
- **Span slicing**: `PathSpan` returned by `splitPathEntries` indexes into the original `path_val` via `path_val[span.start .. span.start + span.len]`; keep `path_val` alive while using spans.
- **Mutable outputs**: `splitPathEntries` writes into `out: []PathSpan`; `joinPathEntries` and `prependPathEntry` write into `out: []u8` and return bytes written.
- **Not-found sentinel**: `findPathKeyIndex` returns `-1` when absent; `findPathValue` returns `null`.
