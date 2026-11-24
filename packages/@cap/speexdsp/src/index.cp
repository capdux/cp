// @cap/speexdsp — Xiph SpeexDSP 1.2.1 AEC, noise suppression, AGC, and VAD binding for CP
// License: BSD-3-Clause (Upstream: Xiph.Org Foundation / speexdsp)

pub @c {
    fn speex_echo_state_init(frame_size: i32, filter_length: i32): *!?;
    fn speex_echo_state_destroy(st: *!?);
    fn speex_echo_cancellation(
        st: *!?,
        rec: [*]const i16!,
        play: [*]const i16!,
        out: [*]i16!
    );
    fn speex_echo_state_reset(st: *!?);
    fn speex_echo_ctl(st: *!?, request: i32, ptr: *!?): i32;

    fn speex_preprocess_state_init(frame_size: i32, sampling_rate: i32): *!?;
    fn speex_preprocess_state_destroy(st: *!?);
    fn speex_preprocess_run(st: *!?, x: [*]i16!): i32;
    fn speex_preprocess_ctl(st: *!?, request: i32, ptr: *!?): i32;
}

pub const SPEEX_ECHO_GET_FRAME_SIZE: i32 = 3;
pub const SPEEX_ECHO_SET_SAMPLING_RATE: i32 = 24;
pub const SPEEX_ECHO_GET_SAMPLING_RATE: i32 = 25;

pub const SPEEX_PREPROCESS_SET_DENOISE: i32 = 0;
pub const SPEEX_PREPROCESS_GET_DENOISE: i32 = 1;
pub const SPEEX_PREPROCESS_SET_AGC: i32 = 2;
pub const SPEEX_PREPROCESS_GET_AGC: i32 = 3;
pub const SPEEX_PREPROCESS_SET_VAD: i32 = 4;
pub const SPEEX_PREPROCESS_GET_VAD: i32 = 5;
pub const SPEEX_PREPROCESS_SET_NOISE_SUPPRESS: i32 = 18;
pub const SPEEX_PREPROCESS_SET_ECHO_SUPPRESS: i32 = 20;
pub const SPEEX_PREPROCESS_SET_ECHO_SUPPRESS_ACTIVE: i32 = 22;
pub const SPEEX_PREPROCESS_SET_ECHO_STATE: i32 = 24;

pub err SpeexDspError {
    InvalidArg,
    AllocFail,
    InvalidState,
    BufferTooSmall,
    CtlFailed,
}

/// Owned RAII SpeexDSP acoustic echo canceller state. Automatically destroyed on scope exit.
pub struct SpeexEchoState {
    raw: *!? = null,
    pub frame_size: i32 = 0,
    pub filter_length: i32 = 0,

    pub static fn create(frame_size: i32, filter_length: i32): SpeexEchoState! {
        if (frame_size <= 0 or filter_length <= 0) return SpeexDspError.InvalidArg;
        let st: *!? = null;
        unsafe {
            st = speex_echo_state_init(frame_size, filter_length);
        }
        if (st == null) return SpeexDspError.AllocFail;
        return SpeexEchoState{
            .raw = st,
            .frame_size = frame_size,
            .filter_length = filter_length,
        };
    }

    pub fn setSamplingRate(sample_rate: i32): void! {
        if (_.raw == null) return SpeexDspError.InvalidState;
        if (sample_rate <= 0) return SpeexDspError.InvalidArg;
        let rate: i32 = sample_rate;
        let rc: i32 = 0;
        unsafe {
            rc = speex_echo_ctl(_.raw, SPEEX_ECHO_SET_SAMPLING_RATE, @ptrCast(&rate));
        }
        if (rc != 0) return SpeexDspError.CtlFailed;
    }

    /// Cancels far-end `play` echo from microphone `rec` frame into `out` (all `frame_size` i16 samples).
    pub fn cancel(rec: []const i16, play: []const i16, out: []i16): void! {
        if (_.raw == null) return SpeexDspError.InvalidState;
        const needed: usize = @intCast(_.frame_size);
        if (rec.len < needed or play.len < needed or out.len < needed) {
            return SpeexDspError.BufferTooSmall;
        }
        unsafe {
            speex_echo_cancellation(_.raw, rec.ptr, play.ptr, out.ptr);
        }
    }

