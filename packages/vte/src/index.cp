// @cap/vte — Zero-allocation ANSI/VT100 terminal escape sequence parser (Paul Williams model)
// Inspired by crates.io:vte (https://github.com/alacritty/vte)

pub enum VteActionKind {
    Print,
    Execute,
    CsiDispatch,
    OscDispatch,
    EscDispatch,
}

pub enum VteState {
    Ground,
    Escape,
    CsiEntry,
    CsiParam,
    OscString,
    CsiIgnore,
    OscEsc,
}

pub struct VteEvent {
    kind: VteActionKind,
    ch: u8,
    final_byte: u8,
    param_count: usize,
    params: [8]u16,
    intermediate: u8,
}

fn vtepZeroParams(): [8]u16 {
    const z: u16 = 0;
    const arr: [8]u16 = [z, z, z, z, z, z, z, z];
    return arr;
}

fn vtepZeroOsc(): [64]u8 {
    let arr: [64]u8 = undefined;
    let i: usize = 0;
    while (i < 64) {
        arr[i] = 0;
        i += 1;
    }
    return arr;
}

fn vtepIsExecute(b: u8): bool {
    if (b <= 0x17) { return true; }
    if (b == 0x19) { return true; }
    if (b >= 0x1C && b <= 0x1F) { return true; }
    return false;
}

fn vtepAddDigit(cur: u16, digit: u8): u16 {
    const d: u32 = digit as u32;
    const next: u32 = (cur as u32) * 10 + d;
    if (next > 65535) {
        return 65535;
    }
    return next as u16;
}

fn vtepParseOscCmd(buf: []const u8, len: usize): u16 {
    let acc: u16 = 0;
    let i: usize = 0;
    while (i < len) {
        const b = buf[i];
        if (b >= 48 && b <= 57) {
            acc = vtepAddDigit(acc, b - 48);
            i += 1;
        } else {
            break;
        }
    }
    return acc;
}

fn vtepHasOscCmd(buf: []const u8, len: usize): bool {
    if (len == 0) { return false; }
    const b = buf[0];
    return b >= 48 && b <= 57;
}

fn vtepMakeEvent(
    kind: VteActionKind,
    ch: u8,
    final_byte: u8,
    param_count: usize,
    params: [8]u16,
    intermediate: u8,
): VteEvent {
    return VteEvent{
        kind: kind,
        ch: ch,
        final_byte: final_byte,
        param_count: param_count,
        params: params,
        intermediate: intermediate,
    };
}

pub struct VteParser {
    state: VteState,
    params: [8]u16,
    param_count: usize,
    cur_param: u16,
    has_param: bool,
    intermediate: u8,
    osc_buf: [64]u8,
    osc_len: usize,

    static fn new(): VteParser {
        return VteParser{
            state: VteState.Ground,
            params: vtepZeroParams(),
            param_count: 0,
            cur_param: 0,
            has_param: false,
            intermediate: 0,
            osc_buf: vtepZeroOsc(),
            osc_len: 0,
        };
    }

    fn reset(*_) {
        _.state = VteState.Ground;
        _.params = vtepZeroParams();
        _.param_count = 0;
        _.cur_param = 0;
        _.has_param = false;
        _.intermediate = 0;
        _.osc_len = 0;
    }

    fn oscLen(): usize {
        return _.osc_len;
    }

    fn oscByte(idx: usize): u8 {
        if (idx >= _.osc_len) { return 0; }
        return _.osc_buf[idx];
    }

    fn clearCsi(*_) {
        _.params = vtepZeroParams();
        _.param_count = 0;
        _.cur_param = 0;
        _.has_param = false;
        _.intermediate = 0;
    }

    fn pushParam(*_) {
        if (_.param_count < 8) {
            _.params[_.param_count] = _.cur_param;
            _.param_count += 1;
        }
        _.cur_param = 0;
    }

