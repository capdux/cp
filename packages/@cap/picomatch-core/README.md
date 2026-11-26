# @cap/picomatch-core

Zero-allocation, Clean-room MIT implementation of `picomatch` v4 core capabilities for CP, built on top of `@cap/glob-match`.

## Features

- **`scan(pattern: string): ScanResult`**: Splits a glob pattern into `prefix` (`!`, `./`, `!./`), static `base` directory, dynamic `glob` suffix, `is_glob`, and `negated`.
- **`isMatch(path: string, pattern: string, opts: PicomatchOptions): bool`**: Full path matching with `!` negation, `dot` hidden-file control, `match_base`, `nocase`, `bash`, and extglobs.
- **`matchExtglob(text: string, pat: string): bool`**: Supports `@(a|b)`, `!(a|b)`, `?(a|b)`, `+(a|b)`, and `*(a|b)` with zero heap allocations.
- **Reuses `@cap/glob-match`**: Delegates core `*`, `**`, `?`, `[abc]`, and `{a,b}` wildcard matching directly to `@cap/glob-match`.

## Usage

```cp
import { scan, isMatch, matchExtglob, PicomatchOptions } from "@cap/picomatch-core";

let res = scan("!./src/**/*.cp");
// res.prefix == "!./", res.base == "src", res.glob == "**/*.cp", res.is_glob == true, res.negated == true

let opts = PicomatchOptions.defaults();
let ok = isMatch("src/main.cp", "src/**/*.@(cp|zig)", opts); // true
```

## License

MIT
