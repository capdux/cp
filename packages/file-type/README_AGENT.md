# file-type — AI Agent Quick Ref

Zero-allocation binary file magic bytes signature sniffer (25+ image, archive, document, executable, audio, video, and database formats) for CP. (`v1.0.0`, `MIT`)

## Import

```cp
import {
    FileType,
    mbMatch,
    mbEqAt,
    detect,
    isImage,
    isArchive,
    isExecutable,
} from "file-type";
```

## Core Types & Constants

```cp
class FileType { ext: string, mime: string, kind: u8, matched: bool }
```

## API Signatures

```cp
fn mbMatch(ext: string, mime: string, kind: u8): FileType;
fn mbEqAt(data: &[]u8, offset: usize, lit: &[]u8): bool;
fn detect(data: &[]u8): FileType;
fn isImage(data: &[]u8): bool;
fn isArchive(data: &[]u8): bool;
fn isExecutable(data: &[]u8): bool;
```

## Minimal Usage

```cp
import { mbMatch } from "file-type";

pub fn main() {
    let result = mbMatch("example", "example", 0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"file-type"`; install via `cap add file-type`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
