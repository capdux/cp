// @cap/rnnoise — Xiph RNNoise 0.1.1 neural speech denoising & VAD binding for CP
// License: BSD-3-Clause (Upstream: Xiph.Org Foundation / Mozilla rnnoise)

pub @c {
    fn rnnoise_get_size(): i32;
    fn rnnoise_get_frame_size(): i32;
    fn rnnoise_create(model: *!?): *!?;
    fn rnnoise_destroy(st: *!?);
    fn rnnoise_process_frame(st: *!?, out: [*]f32!, input: [*]const f32!): f32;
}

pub const FRAME_SIZE: usize = 480;

pub err RnnoiseError {
    AllocFail,
    InvalidState,
    BufferTooSmall,
}

fn queryFrameSize(): usize {
    let n: i32 = 0;
    unsafe {
        n = rnnoise_get_frame_size();
    }
    if (n <= 0) return FRAME_SIZE;
    return @intCast(n);
}

/// Returns the number of 48kHz float samples processed per frame (480 samples = 10ms).
pub fn frameSize(): usize {
    return queryFrameSize();
}

/// Owned RAII RNNoise denoiser state. Automatically freed on scope exit.
pub struct DenoiseState {
    raw: *!? = null,

    pub static fn frameSize(): usize {
        return queryFrameSize();
    }

    pub static fn create(): DenoiseState! {
        let st: *!? = null;
        unsafe {
            st = rnnoise_create(null);
        }
        if (st == null) return RnnoiseError.AllocFail;
        return DenoiseState{ .raw = st };
    }

    /// Denoises a single 480-sample 48kHz float frame (PCM range [-32768.0, 32767.0]).
    /// Returns the estimated Voice Activity Detection (VAD) probability in [0.0, 1.0].
    pub fn processFrame(out: []f32, input: []const f32): f32! {
        if (_.raw == null) return RnnoiseError.InvalidState;
        const needed: usize = queryFrameSize();
        if (out.len < needed or input.len < needed) return RnnoiseError.BufferTooSmall;
        let vad: f32 = 0.0;
        unsafe {
            vad = rnnoise_process_frame(_.raw, out.ptr, input.ptr);
        }
        return vad;
    }

    ~init() {
        if (p := _.raw) {
            unsafe { rnnoise_destroy(p); }
            _.raw = null;
        }
    }
}
