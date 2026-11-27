# commondir (AI Agent Quick Ref)

Zero-heap deepest common parent directory computation + POSIX path normalization and resolution.

## Import
```typescript
import {
    COMMONDIR_MAX_PATH,
    normalizePosixPath, resolvePosixPath,
    commonDirTwo, commonDir,
    commonDirTwoNormalized, commonDirNormalized, commonDirWithBase,
} from "commondir";
```

## API Signatures
- `const COMMONDIR_MAX_PATH: usize` (`1024`)
- `normalizePosixPath(path: string, out: []u8): usize`
- `resolvePosixPath(base: string, rel: string, out: []u8): usize`
- `commonDirTwo(a: string, b: string): string` (zero-copy slice of `a`)
- `commonDir(paths: &[]string): string` (zero-copy slice of `paths[0]`)
- `commonDirTwoNormalized(a: string, b: string, out: []u8): usize`
- `commonDirNormalized(paths: &[]string, out: []u8): usize`
- `commonDirWithBase(base: string, files: &[]string, out: []u8): usize`

## Minimal Example
```typescript
import { commonDir, commonDirWithBase } from "commondir";

fn findRoot(paths: &[]string): string {
    return commonDir(paths);
}

fn findRelRoot(base: string, files: &[]string, out: []u8): usize {
    return commonDirWithBase(base, files, out);
}
```

## Pitfalls & Borrowing Rules
- `commonDir` and `commonDirTwo` do NOT collapse `.` / `..` / `//`; use `commonDirNormalized` or `commonDirTwoNormalized` when inputs may be unnormalized.
- `commonDir(paths)` returns a zero-copy `string` slice borrowing `paths[0]` (or `""` if `paths.len == 0`); do not let it outlive `paths[0]`.
- Internal normalization stack buffers are capped at `COMMONDIR_MAX_PATH` (`1024` bytes) and `128` path segments; size `out` up to `1024` bytes.
- `commonDirWithBase` resolves relative entries against `base` (absolute entries in `files` override `base`), matching `npm/commondir(basedir, relfiles)`.
- Comparison is segment-boundary safe (`/foo` vs `/foobar` yields `/`, never `/foo`); both `/` and `\` are recognized as separators.
