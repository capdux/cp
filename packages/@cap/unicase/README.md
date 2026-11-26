# @cap/unicase

Zero-allocation case-insensitive string wrapper, comparison, search, and FNV-1a 64-bit hashing for CP — clean-room MIT parity with Rust `unicase` (516M downloads on crates.io).

纯 CP 零堆分配大小写不敏感字符串包装、三向比较、子串查找与 FNV-1a 64-bit 哈希引擎，专为 HTTP Header、DNS 域名、Email 与不区分大小写路径场景打造。

## Install / 安装

```bash
cap add @cap/unicase
```

## API

| Symbol | Signature | Description |
| --- | --- | --- |
| `UniCase` | `struct { inner: string }` | Zero-copy wrapper (`new`, `asStr`, `equals`, `cmp`, `hash64`) |
| `eqIgnoreAsciiCase` | `(a: string, b: string) -> bool` | Zero-alloc ASCII case-insensitive equality |
| `cmpIgnoreAsciiCase` | `(a: string, b: string) -> i32` | Three-way comparison (`-1` / `0` / `1`) |
| `startsWithIgnoreCase` / `endsWithIgnoreCase` | `(s: string, fix: string) -> bool` | Prefix / suffix matching ignoring ASCII case |
| `indexOfIgnoreCase` / `containsIgnoreCase` | `(haystack: string, needle: string) -> usize? / bool` | Case-insensitive substring search |
| `hashIgnoreCase64` | `(s: string) -> u64` | Case-folded FNV-1a 64-bit hash |
| `toLowerAscii` / `toUpperAscii` | `(s: string, out: []u8) -> string` | ASCII case conversion preserving UTF-8 bytes |

## Usage / 使用示例

```cp
import { UniCase, containsIgnoreCase, hashIgnoreCase64 } from "@cap/unicase";

const h1 = UniCase.new("Content-Type");
const h2 = UniCase.new("content-type");
// h1.equals(h2) == true && h1.hash64() == h2.hash64()
```
