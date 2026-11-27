# tsconfig-paths@1.0.0 (Agent Quickref)

> Zero-allocation TypeScript tsconfig.json baseUrl + paths alias wildcard matcher and path resolver for CP | License: MIT
> Import: `import { ... } from "tsconfig-paths";` | Full types: `src/index.d.cp`

## Functions
- `export fn eqStr(a: string, b: string): bool;`
- `export fn startsWith(s: string, prefix: string): bool;`
- `export fn endsWith(s: string, suffix: string): bool;`
- `export fn findStar(s: string): usize?;`
- `export fn matchesPattern(pattern: string, specifier: string): PatternMatch;`
- `export fn appendByte(out: []u8, pos: usize, c: u8): usize;`
- `export fn appendStr(out: []u8, pos: usize, s: string): usize;`
- `export fn writeNormalizedPath(raw: string, out: []u8, start_pos: usize): usize;`
- `export fn expandTargetInto(base_url: string, target: string, captured: string, out: []u8, start_pos: usize): usize;`
- `export fn findBestRule(matcher: PathsMatcher, specifier: string): BestRuleHit;`
- `export fn resolvePath(matcher: PathsMatcher, specifier: string, out: []u8): string?;`
- `export fn resolveAllCandidates(matcher: PathsMatcher, specifier: string, out_buf: []u8): CandidateList;`
