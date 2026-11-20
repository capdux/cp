// Source: cp/lib/midi/midi.cp (Pure CP Standard MIDI File Format 0/1 Parser, VLQ, Running Status & Builder)
// Pure CP in-memory Standard MIDI File (SMF) Format 0/1/2 decoder, encoder, VLQ codec, and Running Status parser.

const std = @import("std");

/// Error set returned by MIDI parsing, encoding, and VLQ operations.
pub err MidiError {
    InvalidHeader,
    InvalidTrackHeader,
    UnexpectedEof,
    InvalidVlq,
    InvalidStatusByte,
    UnsupportedFormat,
    OutOfMemory,
}

/// Standard MIDI File (SMF) track layout format.
pub enum MidiFormat: u16 {
    SingleTrack = 0,
    MultiTrackSync = 1,
    MultiTrackAsync = 2,
}

/// Payload for note_off and note_on channel voice events.
pub struct NoteEventData {
    channel: u4 = 0,
    note: u7 = 0,
    velocity: u7 = 0,
}

/// Payload for polyphonic key pressure (aftertouch) events.
pub struct PolyPressureData {
    channel: u4 = 0,
    note: u7 = 0,
    pressure: u7 = 0,
}

/// Payload for control change events.
pub struct ControlChangeData {
    channel: u4 = 0,
    controller: u7 = 0,
    value: u7 = 0,
}

/// Payload for program change events.
pub struct ProgramChangeData {
    channel: u4 = 0,
    program: u7 = 0,
}

/// Payload for channel pressure (monophonic aftertouch) events.
pub struct ChannelPressureData {
    channel: u4 = 0,
    pressure: u7 = 0,
}

/// Payload for 14-bit pitch bend events (center = 8192).
pub struct PitchBendData {
    channel: u4 = 0,
    value: u14 = 8192,
}

/// Payload for arbitrary MIDI meta events.
pub struct MetaOtherData {
    meta_type: u8 = 0,
    data: []const u8 = "",
}

/// Tagged union of MIDI channel, meta, and SysEx events.
pub enum MidiEventKind {
    note_off: NoteEventData,
    note_on: NoteEventData,
    poly_pressure: PolyPressureData,
    control_change: ControlChangeData,
    program_change: ProgramChangeData,
    channel_pressure: ChannelPressureData,
    pitch_bend: PitchBendData,
    meta_tempo: u32,
    meta_end_of_track: void,
    meta_other: MetaOtherData,
    sysex: []const u8,
}

/// Single timed MIDI event with delta-time in ticks.
pub struct MidiEvent {
    delta_ticks: u32 = 0,
    kind: MidiEventKind,
}

/// A single MTrk track containing an ordered sequence of MIDI events.
pub struct MidiTrack {
    events: []MidiEvent = [],
}

/// Parsed or constructed Standard MIDI File container.
pub struct MidiFile {
    format: MidiFormat = MidiFormat.SingleTrack,
    ticks_per_quarter: u16 = 480,
    tracks: []MidiTrack = [],

    /// Frees all owned tracks, event arrays, and duped SysEx/Meta byte slices.
    pub fn deinit(*_, allocator: std.mem.Allocator) {
        for (track in _.tracks) {
            for (ev in track.events) {
                switch (ev.kind) {
                    .meta_other => |m| {
                        if (m.data.len > 0) {
                            allocator.free(m.data);
                        }
                    },
                    .sysex => |s| {
                        if (s.len > 0) {
                            allocator.free(s);
                        }
                    },
                    else => {},
                }
            }
            if (track.events.len > 0) {
                allocator.free(track.events);
            }
        }
        if (_.tracks.len > 0) {
            allocator.free(_.tracks);
            _.tracks = [];
        }
    }

    /// Returns the maximum cumulative delta_ticks across all tracks.
    pub fn totalTicks(): u64 {
        let max_ticks: u64 = 0;
        for (track in _.tracks) {
            let sum: u64 = 0;
            for (ev in track.events) {
                sum += ev.delta_ticks as u64;
            }
            if (sum > max_ticks) {
                max_ticks = sum;
            }
        }
        return max_ticks;
    }

    /// Converts a tick count into milliseconds given microseconds per quarter note.
    pub fn ticksToMs(ticks: u64, us_per_quarter: u32): f64 {
        if (_.ticks_per_quarter == 0) return 0.0;
        const num = (ticks as f64) * (us_per_quarter as f64);
        const den = (_.ticks_per_quarter as f64) * 1000.0;
        return num / den;
    }
}

fn readU16Be(b: []const u8, off: usize): u16 {
    return ((b[off] as u16) << 8) | (b[off + 1] as u16);
}

