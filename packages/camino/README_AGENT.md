# camino (AI Agent Quick Ref)

- **Import**: `import { Utf8PathBuf, CAMINO_MAX_PATH } from "camino";`
- **Source**: `crates.io:camino` v1 (MIT, zero-heap strictly validated UTF-8 path up to 512 bytes, `undefined` backing buf).

## Core Signatures

```cp
export const CAMINO_MAX_PATH: usize = 512;

export class Utf8PathBuf {
    buf: [512]u8, length: usize,
    fn fromStr(s: string): Utf8PathBuf;
    fn fromBytes(b: []u8): Utf8PathBuf?;
    fn isAbsolute(): bool;
    fn isRelative(): bool;
    fn len(): usize;
    fn isEmpty(): bool;
    fn equals(other: string): bool;
    fn push(_: auto, path: string);
    fn pop(_: auto): bool;
    fn parent(): Utf8PathBuf?;
    fn fileNameInto(dst: []u8): usize?;
    fn fileStemInto(dst: []u8): usize?;
    fn extensionInto(dst: []u8): usize?;
    fn setFileName(_: auto, name: string);
    fn setExtension(_: auto, ext: string): bool;
    fn componentCount(): usize;
    fn hasPrefixPath(base: string): bool;
    fn stripPrefix(base: string): Utf8PathBuf?;
}
```

## Minimal Pattern

```cp
let mut p = Utf8PathBuf.fromStr("/etc/nginx");
p.push("nginx.conf");
let mut stem: [64]u8 = undefined;
if (let n = p.fileStemInto(stem[0..64])) {
    let name = stem[0..n]; // "nginx"
}
```

## Pitfalls

1. **512-byte cap (`CAMINO_MAX_PATH`)**: `fromBytes` returns `null` if `b.len > 512` or UTF-8 is invalid; `fromStr` / `push` clamp to 512 bytes.
2. **Absolute `push` replaces path**: Just like Rust `PathBuf::push`, `p.push("/new/root")` replaces the entire path when the argument starts with `/`.
3. **Slice valid bytes only**: Read raw bytes via `p.buf[0..p.length]`; tail bytes beyond `p.length` are `undefined`.
4. **Mutable variable required**: Declare `let mut p = ...` for `push`, `pop`, `setFileName`, and `setExtension`.
5. **Dotfiles have no extension**: `".bashrc"` returns `null` from `extensionInto` and `".bashrc"` from `fileStemInto`.
