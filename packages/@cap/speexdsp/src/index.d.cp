// Type definitions for CP

export fn speex_echo_state_init(frame_size: i32, filter_length: i32): *auto!?;
export fn speex_echo_state_destroy(st: *auto!?);
export fn speex_echo_cancellation(st: *auto!?, rec: [*]&i16!, play: [*]&i16!, out: [*]i16!);
export fn speex_echo_state_reset(st: *auto!?);
export fn speex_echo_ctl(st: *auto!?, request: i32, ptr: *auto!?): i32;
export fn speex_preprocess_state_init(frame_size: i32, sampling_rate: i32): *auto!?;
export fn speex_preprocess_state_destroy(st: *auto!?);
export fn speex_preprocess_run(st: *auto!?, x: [*]i16!): i32;
export fn speex_preprocess_ctl(st: *auto!?, request: i32, ptr: *auto!?): i32;
export error SpeexDspError { InvalidArg, AllocFail, InvalidState, BufferTooSmall, CtlFailed }

export class SpeexEchoState {
    raw: *auto!?,
    frame_size: i32,
    filter_length: i32,
    fn create(frame_size: i32, filter_length: i32): !SpeexEchoState;
    fn setSamplingRate(sample_rate: i32): !void;
    fn cancel(rec: &[]i16, play: &[]i16, out: []i16): !void;
    fn reset(): !void;
    fn rawPtr(): *auto!?;
    fn __cp_deinit(_: &mut SpeexEchoState);
}

export class SpeexPreprocessState {
    raw: *auto!?,
    frame_size: i32,
    sample_rate: i32,
    fn create(frame_size: i32, sample_rate: i32): !SpeexPreprocessState;
    fn setDenoise(enabled: bool): !void;
    fn setAgc(enabled: bool): !void;
    fn setVad(enabled: bool): !void;
    fn setEchoState(echo: &mut SpeexEchoState): !void;
    fn run(pcm: []i16): !i32;
    fn __cp_deinit(_: &mut SpeexPreprocessState);
}