    pub fn reset(): void! {
        if (_.raw == null) return SpeexDspError.InvalidState;
        unsafe {
            speex_echo_state_reset(_.raw);
        }
    }

    pub fn rawPtr(): *!? {
        return _.raw;
    }

    ~init() {
        if (p := _.raw) {
            unsafe { speex_echo_state_destroy(p); }
            _.raw = null;
        }
    }
}

/// Owned RAII SpeexDSP preprocessor (denoiser / residual echo suppression / AGC / VAD).
pub struct SpeexPreprocessState {
    raw: *!? = null,
    pub frame_size: i32 = 0,
    pub sample_rate: i32 = 0,

    pub static fn create(frame_size: i32, sample_rate: i32): SpeexPreprocessState! {
        if (frame_size <= 0 or sample_rate <= 0) return SpeexDspError.InvalidArg;
        let st: *!? = null;
        unsafe {
            st = speex_preprocess_state_init(frame_size, sample_rate);
        }
        if (st == null) return SpeexDspError.AllocFail;
        return SpeexPreprocessState{
            .raw = st,
            .frame_size = frame_size,
            .sample_rate = sample_rate,
        };
    }

    pub fn setDenoise(enabled: bool): void! {
        if (_.raw == null) return SpeexDspError.InvalidState;
        let val: i32 = if (enabled) 1 else 0;
        let rc: i32 = 0;
        unsafe {
            rc = speex_preprocess_ctl(_.raw, SPEEX_PREPROCESS_SET_DENOISE, @ptrCast(&val));
        }
        if (rc != 0) return SpeexDspError.CtlFailed;
    }

    pub fn setAgc(enabled: bool): void! {
        if (_.raw == null) return SpeexDspError.InvalidState;
        let val: i32 = if (enabled) 1 else 0;
        let rc: i32 = 0;
        unsafe {
            rc = speex_preprocess_ctl(_.raw, SPEEX_PREPROCESS_SET_AGC, @ptrCast(&val));
        }
        if (rc != 0) return SpeexDspError.CtlFailed;
    }

    pub fn setVad(enabled: bool): void! {
        if (_.raw == null) return SpeexDspError.InvalidState;
        let val: i32 = if (enabled) 1 else 0;
        let rc: i32 = 0;
        unsafe {
            rc = speex_preprocess_ctl(_.raw, SPEEX_PREPROCESS_SET_VAD, @ptrCast(&val));
        }
        if (rc != 0) return SpeexDspError.CtlFailed;
    }

    /// Associates an echo canceller state for residual echo suppression.
    pub fn setEchoState(echo: *SpeexEchoState): void! {
        if (_.raw == null) return SpeexDspError.InvalidState;
        let echo_ptr: *!? = echo.rawPtr();
        let rc: i32 = 0;
        unsafe {
            rc = speex_preprocess_ctl(_.raw, SPEEX_PREPROCESS_SET_ECHO_STATE, echo_ptr);
        }
        if (rc != 0) return SpeexDspError.CtlFailed;
    }

    /// Preprocesses an audio frame in-place (`frame_size` i16 samples).
    /// Returns 1 for speech and 0 for silence/noise when VAD is enabled.
    pub fn run(pcm: []i16): i32! {
        if (_.raw == null) return SpeexDspError.InvalidState;
        const needed: usize = @intCast(_.frame_size);
        if (pcm.len < needed) return SpeexDspError.BufferTooSmall;
        let vad: i32 = 0;
        unsafe {
            vad = speex_preprocess_run(_.raw, pcm.ptr);
        }
        return vad;
    }

    ~init() {
        if (p := _.raw) {
            unsafe { speex_preprocess_state_destroy(p); }
            _.raw = null;
        }
    }
}
