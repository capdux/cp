# cosmiconfig@1.0.0 (Agent Quickref)

> Zero-heap pure CP configuration file search planner, candidate path builder, and loader detector (clean-room MIT implementation of npm cosmiconfig v9) | License: MIT
> Import: `import { ... } from "cosmiconfig";` | Full types: `src/index.d.cp`

## Functions
- `export fn cosmiEndsWithIgnoreCase(s: string, suffix: string): bool;`
- `export fn cosmiBasename(filepath: string): string;`
- `export fn cosmiTrimTrailingSlash(dir: string): string;`
- `export fn detectLoader(filepath: string): u8;`
- `export fn formatSearchPlace(module_name: string, place_idx: usize, out: []u8): usize;`
- `export fn buildCandidatePath(dir: string, place: string, out: []u8): usize;`
