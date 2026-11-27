# @cap/lilconfig

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:lilconfig` ([https://github.com/antonk52/lilconfig](https://github.com/antonk52/lilconfig))

Zero-heap, pure CP lightweight configuration explorer (clean-room MIT implementation inspired by npm `lilconfig` v3).

## Features

- **Zero heap allocation**: Fixed 8-slot custom loader registry and zero-copy upward directory traversal.
- **Composable**: Reuses `@cap/cosmiconfig`'s `SearchPlanner`, `buildCandidatePath`, `detectLoader`, and `CosmiConfigResult`.
- **Lightweight defaults**: 6 built-in search places (`package.json`, `.<mod>rc`, `.<mod>rc.json`, `.<mod>rc.js`, `.<mod>rc.cjs`, `<mod>.config.js`) supporting JSON/JS by default without YAML unless explicitly registered.

## Usage

```cp
import {
    LilconfigOptions, LilExplorer, isSupportedByDefault
} from "@cap/lilconfig";

let opts = LilconfigOptions.defaults("postcss");
let explorer = LilExplorer.new("/repo/packages/ui", opts);
* = explorer.registerLoader(".yaml", 2);
```
