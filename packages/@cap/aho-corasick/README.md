# @cap/aho-corasick

Zero-allocation Aho-Corasick multi-pattern O(N + Z) search automaton with LCRS static node pool, BFS failure links, and dictionary suffix links for CP — clean-room MIT parity with Rust `aho-corasick` (1.45B downloads on crates.io).

纯 CP 零堆分配 AC 自动机多模式串线性时间搜索引擎（静态 256 节点 LCRS Trie + 64 模式上限），支持全量重叠后缀匹配，适用于日志关键字扫描、敏感词过滤、WAF 规则匹配与入侵检测。

## Install / 安装

```bash
cap add @cap/aho-corasick
```

## API

| Symbol | Signature | Description |
| --- | --- | --- |
| `AcMatch` | `struct { pattern_id: u16, start: usize, end: usize, found: bool }` | Match span result |
| `AhoCorasick` | `init()`, `addPattern(*_, pat) -> i32`, `build(*_) -> bool`, `patternCount() -> usize` | Automaton builder |
| `isMatch` | `(text: []const u8) -> bool` | Fast existence check |
| `findFirst` | `(text: []const u8) -> AcMatch` | First match with exact `[start, end)` |
| `findAll` | `(text: []const u8, out: []AcMatch) -> usize` | All overlapping matches written into `out` |
| `countMatches` | `(text: []const u8) -> usize` | Total count of all overlapping matches |

## Usage / 使用示例

```cp
import { AhoCorasick, AcMatch } from "@cap/aho-corasick";

let ac = AhoCorasick.init();
* = ac.addPattern("he");
* = ac.addPattern("she");
* = ac.addPattern("his");
* = ac.addPattern("hers");
* = ac.build();

let out: [16]AcMatch = undefined;
const n = ac.findAll("ushers", out[0..16]); // 3 matches: "she", "he", "hers"
```
