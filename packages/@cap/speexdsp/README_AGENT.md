# @cap/speexdsp@1.2.1 (Agent Quickref)

> Xiph SpeexDSP 1.2.1 real-time acoustic echo cancellation (AEC), noise suppression, AGC, and VAD for CP | License: BSD-3-Clause
> Import: `import { ... } from "@cap/speexdsp";` | Full types: `src/index.d.cp`

## Functions
- `export fn speex_echo_state_init(frame_size: i32, filter_length: i32): *auto!?;`
- `export fn speex_echo_state_destroy(st: *auto!?);`
- `export fn speex_echo_cancellation(st: *auto!?, rec: [*]&i16!, play: [*]&i16!, out: [*]i16!);`
- `export fn speex_echo_state_reset(st: *auto!?);`
- `export fn speex_echo_ctl(st: *auto!?, request: i32, ptr: *auto!?): i32;`
- `export fn speex_preprocess_state_init(frame_size: i32, sampling_rate: i32): *auto!?;`
- `export fn speex_preprocess_state_destroy(st: *auto!?);`
- `export fn speex_preprocess_run(st: *auto!?, x: [*]i16!): i32;`
- `export fn speex_preprocess_ctl(st: *auto!?, request: i32, ptr: *auto!?): i32;`
