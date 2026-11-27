# convert-source-map@1.0.0 (Agent Quickref)

> Zero-allocation inline sourceMappingURL comment extractor, Base64 decoder/encoder, and comment stripper (clean-room MIT implementation of npm convert-source-map, #96 / 307M wk) | License: MIT
> Import: `import { ... } from "convert-source-map";` | Full types: `src/index.d.cp`

## Types & Constants
- `export enum SourceMapCommentKind`

## Functions
- `export fn findSourceMapComment(src: string): SourceMapComment;`
- `export fn fromSource(src: string, out: []u8): i64;`
- `export fn toComment(json_map: string, multiline: bool, out: []u8): usize;`
- `export fn removeComments(src: string, out: []u8): usize;`
