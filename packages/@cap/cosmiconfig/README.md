# @cap/cosmiconfig

Zero-heap, pure CP configuration file discovery planner, candidate path builder, and loader detector (clean-room MIT implementation inspired by npm `cosmiconfig` v9).

## Features

- **Zero heap allocation**: Upward directory traversal via string slicing (`SearchPlanner`) and caller-owned output buffers.
- **Standard search places**: Generates all 8 default cosmiconfig search places (`package.json`, `.<mod>rc`, `.<mod>rc.json`, `.<mod>rc.yaml`, `.<mod>rc.yml`, `.config/<mod>rc`, `<mod>.config.js`, `<mod>.config.cjs`).
- **Loader detection**: Classifies `LOADER_PKG_JSON`, `LOADER_JSON`, `LOADER_YAML` (including extensionless rc files), `LOADER_JS` (`.js`/`.cjs`/`.mjs`), and `LOADER_UNKNOWN`.

## Usage

```cp
import {
    ExplorerOptions, SearchPlanner, DEFAULT_SEARCH_PLACE_COUNT,
    formatSearchPlace, buildCandidatePath, detectLoader
} from "@cap/cosmiconfig";

let opts = ExplorerOptions.defaults("myapp");
let planner = SearchPlanner.new("/workspace/pkg/sub", "/workspace");
let place_buf: [64]u8 = [0 as u8; 64];
let path_buf: [256]u8 = [0 as u8; 256];
```
