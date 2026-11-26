# @cap/word-count

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:words-count` ([https://github.com/magiclen/words-count](https://github.com/magiclen/words-count)) / `npm:word-count` ([https://github.com/yuehu/word-count](https://github.com/yuehu/word-count))（原库名 `words-count` → CP 包名 `@cap/word-count`）

Zero-allocation multilingual (Latin + CJK) word/character/line counter and reading-time estimator for CP.

## Features

- **Zero heap allocation**: single-pass O(N) UTF-8 scanner computing `bytes`, `chars`, `words`, `cjk_chars`, `latin_words`, and `lines`.
- **CJK-aware**: recognizes CJK Unified Ideographs (`U+4E00..U+9FFF`), Extension A (`U+3400..U+4DBF`), Hiragana/Katakana (`U+3040..U+30FF`), and Hangul (`U+AC00..U+D7AF`).
- **Reading-time estimation**: `estimateReadingSeconds(text, wpm)` with ceiling rounding and zero-safe default WPM.

## Usage

```cp
import { analyze, estimateReadingSeconds } from "@cap/word-count";

const stats = analyze("你好世界，欢迎使用CP语言");
// stats.cjk_chars == 10, stats.latin_words == 1, stats.words == 11
const secs = estimateReadingSeconds("Hello world from CP", 200 as u32);
```
