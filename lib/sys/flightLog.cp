// Origin: Pure-CP Stdlib (Zero-Alloc Black-Box Binary Flight-Recorder Event Ring Buffer) | Created: 2026-11-22
// 100% Pure CP zero-heap-allocation binary flight recorder for crash diagnostics & incident forensics:
//   - Compact 8 KiB power-of-two binary ring buffer with automatic oldest-event overwrite & exact `dropped_count`
//   - Monotonic `seq_id`, `timestamp_ns`, `event_kind`, `severity`, and variable-length payload (0..=256B)
//   - Incremental `after_seq_id` and recent-N ordered snapshot replay (`snapshotEvents`)
//   - Crash-freeze / unfreeze latch (`freeze()` / `unfreeze()`) preventing post-fault log overwrite
//   - CRC32-protected binary dump export, structural verification & replay (`exportBinaryDump`, `verifyBinaryDump`, `parseBinaryDump`)

// ============================================================================
// Constants
// ============================================================================

/// Fixed power-of-two capacity of the internal binary ring buffer (8 KiB).
pub const FLIGHT_LOG_RING_BYTES: usize = 8192;

/// Maximum payload length per recorded event in bytes.
pub const FLIGHT_LOG_MAX_PAYLOAD: usize = 256;

/// Encoded per-event binary header size in bytes (`seq:8 + ts:8 + kind:2 + sev:1 + rsv:1 + len:2 = 22`).
pub const FLIGHT_LOG_RECORD_HEADER_SIZE: usize = 22;

/// 4-byte binary dump magic `"CFL1"` in little-endian.
pub const FLIGHT_LOG_DUMP_MAGIC: u32 = 0x314C4643;

/// Fixed binary dump header size in bytes.
pub const FLIGHT_LOG_DUMP_HEADER_SIZE: usize = 32;

/// Trailing IEEE CRC32 checksum size in bytes.
pub const FLIGHT_LOG_DUMP_TRAILER_SIZE: usize = 4;

const RING_MASK: usize = FLIGHT_LOG_RING_BYTES - 1;
const DUMP_VERSION: u16 = 1;

// ============================================================================
// Internal Little-Endian & CRC32 Helpers
// ============================================================================

fn readU16Le(buf: []const u8, off: usize): u16 {
    return (buf[off] as u16) | ((buf[off + 1] as u16) << 8);
}

fn writeU16Le(buf: *[]u8, off: usize, v: u16) {
    buf[off] = (v & 0xff) as u8;
    buf[off + 1] = ((v >> 8) & 0xff) as u8;
}

fn readU32Le(buf: []const u8, off: usize): u32 {
    return (buf[off] as u32) |
        ((buf[off + 1] as u32) << 8) |
        ((buf[off + 2] as u32) << 16) |
        ((buf[off + 3] as u32) << 24);
}

fn writeU32Le(buf: *[]u8, off: usize, v: u32) {
    buf[off] = (v & 0xff) as u8;
    buf[off + 1] = ((v >> 8) & 0xff) as u8;
    buf[off + 2] = ((v >> 16) & 0xff) as u8;
    buf[off + 3] = ((v >> 24) & 0xff) as u8;
}

fn readU64Le(buf: []const u8, off: usize): u64 {
    const lo: u64 = readU32Le(buf, off) as u64;
    const hi: u64 = readU32Le(buf, off + 4) as u64;
    return lo | (hi << 32);
}

fn writeU64Le(buf: *[]u8, off: usize, v: u64) {
    writeU32Le(buf, off, (v & 0xffffffff) as u32);
    writeU32Le(buf, off + 4, ((v >> 32) & 0xffffffff) as u32);
}

fn crc32IeeeSlice(bytes: []const u8): u32 {
    let crc: u32 = 0xFFFFFFFF;
    let i: usize = 0;
    while (i < bytes.len) : (i += 1) {
        crc = crc ^ (bytes[i] as u32);
        let b: usize = 0;
        while (b < 8) : (b += 1) {
            if ((crc & 1) != 0) {
                crc = (crc >> 1) ^ 0xEDB88320;
            } else {
                crc = crc >> 1;
            }
        }
    }
    return crc ^ 0xFFFFFFFF;
}

// ============================================================================
// 1. Enums & Data Models
// ============================================================================

/// Standard severity levels for flight recorder events.
pub enum FlightSeverity: u8 {
    trace = 0,
    debug = 1,
    info = 2,
    warn = 3,
    err = 4,
    fatal = 5,
}

