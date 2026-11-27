# dom-serializer@2.0.0 (Agent Quickref)

> Zero-heap HTML/XML DOM node serializer (covers domelementtype and w3c-xmlserializer) | License: MIT
> Import: `import { ... } from "dom-serializer";` | Full types: `src/index.d.cp`

## Types & Constants
- `export enum DomNodeKind`

## Functions
- `export fn dsrlWriteByte(out: []u8, pos: usize, b: u8): usize;`
- `export fn dsrlWriteAttrValue(out: []u8, pos: usize, val: string): usize;`
- `export fn isVoidElement(tag: string): bool;`
- `export fn serializeText(text: string, out: []u8): usize;`
- `export fn serializeOpenTag(tag: string, attrs: &[]DomAttr, self_close: bool, out: []u8): usize;`
- `export fn serializeElement(tag: string, attrs: &[]DomAttr, inner_html: string, opts: SerializeOptions, out: []u8): usize;`
- `export fn serializeComment(text: string, out: []u8): usize;`
- `export fn serializeCdata(text: string, out: []u8): usize;`
