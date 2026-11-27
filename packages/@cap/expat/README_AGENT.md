# @cap/expat@2.6.4 (Agent Quickref)

> Stream-oriented XML 1.0 SAX parser (libexpat 2.6.4) native C binding for CP | License: MIT
> Import: `import { ... } from "@cap/expat";` | Full types: `src/index.d.cp`

## Functions
- `export fn cp_expat_version(): [*:0]&u8!;`
- `export fn cp_expat_count_elements(xml: [*]&u8!?, len: usize, out_elem_count: *usize!): i32;`
- `export fn versionString(): string;`
- `export fn countElements(xml: &[]u8): !usize;`