/// Result status of `FlightRecorder.recordEvent(...)`.
pub enum FlightRecordStatus: u8 {
    ok = 0,
    /// Recorder is frozen (e.g. post-crash); writes are rejected to preserve forensic state.
    frozen = 1,
    /// Single event payload exceeds `FLIGHT_LOG_MAX_PAYLOAD` (256 bytes).
    payload_too_large = 2,
}

/// Result status of `verifyBinaryDump(...)`.
pub enum FlightDumpStatus: u8 {
    ok = 0,
    invalid_magic = 1,
    invalid_version = 2,
    truncated = 3,
    checksum_mismatch = 4,
    corrupt_record = 5,
}

/// Decoded flight recorder event returned by `snapshotEvents` and `parseBinaryDump`.
pub struct FlightEvent {
    seq_id: u64 = 0;
    timestamp_ns: u64 = 0;
    event_kind: u16 = 0;
    severity: u8 = 0;
    payload_len: u16 = 0;
    payload: [FLIGHT_LOG_MAX_PAYLOAD]u8 = undefined;

    /// Returns a zero-copy slice of the valid payload bytes.
    fn payloadSlice(): []const u8 {
        const n: usize = _.payload_len as usize;
        const clamped: usize = if (n > FLIGHT_LOG_MAX_PAYLOAD) FLIGHT_LOG_MAX_PAYLOAD else n;
        return _.payload[0..clamped];
    }
}

/// Verification metadata returned by `verifyBinaryDump(dump)`.
pub struct FlightDumpMeta {
    status: FlightDumpStatus = FlightDumpStatus.ok;
    record_count: u32 = 0;
    dropped_count: u64 = 0;
    next_seq_id: u64 = 0;
    frozen: bool = false;
    crc32: u32 = 0;
}

/// Telemetry counters for `FlightRecorder`.
pub struct FlightLogStats {
    live_events: usize = 0;
    used_bytes: usize = 0;
    capacity_bytes: usize = FLIGHT_LOG_RING_BYTES;
    total_recorded: u64 = 0;
    dropped_count: u64 = 0;
    frozen_rejects: u64 = 0;
    oversized_rejects: u64 = 0;
    next_seq_id: u64 = 1;
    frozen: bool = false;
}

// ============================================================================
// 2. FlightRecorder (Zero-Alloc Binary Ring Buffer)
// ============================================================================

/// Zero-heap-allocation black-box binary flight recorder ring buffer.
pub struct FlightRecorder {
    buf: [FLIGHT_LOG_RING_BYTES]u8 = undefined;
    read_pos: usize = 0;
    write_pos: usize = 0;
    used_bytes: usize = 0;
    live_events: usize = 0;
    next_seq_id: u64 = 1;
    total_recorded: u64 = 0;
    dropped_count: u64 = 0;
    frozen: bool = false;
    frozen_rejects: u64 = 0;
    oversized_rejects: u64 = 0;

    /// Creates a fresh empty `FlightRecorder`.
    static fn init(): FlightRecorder {
        return FlightRecorder{};
    }

    fn ringReadBytes(start_pos: usize, dst: *[]u8) {
        if (dst.len == 0) return;
        const start: usize = start_pos & RING_MASK;
        const avail: usize = FLIGHT_LOG_RING_BYTES - start;
        const first: usize = if (dst.len < avail) dst.len else avail;
        @memcpy(dst[0..first], _.buf[start..start + first]);
        if (first < dst.len) {
            @memcpy(dst[first..dst.len], _.buf[0..dst.len - first]);
        }
    }

    fn ringWriteBytes(*_, src: []const u8) {
        if (src.len == 0) return;
        const start: usize = _.write_pos & RING_MASK;
        const avail: usize = FLIGHT_LOG_RING_BYTES - start;
        const first: usize = if (src.len < avail) src.len else avail;
        @memcpy(_.buf[start..start + first], src[0..first]);
        if (first < src.len) {
            @memcpy(_.buf[0..src.len - first], src[first..src.len]);
        }
        _.write_pos = (_.write_pos + src.len) & RING_MASK;
    }

    fn evictOldestOne(*_): bool {
        if (_.live_events == 0 || _.used_bytes < FLIGHT_LOG_RECORD_HEADER_SIZE) return false;
        let hdr_arr: [FLIGHT_LOG_RECORD_HEADER_SIZE]u8 = [0; FLIGHT_LOG_RECORD_HEADER_SIZE];
        let hdr_s: []u8 = hdr_arr[0..];
        _.ringReadBytes(_.read_pos, *hdr_s);
        const plen: usize = readU16Le(hdr_arr[0..], 20) as usize;
        const rec_size: usize = FLIGHT_LOG_RECORD_HEADER_SIZE + plen;
        if (rec_size > _.used_bytes) return false;

        _.read_pos = (_.read_pos + rec_size) & RING_MASK;
        _.used_bytes -= rec_size;
        _.live_events -= 1;
        _.dropped_count += 1;
        return true;
    }

