# regex@10.44.0 (Agent Quickref)

> PCRE2 10.44 Perl-compatible regular expression engine with SLJIT native JIT acceleration for CP | License: BSD-3-Clause
> Import: `import { ... } from "regex";` | Full types: `src/index.d.cp`

## Functions
- `export fn cp_pcre2_compile(pattern: [*]&u8!, pat_len: usize, opts: u32, jit: i32): *auto!?;`
- `export fn cp_pcre2_match(re: *auto!?, subj: [*]&u8!, subj_len: usize, start_off: usize, out_start: *usize!?, out_end: *usize!?): i32;`
- `export fn cp_pcre2_free(re: *auto!?);`
- `export fn cp_pcre2_is_jit(re: *auto!?): i32;`
- `export fn cp_pcre2_version(): [*:0]&u8!?;`
