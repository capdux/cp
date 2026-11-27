// Type definitions for CP

export fn rnnoise_get_size(): i32;
export fn rnnoise_get_frame_size(): i32;
export fn rnnoise_create(model: *auto!?): *auto!?;
export fn rnnoise_destroy(st: *auto!?);
export fn rnnoise_process_frame(st: *auto!?, out: [*]f32!, input: [*]&f32!): f32;
export error RnnoiseError { AllocFail, InvalidState, BufferTooSmall }

export fn queryFrameSize(): usize;
export fn frameSize(): usize;
export class DenoiseState {
    raw: *auto!?,
    fn frameSize(): usize;
    fn create(): !DenoiseState;
    fn processFrame(out: []f32, input: &[]f32): !f32;
    fn __cp_deinit(_: &mut DenoiseState);
}

