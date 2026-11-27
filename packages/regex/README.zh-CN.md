[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# regex

> 版本：`10.44.0` | 许可证：`BSD-3-Clause` | 类型声明：[src/index.d.cp](./src/index.d.cp)

PCRE2 10.44 Perl-compatible regular expression engine with SLJIT native JIT acceleration for CP

## 安装与导入

```bash
cap add regex
```

```cp
import { cp_pcre2_compile } from "regex";
```

## 核心导出 API

- `export fn cp_pcre2_compile(pattern: [*]&u8!, pat_len: usize, opts: u32, jit: i32): *auto!?;`
- `export fn cp_pcre2_match(re: *auto!?, subj: [*]&u8!, subj_len: usize, start_off: usize, out_start: *usize!?, out_end: *usize!?): i32;`
- `export fn cp_pcre2_free(re: *auto!?);`
- `export fn cp_pcre2_is_jit(re: *auto!?): i32;`
- `export fn cp_pcre2_version(): [*:0]&u8!?;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`regex@10.44.0`)，许可证：MIT
