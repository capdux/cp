[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# is-binary-path

Zero-allocation binary file extension checker and media/archive/executable/font category classifier for CP, matching npm [`is-binary-path`](https://github.com/sindresorhus/is-binary-path) v3 (107M+/wk) and `binary-extensions`.

## Source Alignment

- **Upstream**: npm `is-binary-path` v3.0.0 (`sindresorhus/is-binary-path`, MIT) + `binary-extensions`
- **CP Package**: `is-binary-path` v3.0.0 (clean-room MIT in pure CP)
- **Design**: Zero heap allocations, case-insensitive extension matching, plus structured `BinaryCategory` classification and category convenience predicates.

## Installation & Import

```bash
cap add is-binary-path
```

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

## Core API

### Enum

| Symbol | Variants | Description |
| :--- | :--- | :--- |
| `BinaryCategory` | `None \| Image \| AudioVideo \| Archive \| Executable \| Font \| DocumentData` | High-level binary file classification (`.None` means not a known binary extension) |

### Functions

| Function | Signature | Description |
| :--- | :--- | :--- |
| `isBinaryPath` | `fn isBinaryPath(file_path: string): bool` | Returns `true` if `file_path` ends with a known binary file extension |
| `isTextPath` | `fn isTextPath(file_path: string): bool` | Returns `true` if `file_path` is non-empty and not a known binary extension |
| `isBinaryExtension` | `fn isBinaryExtension(ext: string): bool` | Returns `true` if `ext` (without leading `.`, case-insensitive) is a known binary extension |
| `extractExtension` | `fn extractExtension(file_path: string): string` | Zero-copy slice of the file extension without leading `.` (ignores dotfiles like `.gitignore`, handles `/` and `\`) |
| `classifyBinaryPath` | `fn classifyBinaryPath(file_path: string): BinaryCategory` | Classifies `file_path` into a `BinaryCategory` |
| `classifyBinaryExtension` | `fn classifyBinaryExtension(ext: string): BinaryCategory` | Classifies `ext` (without leading `.`, case-insensitive) into a `BinaryCategory` |
| `isImagePath` | `fn isImagePath(file_path: string): bool` | Returns `true` iff `classifyBinaryPath(file_path) == .Image` |
| `isMediaPath` | `fn isMediaPath(file_path: string): bool` | Returns `true` iff `classifyBinaryPath(file_path) == .AudioVideo` |
| `isArchivePath` | `fn isArchivePath(file_path: string): bool` | Returns `true` iff `classifyBinaryPath(file_path) == .Archive` |
| `isExecutablePath` | `fn isExecutablePath(file_path: string): bool` | Returns `true` iff `classifyBinaryPath(file_path) == .Executable` |
| `isFontPath` | `fn isFontPath(file_path: string): bool` | Returns `true` iff `classifyBinaryPath(file_path) == .Font` |

## Examples

### 1. Filtering Binary vs. Text Paths

```cp
import { isBinaryPath, isTextPath } from "is-binary-path";

pub fn shouldIndexFile(path: string): bool {
    if (isBinaryPath(path)) {
        return false; // skips .png, .zip, .wasm, .pdf, .exe, ...
    }
    return isTextPath(path); // true for "src/main.cp", ".gitignore", "Makefile"
}
```

### 2. Category Dispatch

```cp
import { BinaryCategory, classifyBinaryPath } from "is-binary-path";

pub fn assetBucket(path: string): string {
    return switch (classifyBinaryPath(path)) {
        .Image => "images",
        .AudioVideo => "media",
        .Font => "fonts",
        .Archive => "archives",
        .Executable => "bins",
        .DocumentData => "docs",
        .None => "text",
    };
}
```

### 3. Direct Extension Extraction

```cp
import { extractExtension, isBinaryExtension } from "is-binary-path";

pub fn inspectExt(path: string): bool {
    const ext = extractExtension(path); // "assets/logo.PNG" -> "PNG"
    return isBinaryExtension(ext);      // true (case-insensitive)
}
```

## Differences from npm `is-binary-path` & Safety Highlights

- **Zero Allocation & Zero Copy**: `extractExtension` returns a borrowed slice of `file_path`; all comparisons are ASCII case-insensitive with no temporary lowercased strings.
- **Built-In Category Taxonomy**: Extends boolean `isBinaryPath` with `BinaryCategory` (`Image`, `AudioVideo`, `Archive`, `Executable`, `Font`, `DocumentData`) and dedicated `is*Path` predicates.
- **Cross-Platform Path Separators**: Correctly handles both POSIX `/` and Windows `\` directory separators as well as hidden dotfiles (e.g. `.env` has empty extension `""`).
