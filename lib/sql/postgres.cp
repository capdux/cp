// lib/sql/postgres.cp
// PostgreSQL v3 Wire Authentication, DataRow Iterator & CommandComplete Tag Parser
// Absorbed from Bun src/sql/postgres/protocol/ & src/sql/postgres/CommandTag.cp

pub enum PostgresAuthKind: u8 {
    Ok = 0,
    KerberosV5 = 2,
    CleartextPassword = 3,
    MD5Password = 5,
    SCMCredential = 6,
    GSS = 7,
    GSSContinue = 8,
    SSPI = 9,
    SASL = 10,
    SASLContinue = 11,
    SASLFinal = 12,
    Unknown = 255,

    pub static fn fromCode(code: u32): PostgresAuthKind {
        if (code == 0) { return PostgresAuthKind.Ok; }
        if (code == 2) { return PostgresAuthKind.KerberosV5; }
        if (code == 3) { return PostgresAuthKind.CleartextPassword; }
        if (code == 5) { return PostgresAuthKind.MD5Password; }
        if (code == 6) { return PostgresAuthKind.SCMCredential; }
        if (code == 7) { return PostgresAuthKind.GSS; }
        if (code == 8) { return PostgresAuthKind.GSSContinue; }
        if (code == 9) { return PostgresAuthKind.SSPI; }
        if (code == 10) { return PostgresAuthKind.SASL; }
        if (code == 11) { return PostgresAuthKind.SASLContinue; }
        if (code == 12) { return PostgresAuthKind.SASLFinal; }
        return PostgresAuthKind.Unknown;
    }
}

pub struct PostgresAuthChallenge {
    pub kind: PostgresAuthKind = PostgresAuthKind.Unknown;
    pub md5_salt: [4]u8 = [0, 0, 0, 0];
    pub sasl_mechanism: []const u8 = "";
    pub sasl_data: []const u8 = "";

    /// Parses the payload of a PostgreSQL `'R'` Authentication backend message (after the 5-byte tag+length header).
    pub static fn parse(payload: []const u8): PostgresAuthChallenge? {
        if (payload.len < 4) {
            return null;
        }
        const code = ((payload[0] as u32) << 24) |
                     ((payload[1] as u32) << 16) |
                     ((payload[2] as u32) << 8) |
                     (payload[3] as u32);
        const kind = PostgresAuthKind.fromCode(code);
        let challenge = PostgresAuthChallenge {
            kind: kind,
        };

        if (kind == PostgresAuthKind.MD5Password) {
            if (payload.len < 8) {
                return null;
            }
            challenge.md5_salt[0] = payload[4];
            challenge.md5_salt[1] = payload[5];
            challenge.md5_salt[2] = payload[6];
            challenge.md5_salt[3] = payload[7];
        } else if (kind == PostgresAuthKind.SASL) {
            let end: usize = 4;
            while (end < payload.len && payload[end] != 0) {
                end += 1;
            }
            challenge.sasl_mechanism = payload[4..end];
        } else if (kind == PostgresAuthKind.SASLContinue || kind == PostgresAuthKind.SASLFinal) {
            if (payload.len > 4) {
                challenge.sasl_data = payload[4..payload.len];
            }
        }

        return challenge;
    }
}

pub struct PostgresColumnSlice {
    pub is_null: bool = false;
    pub data: []const u8 = "";
}

pub struct PostgresDataRow {
    pub payload: []const u8 = "";
    pub column_count: u16 = 0;
    pub cursor: usize = 0;
    pub current_col: u16 = 0;

    /// Initializes a zero-copy DataRow column iterator from a `'D'` message payload (starting at the 2-byte column count).
    pub static fn init(payload: []const u8): PostgresDataRow? {
        if (payload.len < 2) {
            return null;
        }
        const cols = ((payload[0] as u16) << 8) | (payload[1] as u16);
        return PostgresDataRow {
            payload: payload,
            column_count: cols,
            cursor: 2,
            current_col: 0,
        };
    }

    pub fn nextColumn(*_): PostgresColumnSlice? {
        if (_.current_col >= _.column_count || _.cursor + 4 > _.payload.len) {
            return null;
        }
        const b0 = _.payload[_.cursor] as u32;
        const b1 = _.payload[_.cursor + 1] as u32;
        const b2 = _.payload[_.cursor + 2] as u32;
        const b3 = _.payload[_.cursor + 3] as u32;
        const raw_len = (b0 << 24) | (b1 << 16) | (b2 << 8) | b3;
        _.cursor += 4;
        _.current_col += 1;

        // 0xFFFFFFFF (-1 in two's complement i32) indicates a SQL NULL column value
        if (raw_len == 0xFFFFFFFF) {
            return PostgresColumnSlice {
                is_null: true,
                data: "",
            };
        }

        const col_len = raw_len as usize;
        if (_.cursor + col_len > _.payload.len) {
            return null;
        }
        const slice = _.payload[_.cursor .. _.cursor + col_len];
        _.cursor += col_len;
        return PostgresColumnSlice {
            is_null: false,
            data: slice,
        };
    }
}

pub struct PostgresCommandComplete {
    pub tag: []const u8 = "";
    pub rows_affected: u64 = 0;

    /// Parses a PostgreSQL `'C'` CommandComplete tag (e.g. `"INSERT 0 5"`, `"UPDATE 12"`, `"SELECT 1"`).
    pub static fn parse(raw_tag: []const u8): PostgresCommandComplete {
        let end = raw_tag.len;
        while (end > 0 && raw_tag[end - 1] == 0) {
            end -= 1;
        }
        const clean = raw_tag[0..end];

        // Find the last space to extract trailing row count if present
        let last_space: usize = clean.len;
        let i: usize = clean.len;
        while (i > 0) {
            i -= 1;
            if (clean[i] == ' ') {
                last_space = i;
                break;
            }
        }

        if (last_space < clean.len && last_space + 1 < clean.len) {
            const num_slice = clean[last_space + 1 .. clean.len];
            let acc: u64 = 0;
            let valid = true;
            let k: usize = 0;
            while (k < num_slice.len) {
                const ch = num_slice[k];
                if (ch < '0' || ch > '9') {
                    valid = false;
                    break;
                }
                acc = acc * 10 + ((ch - '0') as u64);
                k += 1;
            }
            if (valid) {
                return PostgresCommandComplete {
                    tag: clean,
                    rows_affected: acc,
                };
            }
        }

        return PostgresCommandComplete {
            tag: clean,
            rows_affected: 0,
        };
    }
}
