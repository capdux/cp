# property-information@6.5.0 (Agent Quickref)

> Zero-allocation HTML/SVG attribute and DOM property name mapping table | License: MIT
> Import: `import { ... } from "property-information";` | Full types: `src/index.d.cp`

## Functions
- `export fn pinfMake(property: string, attribute: string, is_boolean: bool, is_space_separated: bool, is_comma_separated: bool, is_numeric: bool): PropInfo;`
- `export fn pinfNotFound(name: string): PropInfo;`
- `export fn pinfMatchPair(name: string, prop: string, attr: string): bool;`
- `export fn findHtmlProp(name: string): PropInfo;`
- `export fn isBooleanAttr(name: string): bool;`
- `export fn isSpaceSeparatedAttr(name: string): bool;`
- `export fn isCommaSeparatedAttr(name: string): bool;`
- `export fn attrToDomProp(attr: string): string;`
- `export fn domPropToAttr(prop: string): string;`
