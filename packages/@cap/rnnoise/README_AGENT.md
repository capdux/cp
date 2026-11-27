# @cap/rnnoise@0.1.1 (Agent Quickref)

> Xiph RNNoise recurrent neural network speech denoising and VAD for CP voice AI agents | License: BSD-3-Clause
> Import: `import { ... } from "@cap/rnnoise";` | Full types: `src/index.d.cp`

## Functions
- `export fn rnnoise_get_size(): i32;`
- `export fn rnnoise_get_frame_size(): i32;`
- `export fn rnnoise_create(model: *auto!?): *auto!?;`
- `export fn rnnoise_destroy(st: *auto!?);`
- `export fn rnnoise_process_frame(st: *auto!?, out: [*]f32!, input: [*]&f32!): f32;`
- `export fn queryFrameSize(): usize;`
- `export fn frameSize(): usize;`