fn readU32Be(b: []const u8, off: usize): u32 {
    const b0 = b[off] as u32;
    const b1 = b[off + 1] as u32;
    const b2 = b[off + 2] as u32;
    const b3 = b[off + 3] as u32;
    return (b0 << 24) | (b1 << 16) | (b2 << 8) | b3;
}

fn appendU16Be(list: *std.ArrayList(u8), allocator: std.mem.Allocator, val: u16): MidiError!void {
    list.append(allocator, ((val >> 8) & 0xFF) as u8) catch return MidiError.OutOfMemory;
    list.append(allocator, (val & 0xFF) as u8) catch return MidiError.OutOfMemory;
}

fn appendU32Be(list: *std.ArrayList(u8), allocator: std.mem.Allocator, val: u32): MidiError!void {
    list.append(allocator, ((val >> 24) & 0xFF) as u8) catch return MidiError.OutOfMemory;
    list.append(allocator, ((val >> 16) & 0xFF) as u8) catch return MidiError.OutOfMemory;
    list.append(allocator, ((val >> 8) & 0xFF) as u8) catch return MidiError.OutOfMemory;
    list.append(allocator, (val & 0xFF) as u8) catch return MidiError.OutOfMemory;
}

fn tagEql(b: []const u8, off: usize, c0: u8, c1: u8, c2: u8, c3: u8): bool {
    return b[off] == c0 && b[off + 1] == c1 && b[off + 2] == c2 && b[off + 3] == c3;
}

/// Decodes a 1..4 byte big-endian 7-bit VLQ integer, advancing `pos`.
fn readVlq(buf: []const u8, pos: *usize): MidiError!u32 {
    let val: u32 = 0;
    let count: usize = 0;
    while (count < 4) {
        if (pos.* >= buf.len) return MidiError.UnexpectedEof;
        const b = buf[pos.*];
        pos.* += 1;
        val = (val << 7) | ((b & 0x7F) as u32);
        if ((b & 0x80) == 0) {
            return val;
        }
        count += 1;
    }
    return MidiError.InvalidVlq;
}

/// Encodes a 28-bit unsigned integer into 1..4 byte big-endian 7-bit VLQ.
fn writeVlq(list: *std.ArrayList(u8), allocator: std.mem.Allocator, value: u32): MidiError!void {
    if (value > 0x0FFFFFFF) return MidiError.InvalidVlq;
    let buf: [4]u8 = [0, 0, 0, 0];
    let vlen: usize = 1;
    let v = value;
    buf[0] = (v & 0x7F) as u8;
    v >>= 7;
    while (v > 0) {
        buf[vlen] = ((v & 0x7F) as u8) | 0x80;
        vlen += 1;
        v >>= 7;
    }
    let idx: usize = vlen;
    while (idx > 0) {
        idx -= 1;
        list.append(allocator, buf[idx]) catch return MidiError.OutOfMemory;
    }
}

fn readDataByte(buf: []const u8, pos: *usize): MidiError!u7 {
    if (pos.* >= buf.len) return MidiError.UnexpectedEof;
    const b = buf[pos.*];
    if ((b & 0x80) != 0) return MidiError.InvalidStatusByte;
    pos.* += 1;
    return b as u7;
}

fn freeEventList(allocator: std.mem.Allocator, events: []const MidiEvent) {
    for (ev in events) {
        switch (ev.kind) {
            .meta_other => |m| {
                if (m.data.len > 0) {
                    allocator.free(m.data);
                }
            },
            .sysex => |s| {
                if (s.len > 0) {
                    allocator.free(s);
                }
            },
            else => {},
        }
    }
}

fn dupeSlice(allocator: std.mem.Allocator, src: []const u8): MidiError![]const u8 {
    if (src.len == 0) return "";
    return allocator.dupe(u8, src) catch return MidiError.OutOfMemory;
}

