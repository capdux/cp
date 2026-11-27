[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# regex

> Native PCRE2 10.44 Perl-compatible regular expression engine with SLJIT JIT acceleration for CP.

- **License**: BSD-3-Clause
- **Type declarations**: [`src/index.d.cp`](./src/index.d.cp)
- **Install / Import**:
  ```cp
  import { cp_pcre2_compile, cp_pcre2_match, cp_pcre2_free } from "regex";
  ```

## Overview

`regex` provides native PCRE2 10.44 bindings with optional SLJIT native JIT compilation for high-throughput pattern matching in CP.

## Quick Example

```cp
import { cp_pcre2_compile, cp_pcre2_match, cp_pcre2_free } from "regex";

fn matchDigits(input: string): bool {
    let pat = "[0-9]+";
    let re = cp_pcre2_compile(pat.ptr, pat.len, 0, 1) ?? return false;
    defer cp_pcre2_free(re);
    let mut start: usize = 0;
    let mut end: usize = 0;
    return cp_pcre2_match(re, input.ptr, input.len, 0, &start, &end) >= 0;
}
```

For full API signatures, see [`src/index.d.cp`](./src/index.d.cp) or [`README_AGENT.md`](./README_AGENT.md).
