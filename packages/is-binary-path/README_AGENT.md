# is-binary-path — AI Agent Quick Reference

Zero-allocation binary file extension checker and `BinaryCategory` classifier (npm `is-binary-path` v3 parity).

## Import

```cp
import {
    BinaryCategory,
    extractExtension,
    classifyBinaryExtension,
    classifyBinaryPath,
    isBinaryExtension,
    isBinaryPath,
    isTextPath,
    isImagePath,
    isMediaPath,
    isArchivePath,
    isExecutablePath,
    isFontPath,
} from "is-binary-path";
```

## API Signatures

```cp
enum BinaryCategory {
    None,
    Image,
    AudioVideo,
    Archive,
    Executable,
    Font,
    DocumentData,
}

fn extractExtension(file_path: string): string;
fn classifyBinaryExtension(ext: string): BinaryCategory;
fn classifyBinaryPath(file_path: string): BinaryCategory;
fn isBinaryExtension(ext: string): bool;
fn isBinaryPath(file_path: string): bool;
fn isTextPath(file_path: string): bool;
fn isImagePath(file_path: string): bool;
fn isMediaPath(file_path: string): bool;
fn isArchivePath(file_path: string): bool;
fn isExecutablePath(file_path: string): bool;
fn isFontPath(file_path: string): bool;
```

## Minimal Example

```cp
import { isBinaryPath, classifyBinaryPath } from "is-binary-path";

const bin = isBinaryPath("assets/icon.PNG");      // true
const cat = classifyBinaryPath("dist/app.wasm");  // BinaryCategory.Executable
```

## Gotchas & Borrow Rules

- `extractExtension` returns a zero-copy sub-slice of `file_path` without the leading `.` (e.g. `"a/b.tar.gz"` -> `"gz"`, `".gitignore"` -> `""`); keep `file_path` alive while holding the slice.
- `classifyBinaryExtension` and `isBinaryExtension` expect `ext` **without** a leading dot (pass `"png"`, not `".png"`).
- `isTextPath("")` returns `false` (empty path is neither text nor binary); paths without extensions (e.g. `"Makefile"`) return `isTextPath == true`.
- Extension-based heuristic only — performs no filesystem I/O or magic-byte sniffing.
