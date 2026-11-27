# @cap/ogg@1.3.5 (Agent Quickref)

> Xiph libogg 1.3.5 bitstream container framing and page muxing engine for CP | License: BSD-3-Clause
> Import: `import { ... } from "@cap/ogg";` | Full types: `src/index.d.cp`

## Functions
- `export fn cp_ogg_stream_init(serialno: i32): *auto!?;`
- `export fn cp_ogg_stream_packetin(os: *auto!?, data: [*]&u8!, len: isize, bos: i32, eos: i32, granulepos: i64, packetno: i64): i32;`
- `export fn cp_ogg_stream_pageout(os: *auto!?, out_buf: [*]u8!, out_cap: usize, out_len: *usize!): i32;`
- `export fn cp_ogg_stream_flush(os: *auto!?, out_buf: [*]u8!, out_cap: usize, out_len: *usize!): i32;`
- `export fn cp_ogg_stream_clear(os: *auto!?);`
