// Source: Absorbed from /data/dev/bun/src/sql/mysql/protocol/StmtPrepareOKPacket.cp | Synced: 2026-11-22T17:32:14Z
// ============================================================================
// lib/mysql/stmtPrepareOk.cp — Zero-Alloc MySQL COM_STMT_PREPARE_OK Codec
// ============================================================================
// Implements MySQL Binary Protocol `COM_STMT_PREPARE_OK` response decoding and
// encoding (10-byte legacy, 12-byte standard with warning_count, and 13-byte
// MySQL 8.0+ with CLIENT_OPTIONAL_RESULTSET_METADATA `metadata_follows`),
// follow-up packet count computation, and zero-alloc `COM_STMT_PREPARE` /
// `COM_STMT_CLOSE` / `COM_STMT_RESET` command frame encoders.

pub const STMT_PREPARE_OK_STATUS: u8 = 0x00;
pub const STMT_PREPARE_OK_MIN_LEN: usize = 10;
pub const STMT_PREPARE_OK_STD_LEN: usize = 12;
pub const STMT_PREPARE_OK_META_LEN: usize = 13;

pub const COM_STMT_PREPARE_CMD: u8 = 0x16;
pub const COM_STMT_CLOSE_CMD: u8 = 0x19;
pub const COM_STMT_RESET_CMD: u8 = 0x1A;

pub const STMT_METADATA_NONE: u8 = 0;
pub const STMT_METADATA_FOLLOWS: u8 = 1;
pub const STMT_MAX_SINGLE_PACKET_PAYLOAD: usize = 0xFFFFFF;

pub err StmtPrepareOkError {
    InvalidStatus,
    Truncated,
    InvalidReservedByte,
    InvalidMetadataFlag,
    BufferTooSmall,
    EmptySql,
    PayloadTooLarge,
}

fn readLeU16(buf: []const u8, off: usize): u16 {
    const b0: u16 = @as(u16, buf[off]);
    const b1: u16 = @as(u16, buf[off + 1]);
    return b0 | (b1 << 8);
}

fn readLeU24(buf: []const u8, off: usize): u32 {
    const b0: u32 = @as(u32, buf[off]);
    const b1: u32 = @as(u32, buf[off + 1]);
    const b2: u32 = @as(u32, buf[off + 2]);
    return b0 | (b1 << 8) | (b2 << 16);
}

fn readLeU32(buf: []const u8, off: usize): u32 {
    const b0: u32 = @as(u32, buf[off]);
    const b1: u32 = @as(u32, buf[off + 1]);
    const b2: u32 = @as(u32, buf[off + 2]);
    const b3: u32 = @as(u32, buf[off + 3]);
    return b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
}

fn writeLeU16(out: []u8, off: usize, v: u16) {
    out[off] = @as(u8, @intCast(v & 0xFF));
    out[off + 1] = @as(u8, @intCast((v >> 8) & 0xFF));
}

fn writeLeU24(out: []u8, off: usize, v: u32) {
    out[off] = @as(u8, @intCast(v & 0xFF));
    out[off + 1] = @as(u8, @intCast((v >> 8) & 0xFF));
    out[off + 2] = @as(u8, @intCast((v >> 16) & 0xFF));
}

fn writeLeU32(out: []u8, off: usize, v: u32) {
    out[off] = @as(u8, @intCast(v & 0xFF));
    out[off + 1] = @as(u8, @intCast((v >> 8) & 0xFF));
    out[off + 2] = @as(u8, @intCast((v >> 16) & 0xFF));
    out[off + 3] = @as(u8, @intCast((v >> 24) & 0xFF));
}

/// MySQL `COM_STMT_PREPARE_OK` response packet.
pub struct StmtPrepareOKPacket {
    status: u8 = STMT_PREPARE_OK_STATUS,
    statement_id: u32 = 0,
    num_columns: u16 = 0,
    num_params: u16 = 0,
    warning_count: u16 = 0,
    packet_length: u32 = STMT_PREPARE_OK_STD_LEN,
    has_metadata_flag: bool = false,
    metadata_follows: bool = true,
    sequence_id: u8 = 1,

    static fn init(statement_id: u32, num_columns: u16, num_params: u16, warning_count: u16): StmtPrepareOKPacket {
        return StmtPrepareOKPacket{
            status: STMT_PREPARE_OK_STATUS,
            statement_id: statement_id,
            num_columns: num_columns,
            num_params: num_params,
            warning_count: warning_count,
            packet_length: STMT_PREPARE_OK_STD_LEN,
            has_metadata_flag: false,
            metadata_follows: true,
            sequence_id: 1,
        };
    }