    fn advance(*_, input: []const u8, out_events: []VteEvent): usize {
        let emitted: usize = 0;
        let i: usize = 0;
        while (i < input.len) {
            const b = input[i];
            i += 1;

            // Anywhere transitions: CAN (0x18) and SUB (0x1A) abort sequence and execute
            if (b == 0x18 || b == 0x1A) {
                _.state = VteState.Ground;
                if (emitted < out_events.len) {
                    out_events[emitted] = vtepMakeEvent(
                        VteActionKind.Execute,
                        b,
                        0,
                        0,
                        vtepZeroParams(),
                        0,
                    );
                    emitted += 1;
                }
                continue;
            }

            // ESC (0x1B): in OscString moves to OscEsc (for ST); elsewhere starts Escape
            if (b == 0x1B) {
                if (_.state == VteState.OscString) {
                    _.state = VteState.OscEsc;
                } else {
                    _.clearCsi();
                    _.state = VteState.Escape;
                }
                continue;
            }

            if (_.state == VteState.Ground) {
                if (vtepIsExecute(b)) {
                    if (emitted < out_events.len) {
                        out_events[emitted] = vtepMakeEvent(
                            VteActionKind.Execute,
                            b,
                            0,
                            0,
                            vtepZeroParams(),
                            0,
                        );
                        emitted += 1;
                    }
                } else if ((b >= 0x20 && b <= 0x7E) || b >= 0x80) {
                    if (emitted < out_events.len) {
                        out_events[emitted] = vtepMakeEvent(
                            VteActionKind.Print,
                            b,
                            0,
                            0,
                            vtepZeroParams(),
                            0,
                        );
                        emitted += 1;
                    }
                }
            } else if (_.state == VteState.Escape) {
                if (vtepIsExecute(b)) {
                    if (emitted < out_events.len) {
                        out_events[emitted] = vtepMakeEvent(
                            VteActionKind.Execute,
                            b,
                            0,
                            0,
                            vtepZeroParams(),
                            0,
                        );
                        emitted += 1;
                    }
                } else if (b >= 0x20 && b <= 0x2F) {
                    _.intermediate = b;
                } else if (b == 0x5B && _.intermediate == 0) {
                    // '[' -> CSI Entry
                    _.clearCsi();
                    _.state = VteState.CsiEntry;
                } else if (b == 0x5D && _.intermediate == 0) {
                    // ']' -> OSC String
                    _.osc_len = 0;
                    _.state = VteState.OscString;
                } else if (b >= 0x30 && b <= 0x7E) {
                    const inter = _.intermediate;
                    _.state = VteState.Ground;
                    if (emitted < out_events.len) {
                        out_events[emitted] = vtepMakeEvent(
                            VteActionKind.EscDispatch,
                            0,
                            b,
                            0,
                            vtepZeroParams(),
                            inter,
                        );
                        emitted += 1;
                    }
                }
            } else if (_.state == VteState.CsiEntry) {
                if (vtepIsExecute(b)) {
                    if (emitted < out_events.len) {
                        out_events[emitted] = vtepMakeEvent(
                            VteActionKind.Execute,
                            b,
                            0,
                            0,
                            vtepZeroParams(),
                            0,
                        );
                        emitted += 1;
                    }
                } else if (b >= 0x30 && b <= 0x39) {
                    _.cur_param = (b - 0x30) as u16;
                    _.has_param = true;
                    _.state = VteState.CsiParam;
                } else if (b == 0x3B) {
                    _.cur_param = 0;
                    _.pushParam();
                    _.has_param = true;
                    _.state = VteState.CsiParam;
                } else if ((b >= 0x3C && b <= 0x3F) || (b >= 0x20 && b <= 0x2F)) {
                    _.intermediate = b;
                    _.state = VteState.CsiParam;
                } else if (b == 0x3A) {
                    _.state = VteState.CsiIgnore;
                } else if (b >= 0x40 && b <= 0x7E) {
                    const inter = _.intermediate;
                    _.state = VteState.Ground;
                    if (emitted < out_events.len) {
                        out_events[emitted] = vtepMakeEvent(
                            VteActionKind.CsiDispatch,
                            0,
                            b,
                            0,
                            vtepZeroParams(),
                            inter,
                        );
                        emitted += 1;
                    }
                }
            } else if (_.state == VteState.CsiParam) {
                if (vtepIsExecute(b)) {
                    if (emitted < out_events.len) {
                        out_events[emitted] = vtepMakeEvent(
                            VteActionKind.Execute,
                            b,
                            0,
                            0,
                            vtepZeroParams(),
                            0,
                        );
                        emitted += 1;
                    }
                } else if (b >= 0x30 && b <= 0x39) {
                    _.cur_param = vtepAddDigit(_.cur_param, b - 0x30);
                    _.has_param = true;
                } else if (b == 0x3B) {
                    _.pushParam();
                    _.has_param = true;
                } else if (b >= 0x20 && b <= 0x2F) {
                    _.intermediate = b;
                } else if (b == 0x3A || (b >= 0x3C && b <= 0x3F)) {
                    _.state = VteState.CsiIgnore;
                } else if (b >= 0x40 && b <= 0x7E) {
                    if (_.has_param) {
                        _.pushParam();
                    }
                    const pc = _.param_count;
                    const ps = _.params;
                    const inter = _.intermediate;
                    _.state = VteState.Ground;
                    if (emitted < out_events.len) {
                        out_events[emitted] = vtepMakeEvent(
                            VteActionKind.CsiDispatch,
                            0,
                            b,
                            pc,
                            ps,
                            inter,
                        );
                        emitted += 1;
                    }
                }
            } else if (_.state == VteState.CsiIgnore) {
                if (vtepIsExecute(b)) {
                    if (emitted < out_events.len) {
                        out_events[emitted] = vtepMakeEvent(
                            VteActionKind.Execute,
                            b,
                            0,
                            0,
                            vtepZeroParams(),
                            0,
                        );
                        emitted += 1;
                    }
                } else if (b >= 0x40 && b <= 0x7E) {
                    _.state = VteState.Ground;
                }
            } else if (_.state == VteState.OscString) {
                if (b == 0x07 || b == 0x9C) {
                    let osc_params = vtepZeroParams();
                    let osc_pc: usize = 0;
                    let local_osc: [64]u8 = _.osc_buf;
                    if (vtepHasOscCmd(local_osc[0.._.osc_len], _.osc_len)) {
                        osc_params[0] = vtepParseOscCmd(local_osc[0.._.osc_len], _.osc_len);
                        osc_pc = 1;
                    }
                    _.state = VteState.Ground;
                    if (emitted < out_events.len) {
                        out_events[emitted] = vtepMakeEvent(
                            VteActionKind.OscDispatch,
                            0,
                            b,
                            osc_pc,
                            osc_params,
                            0,
                        );
                        emitted += 1;
                    }
                } else if ((b >= 0x20 && b <= 0x7E) || b >= 0x80) {
                    if (_.osc_len < 64) {
                        _.osc_buf[_.osc_len] = b;
                        _.osc_len += 1;
                    }
                }
            } else if (_.state == VteState.OscEsc) {
                if (b == 0x5C) {
                    // ST terminator: ESC \
                    let osc_params = vtepZeroParams();
                    let osc_pc: usize = 0;
                    let local_osc: [64]u8 = _.osc_buf;
                    if (vtepHasOscCmd(local_osc[0.._.osc_len], _.osc_len)) {
                        osc_params[0] = vtepParseOscCmd(local_osc[0.._.osc_len], _.osc_len);
                        osc_pc = 1;
                    }
                    _.state = VteState.Ground;
                    if (emitted < out_events.len) {
                        out_events[emitted] = vtepMakeEvent(
                            VteActionKind.OscDispatch,
                            0,
                            b,
                            osc_pc,
                            osc_params,
                            0,
                        );
                        emitted += 1;
                    }
                } else {
                    // Abort OSC and treat ESC + b as Escape sequence
                    _.clearCsi();
                    if (b == 0x5B) {
                        _.state = VteState.CsiEntry;
                    } else if (b == 0x5D) {
                        _.osc_len = 0;
                        _.state = VteState.OscString;
                    } else if (b >= 0x30 && b <= 0x7E) {
                        _.state = VteState.Ground;
                        if (emitted < out_events.len) {
                            out_events[emitted] = vtepMakeEvent(
                                VteActionKind.EscDispatch,
                                0,
                                b,
                                0,
                                vtepZeroParams(),
                                0,
                            );
                            emitted += 1;
                        }
                    } else {
                        _.state = VteState.Escape;
                    }
                }
            }
        }
        return emitted;
    }
}
