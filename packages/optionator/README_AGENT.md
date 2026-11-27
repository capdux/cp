# optionator@0.9.4 (Agent Quickref)

> Zero-heap CLI option specification and auto-aligned --help documentation generator in pure CP | License: MIT
> Import: `import { ... } from "optionator";` | Full types: `src/index.d.cp`

## Functions
- `export fn optnLeftColLen(spec: OptionSpec): usize;`
- `export fn formatOptionRow(spec: OptionSpec, col_width: usize, out: []u8): usize;`
- `export fn generateHelp(program: string, summary: string, specs: &[]OptionSpec, out: []u8): usize;`
- `export fn findOptionSpec(specs: &[]OptionSpec, flag: string): OptionSpec?;`
