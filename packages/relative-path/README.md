<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **crates.io** [`relative-path`](https://github.com/udoprog/relative-path)（`MIT OR Apache-2.0`），以 Clean-room MIT 纯 CP 零堆方式实现，包名 `@cap/relative-path`。

# `@cap/relative-path`

Portable, platform-independent UTF-8 relative path manipulation, lexical normalization, joining, parent/extension extraction, and relative-to calculation in zero-heap pure CP.

## API

- `RelPath`: Fixed-capacity (`[256]u8`) portable `/`-separated relative path value type:
  - `RelPath.fromStr(s: string): RelPath` — parses & converts `\` separators to `/`, strips leading `/` to keep relative.
  - `asStr(): string`, `len(): usize`, `isEmpty(): bool`
  - `normalize(): RelPath` — lexically resolves `.` and `..` segments while preserving leading `..` hops.
  - `join(other: string): RelPath` — joins another relative path segment.
  - `joinNormalized(other: string): RelPath` — joins and normalizes in one step.
  - `parent(): RelPath?` — returns parent directory or `null` if none.
  - `fileName(): string?` — returns final path component or `null`.
  - `fileStem(): string?` — returns file name without extension.
  - `extension(): string?` — returns file extension without leading `.`.
  - `withExtension(ext: string): RelPath` — replaces or appends extension.
  - `hasPrefixPath(base: string): bool` — component-wise prefix check.
  - `stripPrefix(base: string): RelPath?` — strips leading component prefix.
  - `toLogicalPath(base: string): RelPath` — resolves relative path against a logical base directory.
- Top-level helpers: `normalizeRelPath`, `joinRelPath`, `relativeTo`.
