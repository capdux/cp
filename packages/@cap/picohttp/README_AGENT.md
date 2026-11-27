# @cap/picohttp@1.0.0 (Agent Quickref)

> CP package @cap/picohttp | License: MIT
> Import: `import { ... } from "@cap/picohttp";` | Full types: `src/index.d.cp`

## Functions
- `export fn phr_parse_request(buf: [*]&u8!, len: usize, method: *[*]&u8!?!, method_len: *usize!, path: *[*]&u8!?!, path_len: *usize!, minor_version: *i32!, headers: [*]phr_header!, num_headers: *usize!, last_len: usize): i32;`
- `export fn phr_parse_response(buf: [*]&u8!, len: usize, minor_version: *i32!, status: *i32!, msg: *[*]&u8!?!, msg_len: *usize!, headers: [*]phr_header!, num_headers: *usize!, last_len: usize): i32;`
- `export fn phr_decode_chunked(decoder: *phr_chunked_decoder!, buf: [*]u8!, bufsz: *usize!): isize;`
- `export fn phr_decode_chunked_is_in_data(decoder: *phr_chunked_decoder!): i32;`
