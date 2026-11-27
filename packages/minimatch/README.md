[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# minimatch

> **Source Attribution**: Clean-room MIT CP implementation of [`npm:minimatch`](https://github.com/isaacs/minimatch) (ISC) | CP Package Version: `1.0.0` | Dependencies: `micromatch`, `picomatch`

Zero-heap glob pattern matching utility for CP. Supports globstars (`**`), wildcards (`*`, `?`), character classes, `#` comment lines, `!` negation, `flip_negate`, `match_base`, case-insensitive matching, and batch path filtering.

## Zero-Cost Migration (TypeScript / JS → CP)

Keep the exact same package name (`"minimatch"`) — no renaming required.

| TypeScript / Node.js (`npm:minimatch`) | CP (`minimatch`) |
| :--- | :--- |
| `import { minimatch } from "minimatch";` | `import { minimatch, minimatchWithOptions, MinimatchOptions } from "minimatch";` |
| `minimatch("src/app.ts", "**/*.ts")` | `minimatch("src/app.ts", "**/*.ts")` |
| `minimatch("README.md", "*.MD", { nocase: true })` | `minimatchWithOptions("README.md", "*.MD", opts)` |
| `paths.filter(p => minimatch(p, pat))` | `minimatchList(paths, pat, opts, out_indices)` |

## API Reference

### `MinimatchOptions`

```cp
class MinimatchOptions {
    dot: bool,         // match leading dotfiles (default: false)
    nocase: bool,      // case-insensitive ASCII matching (default: false)
    match_base: bool,  // match basename when pattern has no '/' (default: false)
    noglobstar: bool,  // disable '**' recursive matching (default: false)
    nonegate: bool,    // treat leading '!' as literal (default: false)
    nocomment: bool,   // treat leading '#' as literal (default: false)
    flip_negate: bool, // return true if negated pattern would match without '!' (default: false)
    fn defaults(): MinimatchOptions;
}
```

### Functions

| Signature | Description |
| :--- | :--- |
| `minimatch(path: string, pattern: string): bool` | Match a single path against a glob pattern with default options. |
| `minimatchWithOptions(path: string, pattern: string, opts: MinimatchOptions): bool` | Match a single path with custom `MinimatchOptions`. |
| `isCommentPattern(pattern: string, opts: MinimatchOptions): bool` | Return `true` if `pattern` starts with `#` and `!opts.nocomment`. |
| `isNegatedPattern(pattern: string, opts: MinimatchOptions): bool` | Return `true` if `pattern` starts with `!` and `!opts.nonegate`. |
| `minimatchList(paths: &[]string, pattern: string, opts: MinimatchOptions, out_indices: []usize): usize` | Collect indices of matching paths into `out_indices`; returns match count. |
| `minimatchFilterAll(paths: &[]string, patterns: &[]string, opts: MinimatchOptions, out_indices: []usize): usize` | Multi-pattern batch filter (delegates to `micromatch::mmFilter`). |
| `minimatchRejectAll(paths: &[]string, patterns: &[]string, opts: MinimatchOptions, out_indices: []usize): usize` | Multi-pattern inverse filter (delegates to `micromatch::mmNot`). |

## Usage Examples

### Basic & Option-Driven Glob Matching

```cp
import { minimatch, minimatchWithOptions, MinimatchOptions } from "minimatch";

fn demoBasic() {
    const ok1 = minimatch("src/net/http.cp", "**/*.cp"); // true
    const ok2 = minimatch("src/net/http.cp", "!*.js");   // true

    let opts = MinimatchOptions.defaults();
    opts.nocase = true;
    opts.dot = true;
    const ok3 = minimatchWithOptions(".Env", "*.env", opts); // true
    * = ok1; * = ok2; * = ok3;
}
```

### Batch Filtering & Negated / Comment Patterns

```cp
import {
    MinimatchOptions,
    minimatchList,
    minimatchFilterAll,
    isCommentPattern,
} from "minimatch";

fn demoBatch() {
    const opts = MinimatchOptions.defaults();
    const is_comment = isCommentPattern("# ignore rule", opts); // true

    const paths = ["src/a.cp", "src/b.txt", "test/a.cp"];
    let out: [8]usize = undefined;
    const n = minimatchList(paths[0..], "**/*.cp", opts, out[0..]);
    // n == 2, out[0] == 0, out[1] == 2

    const pats = ["**/*.cp", "!test/**"];
    const m = minimatchFilterAll(paths[0..], pats[0..], opts, out[0..]);
    // m == 1, out[0] == 0 ("src/a.cp")
    * = is_comment; * = n; * = m;
}
```

## Performance & Safety Highlights

- **Zero Heap Allocation**: All matching and batch filtering operate on borrowed `&[]string` slices and write matched indices into caller-owned `out_indices: []usize`.
- **Linear-Time Glob Engine**: Reuses `picomatch` / `micromatch` deterministic matchers — no regex compilation and no catastrophic backtracking.
- **Borrow-Safe Indexing**: Output index slices are strictly bounds-checked; no temporary string allocations or ownership transfers required.

## License

MIT
