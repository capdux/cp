# read-pkg-up — AI Agent Quick Ref

Zero-allocation upward `package.json`/`cap.json` finder and parser composing `find-up` and `read-pkg` (npm `read-pkg-up` parity).

## Import

```cp
import {
    PkgFileEntry, ReadPkgUpOptions, PkgQuickMeta, ReadPkgUpResult,
    parentDir, joinPath, extractPkgStringField, parseQuickPkgMeta,
    readPackageUpNamed, readPackageUpInEntries, readPackageUpWithOptions,
} from "read-pkg-up";
```

## API Signatures

```cp
export class PkgFileEntry { path: string, content: string }
export class ReadPkgUpOptions {
    cwd: string, stop_at: string, normalize: bool, prefer_cap_json: bool,
    static fn defaults(cwd: string): ReadPkgUpOptions;
}
export class PkgQuickMeta {
    name: string, version: string, description: string,
    has_name: bool, has_version: bool, has_description: bool,
}
export class ReadPkgUpResult {
    found: bool, pkg_path_len: usize, pkg: ReadPkgResult,
}
export fn parentDir(path: string): string?;
export fn joinPath(dir: string, name: string, out: []u8): usize;
export fn extractPkgStringField(json: string, key: string): string?;
export fn parseQuickPkgMeta(json: string): PkgQuickMeta;
export fn readPackageUpNamed(cwd: string, stop_at: string, filename: string, normalize: bool, entries: []PkgFileEntry, path_out: []u8): ReadPkgUpResult;
export fn readPackageUpInEntries(cwd: string, stop_at: string, normalize: bool, entries: []PkgFileEntry, path_out: []u8): ReadPkgUpResult;
export fn readPackageUpWithOptions(opts: ReadPkgUpOptions, entries: []PkgFileEntry, path_out: []u8): ReadPkgUpResult;
```

## Minimal Example

```cp
import { PkgFileEntry, ReadPkgUpOptions, readPackageUpWithOptions } from "read-pkg-up";

fn demo(): void {
    const entries: [1]PkgFileEntry = [
        PkgFileEntry { path: "/app/package.json", content: "{\"name\":\"demo\"}" },
    ];
    let mut path_buf: [128]u8 = [0; 128];
    const res = readPackageUpWithOptions(ReadPkgUpOptions.defaults("/app/src"), entries[0..], path_buf[0..]);
    // res.found == true, path_buf[0..res.pkg_path_len] == "/app/package.json"
}
```

## Caveats & Borrowing Rules

- **No implicit FS I/O**: Pass candidate manifests via `entries: []PkgFileEntry`; returned `PkgQuickMeta` and `extractPkgStringField` borrow directly from `content`.
- **Mutable `path_out`**: Provide a mutable stack buffer (`let mut path_buf: [256]u8 = [0; 256]`); read the matched path via `path_buf[0..res.pkg_path_len]` only when `res.found == true`.
- **Stop boundary**: Pass `stop_at: ""` to walk all the way to filesystem root, or a directory prefix to bound traversal.
- **`prefer_cap_json`**: When `true` in `ReadPkgUpOptions`, checks `cap.json` before `package.json` at each directory level.
