# @cap/dr_libs@0.13.17 (Agent Quickref)

> Single-header zero-dependency WAV, MP3, and FLAC audio decoders/encoders (dr_libs) for CP | License: MIT-0
> Import: `import { ... } from "@cap/dr_libs";` | Full types: `src/index.d.cp`

## Functions
- `export fn cp_drwav_encode_pcm16_mem(pcm: [*]&i16!, frame_count: u64, channels: u32, sample_rate: u32, out_size: *usize!): [*]u8!?;`
- `export fn cp_drwav_decode_pcm16_mem(data: [*]&u8!, data_size: usize, out_channels: *u32!, out_sample_rate: *u32!, out_frames: *u64!): [*]i16!?;`
- `export fn cp_drflac_decode_pcm16_mem(data: [*]&u8!, data_size: usize, out_channels: *u32!, out_sample_rate: *u32!, out_frames: *u64!): [*]i16!?;`
- `export fn cp_dr_free(ptr: *auto!?);`
