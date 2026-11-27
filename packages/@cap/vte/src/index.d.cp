// Type definitions for CP

export enum VteActionKind {
    Print,
    Execute,
    CsiDispatch,
    OscDispatch,
    EscDispatch
}

export enum VteState {
    Ground,
    Escape,
    CsiEntry,
    CsiParam,
    OscString,
    CsiIgnore,
    OscEsc
}

export class VteEvent {
    kind: VteActionKind,
    ch: u8,
    final_byte: u8,
    param_count: usize,
    params: [8]u16,
    intermediate: u8,
}

export fn vtepZeroParams(): [8]u16;
export fn vtepZeroOsc(): [64]u8;
export fn vtepIsExecute(b: u8): bool;
export fn vtepAddDigit(cur: u16, digit: u8): u16;
export fn vtepParseOscCmd(buf: &[]u8, len: usize): u16;
export fn vtepHasOscCmd(buf: &[]u8, len: usize): bool;
export fn vtepMakeEvent(kind: VteActionKind, ch: u8, final_byte: u8, param_count: usize, params: [8]u16, intermediate: u8): VteEvent;
export class VteParser {
    state: VteState,
    params: [8]u16,
    param_count: usize,
    cur_param: u16,
    has_param: bool,
    intermediate: u8,
    osc_buf: [64]u8,
    osc_len: usize,
    fn new(): VteParser;
    fn reset(_: auto);
    fn oscLen(): usize;
    fn oscByte(idx: usize): u8;
    fn clearCsi(_: auto);
    fn pushParam(_: auto);
    fn advance(_: auto, input: &[]u8, out_events: []VteEvent): usize;
}