    /// Appends a binary event to the ring buffer, overwriting the oldest events if needed.
    /// Rejects writes when frozen (`frozen`) or when `payload.len > FLIGHT_LOG_MAX_PAYLOAD` (`payload_too_large`).
    fn recordEvent(
        *_,
        timestamp_ns: u64,
        event_kind: u16,
        severity: u8,
        payload: []const u8,
    ): FlightRecordStatus {
        if (_.frozen) {
            _.frozen_rejects += 1;
            return FlightRecordStatus.frozen;
        }
        if (payload.len > FLIGHT_LOG_MAX_PAYLOAD) {
            _.oversized_rejects += 1;
            return FlightRecordStatus.payload_too_large;
        }

        const needed: usize = FLIGHT_LOG_RECORD_HEADER_SIZE + payload.len;
        while (FLIGHT_LOG_RING_BYTES - _.used_bytes < needed) {
            if (!_.evictOldestOne()) break;
        }

        let hdr_arr: [FLIGHT_LOG_RECORD_HEADER_SIZE]u8 = [0; FLIGHT_LOG_RECORD_HEADER_SIZE];
        let hdr_s: []u8 = hdr_arr[0..];
        writeU64Le(*hdr_s, 0, _.next_seq_id);
        writeU64Le(*hdr_s, 8, timestamp_ns);
        writeU16Le(*hdr_s, 16, event_kind);
        hdr_arr[18] = severity;
        hdr_arr[19] = 0;
        writeU16Le(*hdr_s, 20, payload.len as u16);

        _.ringWriteBytes(hdr_arr[0..]);
        if (payload.len > 0) {
            _.ringWriteBytes(payload);
        }

        _.used_bytes += needed;
        _.live_events += 1;
        _.total_recorded += 1;
        _.next_seq_id += 1;
        return FlightRecordStatus.ok;
    }

    /// Freezes the recorder to preserve forensic state after a crash or critical fault.
    fn freeze(*_) {
        _.frozen = true;
    }

    /// Resumes accepting new events into the recorder.
    fn unfreeze(*_) {
        _.frozen = false;
    }

    /// Returns `true` iff the recorder is currently frozen.
    fn isFrozen(): bool {
        return _.frozen;
    }

    /// Clears all buffered events (preserving monotonic `next_seq_id`).
    /// Returns `false` if the recorder is currently frozen.
    fn clear(*_): bool {
        if (_.frozen) return false;
        _.read_pos = 0;
        _.write_pos = 0;
        _.used_bytes = 0;
        _.live_events = 0;
        return true;
    }

