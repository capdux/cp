# @cap/bmh-search

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:boyer-moore-magiclen` ([https://github.com/magiclen/boyer-moore-magiclen](https://github.com/magiclen/boyer-moore-magiclen))（原库名 `boyer-moore-magiclen` → CP 包名 `@cap/bmh-search`）

Zero-heap Boyer-Moore-Horspool sublinear substring search in pure CP.

## Features

- `compilePattern(needle)` & `findWithCompiled(haystack, needle, compiled)`: precomputed 256-byte bad-character shift table reusable across multiple texts
- `findFirst(haystack, needle)` & `findLast(haystack, needle)`: forward and reverse BMH search
- `countNonOverlapping(haystack, needle)` & `findIgnoreCaseAscii(haystack, needle)`

## Usage

```cp
import { compilePattern, findWithCompiled, findLast } from "@cap/bmh-search";

const pat = compilePattern("needle");
const pos = findWithCompiled("find a needle in haystack", "needle", pat); // 7
```
