# url-template@0.1.0 (Agent Quickref)

> Zero-allocation RFC 6570 URI Template (Levels 1-4) expander and validator for CP. | License: MIT
> Import: `import { ... } from "url-template";` | Full types: `src/index.d.cp`

## Functions
- `export fn isValid(tmpl: string): bool;`
- `export fn countVars(tmpl: string): usize;`
- `export fn extractVarName(tmpl: string, index: usize, out: []u8): usize;`
- `export fn pctEncode(input: string, allow_reserved: bool, out: []u8): usize;`
- `export fn expand(tmpl: string, vars: &[]TemplateVar, out: []u8): usize;`