    /// Exports an ordered snapshot of events with `seq_id > after_seq_id` into `out_events`.
    ///
    /// - When `max_recent > 0`, returns only the most recent `min(matched, max_recent, out_events.len)` events.
    /// - When `max_recent == 0`, returns up to `out_events.len` earliest matching events in ascending `seq_id` order.
    fn snapshotEvents(
        after_seq_id: u64,
        max_recent: usize,
        out_events: *[]FlightEvent,
    ): usize {
        if (_.live_events == 0 || out_events.len == 0) return 0;

        // First pass: count matching events so we can skip older ones when `max_recent > 0`.
        let matched_total: usize = 0;
        let scan_pos: usize = _.read_pos;
        let idx: usize = 0;
        while (idx < _.live_events) : (idx += 1) {
            let hdr_arr: [FLIGHT_LOG_RECORD_HEADER_SIZE]u8 = [0; FLIGHT_LOG_RECORD_HEADER_SIZE];
            let hdr_s: []u8 = hdr_arr[0..];
            _.ringReadBytes(scan_pos, *hdr_s);
            const seq: u64 = readU64Le(hdr_arr[0..], 0);
            const plen: usize = readU16Le(hdr_arr[0..], 20) as usize;
            if (seq > after_seq_id) {
                matched_total += 1;
            }
            scan_pos = (scan_pos + FLIGHT_LOG_RECORD_HEADER_SIZE + plen) & RING_MASK;
        }
        if (matched_total == 0) return 0;

        let to_skip: usize = 0;
        let limit: usize = out_events.len;
        if (max_recent > 0) {
            const want: usize = if (max_recent < out_events.len) max_recent else out_events.len;
            if (matched_total > want) {
                to_skip = matched_total - want;
            }
            limit = want;
        }

        // Second pass: decode selected events in ascending `seq_id` order.
        let written: usize = 0;
        let seen_matched: usize = 0;
        scan_pos = _.read_pos;
        idx = 0;
        while (idx < _.live_events && written < limit) : (idx += 1) {
            let hdr_arr: [FLIGHT_LOG_RECORD_HEADER_SIZE]u8 = [0; FLIGHT_LOG_RECORD_HEADER_SIZE];
            let hdr_s: []u8 = hdr_arr[0..];
            _.ringReadBytes(scan_pos, *hdr_s);
            const seq: u64 = readU64Le(hdr_arr[0..], 0);
            const ts: u64 = readU64Le(hdr_arr[0..], 8);
            const kind: u16 = readU16Le(hdr_arr[0..], 16);
            const sev: u8 = hdr_arr[18];
            const plen: usize = readU16Le(hdr_arr[0..], 20) as usize;

            if (seq > after_seq_id) {
                if (seen_matched >= to_skip) {
                    let ev = FlightEvent{
                        seq_id: seq,
                        timestamp_ns: ts,
                        event_kind: kind,
                        severity: sev,
                        payload_len: plen as u16,
                        payload: undefined,
                    };
                    if (plen > 0 && plen <= FLIGHT_LOG_MAX_PAYLOAD) {
                        let p_s: []u8 = ev.payload[0..plen];
                        _.ringReadBytes((scan_pos + FLIGHT_LOG_RECORD_HEADER_SIZE) & RING_MASK, *p_s);
                    }
                    out_events[written] = ev;
                    written += 1;
                }
                seen_matched += 1;
            }
            scan_pos = (scan_pos + FLIGHT_LOG_RECORD_HEADER_SIZE + plen) & RING_MASK;
        }

        return written;
    }

    /// Serializes the entire live flight recorder state into a self-contained binary dump
    /// protected by a trailing IEEE CRC32 checksum. Returns total bytes written, or `null`
    /// if `out_buf` is smaller than `32 + used_bytes + 4`.
    fn exportBinaryDump(out_buf: *[]u8): usize? {
        const total_len: usize = FLIGHT_LOG_DUMP_HEADER_SIZE + _.used_bytes + FLIGHT_LOG_DUMP_TRAILER_SIZE;
        if (out_buf.len < total_len) return null;

        writeU32Le(out_buf, 0, FLIGHT_LOG_DUMP_MAGIC);
        writeU16Le(out_buf, 4, DUMP_VERSION);
        writeU16Le(out_buf, 6, if (_.frozen) 1 else 0);
        writeU32Le(out_buf, 8, _.live_events as u32);
        writeU32Le(out_buf, 12, _.used_bytes as u32);
        writeU64Le(out_buf, 16, _.dropped_count);
        writeU64Le(out_buf, 24, _.next_seq_id);

        if (_.used_bytes > 0) {
            let body_s: []u8 = out_buf[FLIGHT_LOG_DUMP_HEADER_SIZE .. FLIGHT_LOG_DUMP_HEADER_SIZE + _.used_bytes];
            _.ringReadBytes(_.read_pos, *body_s);
        }

        const crc_off: usize = FLIGHT_LOG_DUMP_HEADER_SIZE + _.used_bytes;
        const crc: u32 = crc32IeeeSlice(out_buf[0..crc_off]);
        writeU32Le(out_buf, crc_off, crc);
        return total_len;
    }

    /// Returns telemetry statistics for this flight recorder.
    fn stats(): FlightLogStats {
        return FlightLogStats{
            live_events: _.live_events,
            used_bytes: _.used_bytes,
            capacity_bytes: FLIGHT_LOG_RING_BYTES,
            total_recorded: _.total_recorded,
            dropped_count: _.dropped_count,
            frozen_rejects: _.frozen_rejects,
            oversized_rejects: _.oversized_rejects,
            next_seq_id: _.next_seq_id,
            frozen: _.frozen,
        };
    }
}

// ============================================================================
// 3. Binary Dump Verification & Offline Replay
// ============================================================================