fn parseTrack(allocator: std.mem.Allocator, track_bytes: []const u8): MidiError!MidiTrack {
    let events: std.ArrayList(MidiEvent) = .empty;
    errdefer {
        freeEventList(allocator, events.items);
        events.deinit(allocator);
    }

    let pos: usize = 0;
    let running_status: u8 = 0;

    while (pos < track_bytes.len) {
        const delta = readVlq(track_bytes, &pos)?;
        if (pos >= track_bytes.len) return MidiError.UnexpectedEof;

        const peek = track_bytes[pos];
        let status: u8 = 0;
        if ((peek & 0x80) != 0) {
            status = peek;
            pos += 1;
            if (status >= 0x80 && status <= 0xEF) {
                running_status = status;
            } else {
                // System Common / Meta events clear Running Status per MIDI 1.0 spec
                running_status = 0;
            }
        } else {
            if (running_status == 0) return MidiError.InvalidStatusByte;
            status = running_status;
        }

        if (status >= 0x80 && status <= 0xEF) {
            const msg_type = status & 0xF0;
            const ch: u4 = (status & 0x0F) as u4;
            let kind: MidiEventKind = MidiEventKind.meta_end_of_track;

            if (msg_type == 0x80) {
                const n = readDataByte(track_bytes, &pos)?;
                const v = readDataByte(track_bytes, &pos)?;
                kind = MidiEventKind { note_off: NoteEventData { channel: ch, note: n, velocity: v } };
            } else if (msg_type == 0x90) {
                const n = readDataByte(track_bytes, &pos)?;
                const v = readDataByte(track_bytes, &pos)?;
                kind = MidiEventKind { note_on: NoteEventData { channel: ch, note: n, velocity: v } };
            } else if (msg_type == 0xA0) {
                const n = readDataByte(track_bytes, &pos)?;
                const p = readDataByte(track_bytes, &pos)?;
                kind = MidiEventKind { poly_pressure: PolyPressureData { channel: ch, note: n, pressure: p } };
            } else if (msg_type == 0xB0) {
                const c = readDataByte(track_bytes, &pos)?;
                const v = readDataByte(track_bytes, &pos)?;
                kind = MidiEventKind { control_change: ControlChangeData { channel: ch, controller: c, value: v } };
            } else if (msg_type == 0xC0) {
                const p = readDataByte(track_bytes, &pos)?;
                kind = MidiEventKind { program_change: ProgramChangeData { channel: ch, program: p } };
            } else if (msg_type == 0xD0) {
                const p = readDataByte(track_bytes, &pos)?;
                kind = MidiEventKind { channel_pressure: ChannelPressureData { channel: ch, pressure: p } };
            } else {
                // 0xE0 Pitch Bend (14-bit little-endian 7-bit pair)
                const lsb = readDataByte(track_bytes, &pos)?;
                const msb = readDataByte(track_bytes, &pos)?;
                const bend_val: u14 = ((msb as u14) << 7) | (lsb as u14);
                kind = MidiEventKind { pitch_bend: PitchBendData { channel: ch, value: bend_val } };
            }

            events.append(allocator, MidiEvent { delta_ticks: delta, kind: kind }) catch return MidiError.OutOfMemory;
        } else if (status == 0xF0 || status == 0xF7) {
            const slen = readVlq(track_bytes, &pos)? as usize;
            if (pos + slen > track_bytes.len) return MidiError.UnexpectedEof;
            const raw = track_bytes[pos .. pos + slen];
            pos += slen;
            const duped = dupeSlice(allocator, raw)?;
            errdefer if (duped.len > 0) allocator.free(duped);
            events.append(allocator, MidiEvent {
                delta_ticks: delta,
                kind: MidiEventKind { sysex: duped },
            }) catch return MidiError.OutOfMemory;
        } else if (status == 0xFF) {
            if (pos >= track_bytes.len) return MidiError.UnexpectedEof;
            const meta_type = track_bytes[pos];
            pos += 1;
            const mlen = readVlq(track_bytes, &pos)? as usize;
            if (pos + mlen > track_bytes.len) return MidiError.UnexpectedEof;
            const mdata = track_bytes[pos .. pos + mlen];
            pos += mlen;

            if (meta_type == 0x51 && mlen == 3) {
                const us: u32 = ((mdata[0] as u32) << 16) | ((mdata[1] as u32) << 8) | (mdata[2] as u32);
                events.append(allocator, MidiEvent {
                    delta_ticks: delta,
                    kind: MidiEventKind { meta_tempo: us },
                }) catch return MidiError.OutOfMemory;
            } else if (meta_type == 0x2F && mlen == 0) {
                events.append(allocator, MidiEvent {
                    delta_ticks: delta,
                    kind: MidiEventKind.meta_end_of_track,
                }) catch return MidiError.OutOfMemory;
            } else {
                const duped = dupeSlice(allocator, mdata)?;
                errdefer if (duped.len > 0) allocator.free(duped);
                events.append(allocator, MidiEvent {
                    delta_ticks: delta,
                    kind: MidiEventKind {
                        meta_other: MetaOtherData {
                            meta_type: meta_type,
                            data: duped,
                        },
                    },
                }) catch return MidiError.OutOfMemory;
            }
        } else {
            return MidiError.InvalidStatusByte;
        }
    }

    const owned_events = events.toOwnedSlice(allocator) catch return MidiError.OutOfMemory;
    return MidiTrack { events: owned_events };
}

