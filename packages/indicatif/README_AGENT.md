# indicatif@1.0.0 (Agent Quickref)

> CP package indicatif | License: MIT
> Import: `import { ... } from "indicatif";` | Full types: `src/index.d.cp`
> Deps: `pretty-bytes`, `pretty-ms`

## Functions
- `export fn indWriteByte(out: []u8, pos: usize, b: u8): usize;`
- `export fn indWriteStr(out: []u8, pos: usize, s: &[]u8): usize;`
- `export fn indWriteU64(out: []u8, pos: usize, val: u64): usize;`
- `export fn percent(s: ProgressState): f64;`
- `export fn etaMs(s: ProgressState): u64;`
- `export fn bytesPerSec(s: ProgressState): f64;`
- `export fn renderBar(s: ProgressState, fill_ch: u8, head_ch: u8, empty_ch: u8, out: []u8): usize;`
- `export fn renderLine(s: ProgressState, prefix: string, out: []u8): usize;`
