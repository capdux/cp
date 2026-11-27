# property-information — AI Agent Quick Ref

Zero-allocation HTML/SVG attribute and DOM property name mapping table (`v6.5.0`, `MIT`)

## Import

```cp
import {
    PropInfo,
    pinfMake,
    pinfNotFound,
    pinfMatchPair,
    findHtmlProp,
    isBooleanAttr,
    isSpaceSeparatedAttr,
    isCommaSeparatedAttr,
} from "property-information";
```

## Core Types & Constants

```cp
class PropInfo { property: string, attribute: string, is_boolean: bool, is_space_separated: bool, /* +3 fields */ }
```

## API Signatures

```cp
fn pinfMake(property: string, attribute: string, is_boolean: bool, is_space_separated: bool, is_comma_separated: bool, is_numeric: bool): PropInfo;
fn pinfNotFound(name: string): PropInfo;
fn pinfMatchPair(name: string, prop: string, attr: string): bool;
fn findHtmlProp(name: string): PropInfo;
fn isBooleanAttr(name: string): bool;
fn isSpaceSeparatedAttr(name: string): bool;
fn isCommaSeparatedAttr(name: string): bool;
fn attrToDomProp(attr: string): string;
fn domPropToAttr(prop: string): string;
```

## Minimal Usage

```cp
import { isBooleanAttr } from "property-information";

pub fn main() {
    let result = isBooleanAttr("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"property-information"`; install via `cap add property-information`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
