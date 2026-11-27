# debugid@0.8.0 (Agent Quickref)

> Zero-heap Sentry/Breakpad/PDB DebugId (128-bit UUID + appendix age) parser and formatter | License: Apache-2.0
> Import: `import { ... } from "debugid";` | Full types: `src/index.d.cp`

## Functions
- `export fn parseDebugId(s: string): DebugId;`
- `export fn formatBreakpad(id: DebugId, out: []u8): usize;`
- `export fn formatCanonical(id: DebugId, out: []u8): usize;`
