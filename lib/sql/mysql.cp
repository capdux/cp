// Source: Custom implementation by Antigravity AI
// lib/sql/mysql.cp
// MySQL Wire Protocol: Length-Encoded Integer, OK_Packet, ERR_Packet & Capabilities
// Absorbed from Bun src/sql/mysql/Capabilities.cp & src/sql/mysql/protocol/

pub struct MySqlLenEnc {
    pub value: u64 = 0;
    pub bytes_used: usize = 0;

    /// Encodes a u64 as a MySQL Length-Encoded Integer into `out`.
    pub static fn encode(out: []u8, val: u64): usize {
        if (val < 251) {
            if (out.len < 1) { return 0; }
            out[0] = (val & 0xFF) as u8;
            return 1;
        }
        if (val < 65536) {
            if (out.len < 3) { return 0; }
            out[0] = 0xFC;
            out[1] = (val & 0xFF) as u8;
            out[2] = ((val >> 8) & 0xFF) as u8;
            return 3;
        }
        if (val < 16777216) {
            if (out.len < 4) { return 0; }
            out[0] = 0xFD;
            out[1] = (val & 0xFF) as u8;
            out[2] = ((val >> 8) & 0xFF) as u8;
            out[3] = ((val >> 16) & 0xFF) as u8;
            return 4;
        }
        if (out.len < 9) { return 0; }
        out[0] = 0xFE;
        out[1] = (val & 0xFF) as u8;
        out[2] = ((val >> 8) & 0xFF) as u8;
        out[3] = ((val >> 16) & 0xFF) as u8;
        out[4] = ((val >> 24) & 0xFF) as u8;
        out[5] = ((val >> 32) & 0xFF) as u8;
        out[6] = ((val >> 40) & 0xFF) as u8;
        out[7] = ((val >> 48) & 0xFF) as u8;
        out[8] = ((val >> 56) & 0xFF) as u8;
        return 9;
    }

    /// Decodes a MySQL Length-Encoded Integer from `buf`.
    pub static fn decode(buf: []const u8): MySqlLenEnc? {
        if (buf.len == 0) {
            return null;
        }
        const first = buf[0];
        if (first < 0xFB) {
            return MySqlLenEnc {
                value: first as u64,
                bytes_used: 1,
            };
        }
        if (first == 0xFC) {
            if (buf.len < 3) { return null; }
            const v = (buf[1] as u64) | ((buf[2] as u64) << 8);
            return MySqlLenEnc {
                value: v,
                bytes_used: 3,
            };
        }
        if (first == 0xFD) {
            if (buf.len < 4) { return null; }
            const v = (buf[1] as u64) | ((buf[2] as u64) << 8) | ((buf[3] as u64) << 16);
            return MySqlLenEnc {
                value: v,
                bytes_used: 4,
            };
        }
        if (first == 0xFE) {
            if (buf.len < 9) { return null; }
            const v = (buf[1] as u64) |
                      ((buf[2] as u64) << 8) |
                      ((buf[3] as u64) << 16) |
                      ((buf[4] as u64) << 24) |
                      ((buf[5] as u64) << 32) |
                      ((buf[6] as u64) << 40) |
                      ((buf[7] as u64) << 48) |
                      ((buf[8] as u64) << 56);
            return MySqlLenEnc {
                value: v,
                bytes_used: 9,
            };
        }
        return null;
    }
}

pub struct MySqlCapabilities {
    pub flags: u32 = 0;

    pub static fn flagLongPassword(): u32 { return 1; }
    pub static fn flagFoundRows(): u32 { return 2; }
    pub static fn flagLongFlag(): u32 { return 4; }
    pub static fn flagConnectWithDb(): u32 { return 8; }
    pub static fn flagProtocol41(): u32 { return 512; }
    pub static fn flagSsl(): u32 { return 2048; }
    pub static fn flagTransactions(): u32 { return 8192; }
    pub static fn flagSecureConnection(): u32 { return 32768; }
    pub static fn flagMultiStatements(): u32 { return 65536; }
    pub static fn flagMultiResults(): u32 { return 131072; }
    pub static fn flagPluginAuth(): u32 { return 524288; }
    pub static fn flagDeprecateEof(): u32 { return 16777216; }

    pub static fn defaultClient(): MySqlCapabilities {
        return MySqlCapabilities {
            flags: 1 | 4 | 8 | 512 | 8192 | 32768 | 131072 | 524288 | 16777216,
        };
    }

    pub fn has(flag: u32): bool {
        return (_.flags & flag) != 0;
    }

    pub fn enable(*_, flag: u32) {
        _.flags = _.flags | flag;
    }

    pub fn disable(*_, flag: u32) {
        _.flags = _.flags & (~flag);
    }
}

pub struct MySqlOkPacket {
    pub affected_rows: u64 = 0;
    pub last_insert_id: u64 = 0;
    pub status_flags: u16 = 0;
    pub warnings: u16 = 0;

    /// Parses a MySQL 4.1+ `OK_Packet` payload (header byte `0x00` or `0xFE`).
    pub static fn parse(payload: []const u8): MySqlOkPacket? {
        if (payload.len < 7 || (payload[0] != 0x00 && payload[0] != 0xFE)) {
            return null;
        }
        let pos: usize = 1;
        const aff_opt = MySqlLenEnc.decode(payload[pos..]);
        if (aff_opt == null) { return null; }
        const aff = aff_opt.?;
        pos += aff.bytes_used;

        const lid_opt = MySqlLenEnc.decode(payload[pos..]);
        if (lid_opt == null) { return null; }
        const lid = lid_opt.?;
        pos += lid.bytes_used;

        let status: u16 = 0;
        let warn: u16 = 0;
        if (pos + 2 <= payload.len) {
            status = (payload[pos] as u16) | ((payload[pos + 1] as u16) << 8);
            pos += 2;
        }
        if (pos + 2 <= payload.len) {
            warn = (payload[pos] as u16) | ((payload[pos + 1] as u16) << 8);
        }

        return MySqlOkPacket {
            affected_rows: aff.value,
            last_insert_id: lid.value,
            status_flags: status,
            warnings: warn,
        };
    }
}

pub struct MySqlErrPacket {
    pub error_code: u16 = 0;
    pub sql_state: []const u8 = "";
    pub message: []const u8 = "";

    /// Parses a MySQL 4.1+ `ERR_Packet` payload (header byte `0xFF`).
    pub static fn parse(payload: []const u8): MySqlErrPacket? {
        if (payload.len < 3 || payload[0] != 0xFF) {
            return null;
        }
        const code = (payload[1] as u16) | ((payload[2] as u16) << 8);
        if (payload.len >= 9 && payload[3] == '#') {
            return MySqlErrPacket {
                error_code: code,
                sql_state: payload[4..9],
                message: payload[9..payload.len],
            };
        }
        return MySqlErrPacket {
            error_code: code,
            sql_state: "HY000",
            message: payload[3..payload.len],
        };
    }
}
