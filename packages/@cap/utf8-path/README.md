# @cap/utf8-path

Zero-allocation UTF-8 path manipulation, normalization, relative path calculation, and traversal-safe sandboxing (`safeJoin`) for CP — clean-room MIT parity with Rust `camino` (341M downloads on crates.io).

纯 CP 零堆分配 UTF-8 路径操作与安全沙箱引擎，支持 POSIX/Windows 路径规范化、组件剖析、最小相对路径计算及严格防路径穿越 `safeJoin`。

## Install / 安装

```bash
cap add @cap/utf8-path
```

## API

| Symbol | Signature | Description |
| --- | --- | --- |
| `isAbsolute` | `(p: string) -> bool` | Detect POSIX `/` or Windows `C:\` / `C:/` absolute paths |
| `fileName` / `fileStem` / `extension` | `(p: string) -> string?` | Zero-copy filename, stem (dotfile-aware), and extension extraction |
| `parentDir` | `(p: string) -> string?` | Zero-copy parent directory slice |
| `normalizePath` | `(p: string, out: []u8) -> string` | Normalize `\` → `/`, collapse `//`, resolve `.` and `..` |
| `joinPath` | `(base: string, rel: string, out: []u8) -> string` | Join and normalize two paths |
| `safeJoin` | `(root: string, untrusted_rel: string, out: []u8) -> string?` | Sandbox join rejecting `..` traversal, absolute overrides & NUL bytes |
| `relativePath` | `(from_dir: string, to_path: string, out: []u8) -> string` | Minimal relative path from `from_dir` to `to_path` |
| `withExtension` | `(p: string, new_ext: string, out: []u8) -> string` | Replace or strip file extension |

## Usage / 使用示例

```cp
import { normalizePath, safeJoin, relativePath } from "@cap/utf8-path";

let buf: [256]u8 = undefined;
const safe = safeJoin("/srv/www", "assets/../img/logo.png", buf[0..256]); // "/srv/www/img/logo.png"
const blocked = safeJoin("/srv/www", "../../etc/passwd", buf[0..256]);    // null
```