    static fn initWithMetadata(
        statement_id: u32,
        num_columns: u16,
        num_params: u16,
        warning_count: u16,
        metadata_follows: bool,
    ): StmtPrepareOKPacket {
        return StmtPrepareOKPacket{
            status: STMT_PREPARE_OK_STATUS,
            statement_id: statement_id,
            num_columns: num_columns,
            num_params: num_params,
            warning_count: warning_count,
            packet_length: STMT_PREPARE_OK_META_LEN,
            has_metadata_flag: true,
            metadata_follows: metadata_follows,
            sequence_id: 1,
        };
    }

    /// Decodes a raw `COM_STMT_PREPARE_OK` payload (without the 4-byte MySQL frame header).
    /// Matches Bun's `StmtPrepareOKPacket.decodeInternal` behavior:
    /// - validates `status == 0x00`
    /// - reads LE `statement_id`, `num_columns`, `num_params` and skips the 1-byte reserved filler
    /// - reads `warning_count` when `payload.len >= 12`
    /// - reads MySQL 8.0 `metadata_follows` flag when `payload.len >= 13`
    static fn decodePayload(payload: []const u8, sequence_id: u8): StmtPrepareOkError!StmtPrepareOKPacket {
        if (payload.len < STMT_PREPARE_OK_MIN_LEN) return StmtPrepareOkError.Truncated;
        const status = payload[0];
        if (status != STMT_PREPARE_OK_STATUS) return StmtPrepareOkError.InvalidStatus;

        const stmt_id = readLeU32(payload, 1);
        const cols = readLeU16(payload, 5);
        const params = readLeU16(payload, 7);

        let warnings: u16 = 0;
        if (payload.len >= STMT_PREPARE_OK_STD_LEN) {
            warnings = readLeU16(payload, 10);
        }

        let has_meta = false;
        let meta_follows = true;
        if (payload.len >= STMT_PREPARE_OK_META_LEN) {
            const flag = payload[12];
            if (flag != STMT_METADATA_NONE && flag != STMT_METADATA_FOLLOWS) {
                return StmtPrepareOkError.InvalidMetadataFlag;
            }
            has_meta = true;
            meta_follows = flag == STMT_METADATA_FOLLOWS;
        }

        return StmtPrepareOKPacket{
            status: status,
            statement_id: stmt_id,
            num_columns: cols,
            num_params: params,
            warning_count: warnings,
            packet_length: @as(u32, @intCast(payload.len)),
            has_metadata_flag: has_meta,
            metadata_follows: meta_follows,
            sequence_id: sequence_id,
        };
    }

    /// Strict payload decoder that additionally rejects non-zero reserved filler byte at offset 9.
    static fn decodePayloadStrict(payload: []const u8, sequence_id: u8): StmtPrepareOkError!StmtPrepareOKPacket {
        if (payload.len < STMT_PREPARE_OK_MIN_LEN) return StmtPrepareOkError.Truncated;
        if (payload[9] != 0) return StmtPrepareOkError.InvalidReservedByte;
        return StmtPrepareOKPacket.decodePayload(payload, sequence_id);
    }

    /// Decodes a complete MySQL wire frame (4-byte header + `COM_STMT_PREPARE_OK` payload).
    static fn decodeFrame(frame: []const u8): StmtPrepareOkError!StmtPrepareOKPacket {
        if (frame.len < 4) return StmtPrepareOkError.Truncated;
        const payload_len: usize = @as(usize, readLeU24(frame, 0));
        const seq_id = frame[3];
        if (frame.len < 4 + payload_len) return StmtPrepareOkError.Truncated;
        return StmtPrepareOKPacket.decodePayload(frame[4..(4 + payload_len)], seq_id);
    }

    fn encodedPayloadLen(): usize {
        if (_.has_metadata_flag) return STMT_PREPARE_OK_META_LEN;
        if (_.packet_length == STMT_PREPARE_OK_MIN_LEN) return STMT_PREPARE_OK_MIN_LEN;
        return STMT_PREPARE_OK_STD_LEN;
    }

    /// Encodes the packet payload (without the 4-byte MySQL header) into `out`.
    fn encodePayload(out: []u8): StmtPrepareOkError!usize {
        const need = _.encodedPayloadLen();
        if (out.len < need) return StmtPrepareOkError.BufferTooSmall;
        out[0] = _.status;
        writeLeU32(out, 1, _.statement_id);
        writeLeU16(out, 5, _.num_columns);
        writeLeU16(out, 7, _.num_params);
        out[9] = 0;
        if (need >= STMT_PREPARE_OK_STD_LEN) {
            writeLeU16(out, 10, _.warning_count);
        }
        if (need >= STMT_PREPARE_OK_META_LEN) {
            out[12] = if (_.metadata_follows) STMT_METADATA_FOLLOWS else STMT_METADATA_NONE;
        }
        return need;
    }

