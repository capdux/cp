[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# `@cap/title-case`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:title-case` ([https://github.com/blakeembrey/change-case](https://github.com/blakeembrey/change-case)) / `crates.io:titlecase` ([https://github.com/wezm/titlecase](https://github.com/wezm/titlecase))

Zero-heap-allocation AP/Chicago style English title-case, sentence-case, and capitalization engine in pure CP.

## Features

- **`toTitleCase`**: AP/Chicago style title casing — capitalizes first/last words and words after `:`, preserves all-caps acronyms (`REST`, `API`, `HTTP`, `CP`), lowercases intermediate small words, and handles hyphenated words.
- **`isSmallWord`**: Case-insensitive small-word predicate.
- **`toSentenceCase`**: Capitalizes sentence starts following `.`, `!`, `?`.
- **`capitalizeFirst`**: Uppercases only the first ASCII letter while leaving the rest untouched.
- **Zero Heap Allocation**: Pure CP, stack-only, no FFI.

## API

```cp
import { isSmallWord, toTitleCase, toSentenceCase, capitalizeFirst } from "@cap/title-case";
```

## License

MIT