/// Verifies a binary flight-recorder dump: checks size, magic `"CFL1"`, version `1`,
/// trailing IEEE CRC32 checksum, and structural consistency of all embedded event records.
pub fn verifyBinaryDump(dump: []const u8): FlightDumpMeta {
    const min_len: usize = FLIGHT_LOG_DUMP_HEADER_SIZE + FLIGHT_LOG_DUMP_TRAILER_SIZE;
    if (dump.len < min_len) {
        return FlightDumpMeta{ status: FlightDumpStatus.truncated };
    }
    if (readU32Le(dump, 0) != FLIGHT_LOG_DUMP_MAGIC) {
        return FlightDumpMeta{ status: FlightDumpStatus.invalid_magic };
    }
    if (readU16Le(dump, 4) != DUMP_VERSION) {
        return FlightDumpMeta{ status: FlightDumpStatus.invalid_version };
    }

    const flags: u16 = readU16Le(dump, 6);
    const rec_count: u32 = readU32Le(dump, 8);
    const body_bytes: usize = readU32Le(dump, 12) as usize;
    const dropped: u64 = readU64Le(dump, 16);
    const next_seq: u64 = readU64Le(dump, 24);

    if (dump.len != FLIGHT_LOG_DUMP_HEADER_SIZE + body_bytes + FLIGHT_LOG_DUMP_TRAILER_SIZE) {
        return FlightDumpMeta{ status: FlightDumpStatus.truncated };
    }

    const crc_off: usize = FLIGHT_LOG_DUMP_HEADER_SIZE + body_bytes;
    const stored_crc: u32 = readU32Le(dump, crc_off);
    const calc_crc: u32 = crc32IeeeSlice(dump[0..crc_off]);
    if (stored_crc != calc_crc) {
        return FlightDumpMeta{ status: FlightDumpStatus.checksum_mismatch };
    }

    // Walk every embedded record to validate framing and strictly increasing sequence IDs.
    let off: usize = FLIGHT_LOG_DUMP_HEADER_SIZE;
    const body_end: usize = crc_off;
    let prev_seq: u64 = 0;
    let r: u32 = 0;
    while (r < rec_count) : (r += 1) {
        if (off + FLIGHT_LOG_RECORD_HEADER_SIZE > body_end) {
            return FlightDumpMeta{ status: FlightDumpStatus.corrupt_record };
        }
        const seq: u64 = readU64Le(dump, off);
        const plen: usize = readU16Le(dump, off + 20) as usize;
        if (seq == 0 || seq <= prev_seq || plen > FLIGHT_LOG_MAX_PAYLOAD) {
            return FlightDumpMeta{ status: FlightDumpStatus.corrupt_record };
        }
        if (off + FLIGHT_LOG_RECORD_HEADER_SIZE + plen > body_end) {
            return FlightDumpMeta{ status: FlightDumpStatus.corrupt_record };
        }
        prev_seq = seq;
        off += FLIGHT_LOG_RECORD_HEADER_SIZE + plen;
    }

    if (off != body_end) {
        return FlightDumpMeta{ status: FlightDumpStatus.corrupt_record };
    }

    return FlightDumpMeta{
        status: FlightDumpStatus.ok,
        record_count: rec_count,
        dropped_count: dropped,
        next_seq_id: next_seq,
        frozen: (flags & 1) != 0,
        crc32: stored_crc,
    };
}

/// Verifies `dump` and decodes up to `out_events.len` embedded events in ascending `seq_id` order.
/// Returns `null` if `verifyBinaryDump(dump)` fails.
pub fn parseBinaryDump(dump: []const u8, out_events: *[]FlightEvent): usize? {
    const meta = verifyBinaryDump(dump);
    if (meta.status != FlightDumpStatus.ok) return null;

    let written: usize = 0;
    let off: usize = FLIGHT_LOG_DUMP_HEADER_SIZE;
    let r: u32 = 0;
    while (r < meta.record_count && written < out_events.len) : (r += 1) {
        const seq: u64 = readU64Le(dump, off);
        const ts: u64 = readU64Le(dump, off + 8);
        const kind: u16 = readU16Le(dump, off + 16);
        const sev: u8 = dump[off + 18];
        const plen: usize = readU16Le(dump, off + 20) as usize;

        let ev = FlightEvent{
            seq_id: seq,
            timestamp_ns: ts,
            event_kind: kind,
            severity: sev,
            payload_len: plen as u16,
            payload: undefined,
        };
        if (plen > 0) {
            @memcpy(ev.payload[0..plen], dump[off + FLIGHT_LOG_RECORD_HEADER_SIZE .. off + FLIGHT_LOG_RECORD_HEADER_SIZE + plen]);
        }
        out_events[written] = ev;
        written += 1;
        off += FLIGHT_LOG_RECORD_HEADER_SIZE + plen;
    }
    return written;
}
