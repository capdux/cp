# @cap/flac@1.4.3 (Agent Quickref)

> Xiph libFLAC 1.4.3 lossless audio encoder and decoder engine for CP | License: BSD-3-Clause
> Import: `import { ... } from "@cap/flac";` | Full types: `src/index.d.cp`

## Functions
- `export fn cp_flac_version(): [*:0]&u8!?;`
- `export fn cp_flac_vendor(): [*:0]&u8!?;`
- `export fn cp_flac_encode_pcm16_test(pcm: [*]&i16!, samples: u32, sample_rate: u32, out_bytes_written: *u64!): i32;`
- `export fn version(): string;`
- `export fn vendor(): string;`
- `export fn encodePcm16Test(pcm: &[]i16, sample_rate: u32): !u64;`
