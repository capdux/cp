# shebang-command@1.0.0 (Agent Quickref)

> Zero-allocation shebang executable command extractor reusing shebang-regex for CP | License: MIT
> Import: `import { ... } from "shebang-command";` | Full types: `src/index.d.cp`
> Deps: `shebang-regex`

## Functions
- `export fn skipSpaces(s: string, pos: usize, end: usize): usize;`
- `export fn scanTokenEnd(s: string, pos: usize, end: usize): usize;`
- `export fn trimRight(s: string, start: usize, end: usize): usize;`
- `export fn basenameStart(s: string, start: usize, end: usize): usize;`
- `export fn locateParts(input: string): Parts;`
- `export fn shebangCommand(input: string, out_cmd: []u8): usize;`
- `export fn parseShebangCmd(input: string, out_bin: []u8, out_arg: []u8): ShebangCmd;`
