# ignore@1.0.0 (Agent Quickref)

> Zero-heap .gitignore specification rule engine (clean-room CP implementation of npm ignore) | License: MIT
> Import: `import { ... } from "ignore";` | Full types: `src/index.d.cp`
> Deps: `minimatch`, `normalize-path`

## Functions
- `export fn igTrimRightSpaces(s: string): usize;`
- `export fn igHasInnerSlash(s: string, start: usize, end: usize): bool;`
- `export fn igRuleMatches(rule: IgnoreRule, rel_path: string, is_dir: bool, case_insensitive: bool): bool;`
- `export fn igMatchSingle(pat: &[]u8, anchored: bool, path: string, case_insensitive: bool): bool;`
