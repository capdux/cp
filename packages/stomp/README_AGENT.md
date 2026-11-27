# stomp@1.0.0 (Agent Quickref)

> Zero-allocation STOMP 1.2 messaging protocol frame encoder, decoder, and header escaper in pure CP | License: MIT
> Import: `import { ... } from "stomp";` | Full types: `src/index.d.cp`

## Functions
- `export fn escapeHeader(input: string, out: []u8): usize;`
- `export fn unescapeHeader(input: string, out: []u8): usize;`
- `export fn encodeFrame(command: string, headers: &[]StompHeader, body: string, out: []u8): usize;`
- `export fn parseContentLength(val: string): i64;`
- `export fn decodeFrame(data: string, out_frame: &mut StompFrame): i32;`
- `export fn getHeader(frame: StompFrame, key: string): string;`