    /// Encodes a complete MySQL wire frame (4-byte header + payload) into `out`.
    fn encodeFrame(out: []u8): StmtPrepareOkError!usize {
        const plen = _.encodedPayloadLen();
        const total = 4 + plen;
        if (out.len < total) return StmtPrepareOkError.BufferTooSmall;
        writeLeU24(out, 0, @as(u32, @intCast(plen)));
        out[3] = _.sequence_id;
        * = _.encodePayload(out[4..total])?;
        return total;
    }

    fn hasWarnings(): bool {
        return _.warning_count > 0;
    }

    /// True if the server will send parameter ColumnDefinition41 packets.
    fn expectsParamDefs(): bool {
        return _.num_params > 0;
    }

    /// True if the server will send result column ColumnDefinition41 packets.
    fn expectsColumnDefs(): bool {
        return _.num_columns > 0 && _.metadata_follows;
    }

    /// Computes total follow-up wire packets the server sends immediately after
    /// `COM_STMT_PREPARE_OK` (parameter definitions + optional EOF + column
    /// definitions + optional EOF when `CLIENT_DEPRECATE_EOF` is not set).
    fn expectedFollowupPackets(deprecate_eof: bool): u32 {
        let total: u32 = 0;
        if (_.expectsParamDefs()) {
            total += @as(u32, _.num_params);
            if (!deprecate_eof) total += 1;
        }
        if (_.expectsColumnDefs()) {
            total += @as(u32, _.num_columns);
            if (!deprecate_eof) total += 1;
        }
        return total;
    }

    fn eql(other: StmtPrepareOKPacket): bool {
        return _.status == other.status &&
            _.statement_id == other.statement_id &&
            _.num_columns == other.num_columns &&
            _.num_params == other.num_params &&
            _.warning_count == other.warning_count &&
            _.has_metadata_flag == other.has_metadata_flag &&
            _.metadata_follows == other.metadata_follows &&
            _.sequence_id == other.sequence_id;
    }
}

/// Zero-alloc command frame encoders for MySQL Prepared Statement lifecycle commands.
pub struct StmtCommandCodec {
    /// Encodes a `COM_STMT_PREPARE` (0x16) frame: `[4B header][0x16][sql...]`.
    static fn encodePrepareFrame(sql: []const u8, sequence_id: u8, out: []u8): StmtPrepareOkError!usize {
        if (sql.len == 0) return StmtPrepareOkError.EmptySql;
        const payload_len = 1 + sql.len;
        if (payload_len > STMT_MAX_SINGLE_PACKET_PAYLOAD) return StmtPrepareOkError.PayloadTooLarge;
        const total = 4 + payload_len;
        if (out.len < total) return StmtPrepareOkError.BufferTooSmall;
        writeLeU24(out, 0, @as(u32, @intCast(payload_len)));
        out[3] = sequence_id;
        out[4] = COM_STMT_PREPARE_CMD;
        let i: usize = 0;
        while (i < sql.len) {
            out[5 + i] = sql[i];
            i += 1;
        }
        return total;
    }

    /// Encodes a `COM_STMT_CLOSE` (0x19) frame: `[4B header][0x19][4B LE statement_id]`.
    static fn encodeCloseFrame(statement_id: u32, sequence_id: u8, out: []u8): StmtPrepareOkError!usize {
        if (out.len < 9) return StmtPrepareOkError.BufferTooSmall;
        writeLeU24(out, 0, 5);
        out[3] = sequence_id;
        out[4] = COM_STMT_CLOSE_CMD;
        writeLeU32(out, 5, statement_id);
        return 9;
    }

    /// Encodes a `COM_STMT_RESET` (0x1A) frame: `[4B header][0x1A][4B LE statement_id]`.
    static fn encodeResetFrame(statement_id: u32, sequence_id: u8, out: []u8): StmtPrepareOkError!usize {
        if (out.len < 9) return StmtPrepareOkError.BufferTooSmall;
        writeLeU24(out, 0, 5);
        out[3] = sequence_id;
        out[4] = COM_STMT_RESET_CMD;
        writeLeU32(out, 5, statement_id);
        return 9;
    }
}