/// Parses a Standard MIDI File (Format 0/1/2) from raw bytes.
pub fn parse(allocator: std.mem.Allocator, smf_bytes: []const u8): MidiError!MidiFile {
    if (smf_bytes.len < 4) return MidiError.UnexpectedEof;
    if (!tagEql(smf_bytes, 0, 'M', 'T', 'h', 'd')) return MidiError.InvalidHeader;
    if (smf_bytes.len < 14) return MidiError.UnexpectedEof;

    const header_len = readU32Be(smf_bytes, 4);
    if (header_len != 6) return MidiError.InvalidHeader;

    const fmt_raw = readU16Be(smf_bytes, 8);
    let format: MidiFormat = MidiFormat.SingleTrack;
    if (fmt_raw == 0) {
        format = MidiFormat.SingleTrack;
    } else if (fmt_raw == 1) {
        format = MidiFormat.MultiTrackSync;
    } else if (fmt_raw == 2) {
        format = MidiFormat.MultiTrackAsync;
    } else {
        return MidiError.UnsupportedFormat;
    }

    const num_tracks = readU16Be(smf_bytes, 10);
    if (format == MidiFormat.SingleTrack && num_tracks != 1) {
        return MidiError.InvalidHeader;
    }

    const division = readU16Be(smf_bytes, 12);
    if ((division & 0x8000) != 0) {
        return MidiError.UnsupportedFormat;
    }
    if (division == 0) {
        return MidiError.InvalidHeader;
    }

    let tracks: std.ArrayList(MidiTrack) = .empty;
    errdefer {
        for (tr in tracks.items) {
            freeEventList(allocator, tr.events);
            if (tr.events.len > 0) allocator.free(tr.events);
        }
        tracks.deinit(allocator);
    }

    let pos: usize = 14;
    let t_idx: u16 = 0;
    while (t_idx < num_tracks) {
        if (pos + 8 > smf_bytes.len) return MidiError.UnexpectedEof;
        if (!tagEql(smf_bytes, pos, 'M', 'T', 'r', 'k')) return MidiError.InvalidTrackHeader;
        const track_len = readU32Be(smf_bytes, pos + 4) as usize;
        pos += 8;
        if (pos + track_len > smf_bytes.len) return MidiError.UnexpectedEof;

        const tr = parseTrack(allocator, smf_bytes[pos .. pos + track_len])?;
        pos += track_len;
        tracks.append(allocator, tr) catch {
            freeEventList(allocator, tr.events);
            if (tr.events.len > 0) allocator.free(tr.events);
            return MidiError.OutOfMemory;
        };
        t_idx += 1;
    }

    const owned_tracks = tracks.toOwnedSlice(allocator) catch return MidiError.OutOfMemory;
    return MidiFile {
        format: format,
        ticks_per_quarter: division,
        tracks: owned_tracks,
    };
}

