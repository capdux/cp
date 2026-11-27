# @cap/miniaudio@0.11.21 (Agent Quickref)

> Cross-platform audio playback, capture, decoding (WAV/FLAC/MP3) and PCM resampling (miniaudio 0.11.21) for CP | License: MIT-0
> Import: `import { ... } from "@cap/miniaudio";` | Full types: `src/index.d.cp`

## Functions
- `export fn cp_ma_version_string(): [*:0]&u8!?;`
- `export fn cp_ma_decoder_from_memory(data: [*]&u8!, size: usize, out_channels: *u32!?, out_sample_rate: *u32!?, out_frames: *u64!?): [*]f32!?;`
- `export fn cp_ma_free(ptr: *auto!?);`
- `export fn cp_ma_resampler_create(in_rate: u32, out_rate: u32, channels: u32): *auto!?;`
- `export fn cp_ma_resampler_process(resampler: *auto!?, frames_in: [*]&f32!, frame_count_in: *u64!?, frames_out: [*]f32!, frame_count_out: *u64!?): i32;`
- `export fn cp_ma_resampler_free(resampler: *auto!?);`
- `export fn versionString(): string;`
