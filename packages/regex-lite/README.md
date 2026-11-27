[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# regex-lite

> **Source Attribution**: Clean-room MIT CP implementation of [`crates.io:regex-lite`](https://github.com/rust-lang/regex) (MIT, v0.1 parity) | CP Package Version: `1.0.0` | Dependencies: `regex-syntax`, `memchr`

Zero-allocation, linear-time Thompson NFA (PikeVM) regular expression engine for CP. Composes `regex-syntax` (syntax validation & literal-prefix extraction) and `memchr` (`memmemFind` fast prefix skipping), guaranteeing immunity to catastrophic backtracking (ReDoS).

## Zero-Cost Migration (Rust → CP)

Keep the exact same package name (`"regex-lite"`) — no renaming required.

| Rust (`crates.io:regex-lite`) | CP (`regex-lite`) |
| :--- | :--- |
| `use regex_lite::Regex;` | `import { RegexLite, RegexMatch } from "regex-lite";` |
| `let re = Regex::new(r"\d{4}-\d{2}-\d{2}")?;` | `const re = RegexLite.compile("\\d{4}-\\d{2}-\\d{2}");` |
| `re.is_match(text)` | `re.isMatch(text)` |
| `re.find(text)` | `re.find(text)` |
| `re.find_iter(text)` | `re.findAll(text, out_matches)` |
| `re.replace_all(text, rep)` | `re.replaceAll(text, rep, out)` |

## API Reference

### Constants

- `RL_MAX_INSTS: usize = 128` (max compiled NFA bytecode instructions; max 16 char classes, 64-byte literal prefix)

### Core Types & Methods

| Symbol / Signature | Description |
| :--- | :--- |
| `RegexMatch { matched: bool, start: usize, end: usize }` | Half-open byte span `[start, end)` with `len(): usize`. |
| `RegexLite.compile(pattern: string): RegexLite` | Validate & compile pattern into a stack-resident Thompson NFA. |
| `re.isOk(): bool` / `re.errCode(): i32` | Check compilation status and syntax error code (`0` = ok). |
| `re.isMatch(text: string): bool` | Return `true` if pattern matches anywhere in `text`. |
| `re.find(text: string): RegexMatch` | Find leftmost match in `text`. |
| `re.findAt(text: string, start_pos: usize): RegexMatch` | Find leftmost match starting at or after `start_pos`. |
| `re.matchAnchored(text: string, start_pos: usize): RegexMatch` | Match anchored strictly at `start_pos`. |
| `re.findAll(text: string, out_matches: []RegexMatch): usize` | Collect non-overlapping matches into `out_matches`; returns count. |
| `re.replaceAll(text: string, replacement: string, out: []u8): usize` | Replace all non-overlapping matches into `out`; returns bytes written. |
| `re.splitCount(text: string): usize` | Count segments produced by splitting `text` on matches. |

## Usage Examples

### Basic Search, `findAll` & `replaceAll`

```cp
import { RegexLite, RegexMatch } from "regex-lite";

fn demoSearchReplace() {
    const re = RegexLite.compile("[a-z]+=[0-9]+");
    if (re.isOk()) {
        const text = "a=10, b=20, c=30";
        const first = re.find(text);
        // first.matched == true, text[first.start..first.end] == "a=10"

        let matches: [8]RegexMatch = undefined;
        const count = re.findAll(text, matches[0..]);
        // count == 3

        let out: [64]u8 = undefined;
        const n = re.replaceAll(text, "X", out[0..]);
        // out[0..n] == "X, X, X"
        * = first; * = count; * = n;
    }
}
```

### Anchors, Word Boundaries & Linear-Time Guarantee

```cp
import { RegexLite } from "regex-lite";

fn demoAdvanced() {
    const word_re = RegexLite.compile("\\bfoo\\b");
    const hit1 = word_re.isMatch("bar foo baz"); // true
    const hit2 = word_re.isMatch("foobar");      // false

    // Pathological pattern that blows up backtracking engines runs in O(n * m)
    const redos_safe = RegexLite.compile("(a+)+b");
    const hit3 = redos_safe.isMatch("aaaaaaaaaaaaaaaaaaaa!"); // false in linear time
    * = hit1; * = hit2; * = hit3;
}
```

## Performance & Safety Highlights

- **Guaranteed O(n × m) Thompson NFA**: Lockstep PikeVM execution over `[128]RlInst` eliminates exponential backtracking completely.
- **SIMD-Accelerated Literal Prefix Skipping**: Extracts constant prefixes via `regex-syntax::extractLiteralPrefix` and jumps directly to candidate offsets using `memchr::memmemFind`.
- **Zero Heap Allocation & `undefined` Scratch Lists**: NFA active-state arrays (`cur_list`, `next_list`, `stack: [128]usize = undefined`) and 256-bit character class bitsets (`[4]u64`) live entirely on the stack.

## License

MIT