/// Encodes a `MidiFile` into Standard MIDI File (SMF) binary bytes.
/// Automatically appends an End-of-Track meta event (`FF 2F 00`) if missing.
pub fn encode(allocator: std.mem.Allocator, midi_file: MidiFile): MidiError![]u8 {
    if (midi_file.ticks_per_quarter == 0 || (midi_file.ticks_per_quarter & 0x8000) != 0) {
        return MidiError.InvalidHeader;
    }
    if (midi_file.format == MidiFormat.SingleTrack && midi_file.tracks.len != 1) {
        return MidiError.InvalidHeader;
    }

    let out: std.ArrayList(u8) = .empty;
    errdefer out.deinit(allocator);

    // MThd header (14 bytes)
    out.appendSlice(allocator, "MThd") catch return MidiError.OutOfMemory;
    appendU32Be(&out, allocator, 6)?;
    appendU16Be(&out, allocator, @intFromEnum(midi_file.format))?;
    appendU16Be(&out, allocator, @intCast(midi_file.tracks.len))?;
    appendU16Be(&out, allocator, midi_file.ticks_per_quarter)?;

    for (track in midi_file.tracks) {
        let tbuf: std.ArrayList(u8) = .empty;
        defer tbuf.deinit(allocator);

        let has_eot = false;
        for (ev in track.events) {
            writeVlq(&tbuf, allocator, ev.delta_ticks)?;
            switch (ev.kind) {
                .note_off => |d| {
                    tbuf.append(allocator, 0x80 | (d.channel as u8)) catch return MidiError.OutOfMemory;
                    tbuf.append(allocator, d.note as u8) catch return MidiError.OutOfMemory;
                    tbuf.append(allocator, d.velocity as u8) catch return MidiError.OutOfMemory;
                },
                .note_on => |d| {
                    tbuf.append(allocator, 0x90 | (d.channel as u8)) catch return MidiError.OutOfMemory;
                    tbuf.append(allocator, d.note as u8) catch return MidiError.OutOfMemory;
                    tbuf.append(allocator, d.velocity as u8) catch return MidiError.OutOfMemory;
                },
                .poly_pressure => |d| {
                    tbuf.append(allocator, 0xA0 | (d.channel as u8)) catch return MidiError.OutOfMemory;
                    tbuf.append(allocator, d.note as u8) catch return MidiError.OutOfMemory;
                    tbuf.append(allocator, d.pressure as u8) catch return MidiError.OutOfMemory;
                },
                .control_change => |d| {
                    tbuf.append(allocator, 0xB0 | (d.channel as u8)) catch return MidiError.OutOfMemory;
                    tbuf.append(allocator, d.controller as u8) catch return MidiError.OutOfMemory;
                    tbuf.append(allocator, d.value as u8) catch return MidiError.OutOfMemory;
                },
                .program_change => |d| {
                    tbuf.append(allocator, 0xC0 | (d.channel as u8)) catch return MidiError.OutOfMemory;
                    tbuf.append(allocator, d.program as u8) catch return MidiError.OutOfMemory;
                },
                .channel_pressure => |d| {
                    tbuf.append(allocator, 0xD0 | (d.channel as u8)) catch return MidiError.OutOfMemory;
                    tbuf.append(allocator, d.pressure as u8) catch return MidiError.OutOfMemory;
                },
                .pitch_bend => |d| {
                    tbuf.append(allocator, 0xE0 | (d.channel as u8)) catch return MidiError.OutOfMemory;
                    tbuf.append(allocator, (d.value & 0x7F) as u8) catch return MidiError.OutOfMemory;
                    tbuf.append(allocator, ((d.value >> 7) & 0x7F) as u8) catch return MidiError.OutOfMemory;
                },
                .meta_tempo => |us| {
                    tbuf.append(allocator, 0xFF) catch return MidiError.OutOfMemory;
                    tbuf.append(allocator, 0x51) catch return MidiError.OutOfMemory;
                    tbuf.append(allocator, 0x03) catch return MidiError.OutOfMemory;
                    tbuf.append(allocator, ((us >> 16) & 0xFF) as u8) catch return MidiError.OutOfMemory;
                    tbuf.append(allocator, ((us >> 8) & 0xFF) as u8) catch return MidiError.OutOfMemory;
                    tbuf.append(allocator, (us & 0xFF) as u8) catch return MidiError.OutOfMemory;
                },
                .meta_end_of_track => {
                    tbuf.append(allocator, 0xFF) catch return MidiError.OutOfMemory;
                    tbuf.append(allocator, 0x2F) catch return MidiError.OutOfMemory;
                    tbuf.append(allocator, 0x00) catch return MidiError.OutOfMemory;
                    has_eot = true;
                },
                .meta_other => |m| {
                    tbuf.append(allocator, 0xFF) catch return MidiError.OutOfMemory;
                    tbuf.append(allocator, m.meta_type) catch return MidiError.OutOfMemory;
                    writeVlq(&tbuf, allocator, @intCast(m.data.len))?;
                    tbuf.appendSlice(allocator, m.data) catch return MidiError.OutOfMemory;
                },
                .sysex => |s| {
                    tbuf.append(allocator, 0xF0) catch return MidiError.OutOfMemory;
                    writeVlq(&tbuf, allocator, @intCast(s.len))?;
                    tbuf.appendSlice(allocator, s) catch return MidiError.OutOfMemory;
                },
            }
        }

        if (!has_eot) {
            tbuf.append(allocator, 0x00) catch return MidiError.OutOfMemory;
            tbuf.append(allocator, 0xFF) catch return MidiError.OutOfMemory;
            tbuf.append(allocator, 0x2F) catch return MidiError.OutOfMemory;
            tbuf.append(allocator, 0x00) catch return MidiError.OutOfMemory;
        }

        out.appendSlice(allocator, "MTrk") catch return MidiError.OutOfMemory;
        appendU32Be(&out, allocator, @intCast(tbuf.items.len))?;
        out.appendSlice(allocator, tbuf.items) catch return MidiError.OutOfMemory;
    }

    return out.toOwnedSlice(allocator) catch return MidiError.OutOfMemory;
}
