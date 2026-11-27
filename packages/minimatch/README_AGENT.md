# minimatch@1.0.0 — AI Agent Quick Ref

- **Source**: `npm:minimatch` (`isaacs/minimatch`, ISC) | **Deps**: `micromatch`, `picomatch`
- **Import**: `import { minimatch, minimatchWithOptions, minimatchList, minimatchFilterAll, minimatchRejectAll, MinimatchOptions, isCommentPattern, isNegatedPattern } from "minimatch";`
- **Full Types**: `packages/minimatch/src/index.d.cp`

## Core Types & Functions
```cp
class MinimatchOptions {
    dot: bool, nocase: bool, match_base: bool, noglobstar: bool,
    nonegate: bool, nocomment: bool, flip_negate: bool,
    fn defaults(): MinimatchOptions
}

fn isCommentPattern(pattern: string, opts: MinimatchOptions): bool
fn isNegatedPattern(pattern: string, opts: MinimatchOptions): bool
fn minimatch(path: string, pattern: string): bool
fn minimatchWithOptions(path: string, pattern: string, opts: MinimatchOptions): bool
fn minimatchList(paths: &[]string, pattern: string, opts: MinimatchOptions, out_indices: []usize): usize
fn minimatchFilterAll(paths: &[]string, patterns: &[]string, opts: MinimatchOptions, out_indices: []usize): usize
fn minimatchRejectAll(paths: &[]string, patterns: &[]string, opts: MinimatchOptions, out_indices: []usize): usize
```

## Minimal Pattern
```cp
import { minimatch, minimatchList, MinimatchOptions } from "minimatch";

const hit = minimatch("a/b/c.cp", "**/*.cp"); // true
let opts = MinimatchOptions.defaults();
opts.nocase = true;
const paths = ["README.MD", "src/main.cp"];
let idx: [4]usize = undefined;
const count = minimatchList(paths[0..], "*.md", opts, idx[0..]); // count == 1, idx[0] == 0
```

## Pitfalls & Notes
1. **Comment Patterns**: Leading `#` always returns `false` unless `opts.nocomment = true`.
2. **Negation**: Leading `!` inverts the match unless `opts.nonegate = true`; multiple `!` flip parity. Set `opts.flip_negate = true` to inspect the un-negated result.
3. **Output Indices**: `minimatchList`/`minimatchFilterAll`/`minimatchRejectAll` write matching indices into `out_indices` and stop if `out_indices` is full; allocate `out_indices.len >= paths.len` for full results.
4. **Helper `miniToPico`**: Exposed for interop with `picomatch::PicoOptions`; normal callers only need `MinimatchOptions`.
