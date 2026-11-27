# @cap/opus@1.5.2 (Agent Quickref)

> Xiph libopus 1.5.2 low-latency audio codec for voice AI agents and WebRTC in CP | License: BSD-3-Clause
> Import: `import { ... } from "@cap/opus";` | Full types: `src/index.d.cp`

## Functions
- `export fn opus_get_version_string(): [*:0]&u8!?;`
- `export fn opus_encoder_create(fs: i32, channels: i32, application: i32, err_out: *i32!?): *auto!?;`
- `export fn opus_encoder_destroy(st: *auto!?);`
- `export fn opus_encode(st: *auto!?, pcm: [*]&i16!, frame_size: i32, data: [*]u8!, max_data_bytes: i32): i32;`
- `export fn opus_decoder_create(fs: i32, channels: i32, err_out: *i32!?): *auto!?;`
- `export fn opus_decoder_destroy(st: *auto!?);`
- `export fn opus_decode(st: *auto!?, data: [*]&u8!, len: i32, pcm: [*]i16!, frame_size: i32, decode_fec: i32): i32;`
- `export fn mapOpusError(code: i32): OpusError;`
- `export fn versionString(): string;`
