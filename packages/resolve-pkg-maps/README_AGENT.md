# resolve-pkg-maps@1.0.0 (Agent Quickref)

> Zero-heap package.json exports subpath wildcard and condition resolver | License: MIT
> Import: `import { ... } from "resolve-pkg-maps";` | Full types: `src/index.d.cp`

## Functions
- `export fn rpmEq(a: string, b: string): bool;`
- `export fn rpmFindStar(s: string): usize;`
- `export fn rpmHasParentTraversal(path: string): bool;`
- `export fn rpmConditionRank(cond: string, conditions: &[]string): usize;`
- `export fn matchSubpathPattern(pattern: string, subpath: string, matched_star: []u8): i64;`
- `export fn resolveExportTarget(rules: &[]ExportRule, subpath: string, conditions: &[]string, out: []u8): usize;`
