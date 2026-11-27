# dom-accessibility-api@0.6.3 (Agent Quickref)

> Zero-heap W3C Accessible Name and Description computation (AccName) in pure CP | License: MIT
> Import: `import { ... } from "dom-accessibility-api";` | Full types: `src/index.d.cp`

## Functions
- `export fn a11yIsSpace(c: u8): bool;`
- `export fn a11yToLower(c: u8): u8;`
- `export fn a11yEqIgnoreCase(a: string, b: string): bool;`
- `export fn a11yUsesAlt(tag: string): bool;`
- `export fn collapseWhitespace(input: string, out: []u8): usize;`
- `export fn computeAccessibleName(node: A11yNodeInput, out: []u8): usize;`
- `export fn computeAccessibleDescription(aria_desc: string, title: string, used_title_as_name: bool, out: []u8): usize;`
