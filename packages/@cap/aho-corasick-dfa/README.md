# `@cap/aho-corasick-dfa`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:aho-corasick` ([https://github.com/BurntSushi/aho-corasick](https://github.com/BurntSushi/aho-corasick))（原库名 `aho-corasick` → CP 包名 `@cap/aho-corasick-dfa`）

Zero-allocation multi-pattern Aho-Corasick automaton engine for CP (clean-room MIT, inspired by `aho-corasick` v1).

## Features

- **Zero Heap Allocations**: Compact fixed-pool automaton supporting up to **256 states** and **32 patterns** entirely on the stack.
- **O(N) Linear-Time Scanning**: Trie construction followed by BFS failure-link and output-link compilation (`build()`).
- **ASCII Case-Insensitive Matching**: Cross-package reuse of `foldAsciiByte` from `@cap/unicase`.
- **Full Overlapping Match Reporting**: `findAll` and `countMatches` report all overlapping pattern hits (ordered by ascending `end` offset, then ascending `pattern_id`).

## API

```cp
pub struct AcMatch {
    pub pattern_id: usize,
    pub start: usize,
    pub end: usize,
}

pub struct AhoCorasick {
    pub static fn new(ignore_case: bool): AhoCorasick
    pub fn addPattern(*_, pat: string): bool
    pub fn build(*_)
    pub fn patternCount(): usize
    pub fn stateCount(): usize
    pub fn isMatch(haystack: string): bool
    pub fn findFirst(haystack: string): AcMatch?
    pub fn findAll(haystack: string, out_matches: []AcMatch): usize
    pub fn countMatches(haystack: string): usize
}
```

### Semantics Notes

- Call `build()` after adding all patterns and before calling `isMatch`, `findFirst`, `findAll`, or `countMatches`.
- `findFirst(haystack)` returns the earliest-ending match; when multiple patterns end at the same index, the leftmost-start (longest) match wins, breaking ties by lowest `pattern_id`.
- `findAll(haystack, out_matches)` emits **all overlapping matches** up to `out_matches.len` and returns the number of entries written.
- `addPattern` rejects empty patterns (`""`) and returns `false` without mutating the trie if state/pattern capacity would be exceeded.

## Usage

```cp
import { AcMatch, AhoCorasick } from "@cap/aho-corasick-dfa";

fn example() {
    let ac = AhoCorasick.new(false);
    ac.addPattern("he");
    ac.addPattern("she");
    ac.addPattern("his");
    ac.addPattern("hers");
    ac.build();

    let hits: [8]AcMatch = undefined;
    const n = ac.findAll("ushers", hits[0..8]); // 3 matches: "she", "he", "hers"
}
```

## License

MIT
