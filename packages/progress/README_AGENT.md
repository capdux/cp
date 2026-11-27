# progress@2.0.3 (Agent Quickref)

> Zero-heap terminal progress bar renderer supporting :bar, :percent, :current, :total, :eta, and :rate tokens | License: MIT
> Import: `import { ... } from "progress";` | Full types: `src/index.d.cp`

## Functions
- `export fn newProgressBar(total: u64, width: usize): ProgressBar;`
- `export fn renderProgress(bar: ProgressBar, template: string, out: []u8): usize;`
