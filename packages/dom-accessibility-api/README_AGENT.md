# dom-accessibility-api@0.6.3 (Agent Quickref)

> Zero-heap W3C Accessible Name and Description computation (AccName) in pure CP | License: MIT
> Import: `import { ... } from "dom-accessibility-api";` | Full types: `src/index.d.cp`

## Functions
- `export fn defaultComputeOptions(): ComputeNameOptions;`
- `export fn defaultA11yNodeEx(tag: string): A11yNodeEx;`
- `export fn a11yUsesAlt(tag: string): bool;`
- `export fn a11yIsInputButtonType(input_type: string): bool;`
- `export fn a11yIsFormControl(tag: string): bool;`
- `export fn isPresentationRole(role: string): bool;`
- `export fn isHiddenElement(aria_hidden: bool, is_hidden: bool, include_hidden: bool): bool;`
- `export fn allowsNameFromContent(tag: string, role: string): bool;`
- `export fn collapseWhitespace(input: string, out: []u8): usize;`
- `export fn computeAccessibleName(node: A11yNodeInput, out: []u8): usize;`
- `export fn computeAccessibleNameWithOptions(node: A11yNodeEx, opts: ComputeNameOptions, out: []u8): NameResult;`
- `export fn computeAccessibleDescription(aria_desc: string, title: string, used_title_as_name: bool, out: []u8): usize;`
- `export fn computeAccessibleDescriptionForNode(node: A11yNodeEx, opts: ComputeNameOptions, used_title_as_name: bool, used_placeholder_as_name: bool, out: []u8): usize;`
