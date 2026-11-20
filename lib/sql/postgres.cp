// Source: /data/dev/bun/src/sql/
// lib/sql/postgres.cp
// PostgreSQL v3 Wire Authentication (SCRAM-SHA-256 / MD5), Backend Frame Decoders,
// RowDescription / DataRow / ErrorResponse / ReadyForQuery / ParameterStatus / BackendKeyData
// Absorbed from Bun src/sql/postgres/protocol/ & src/sql/postgres/CommandTag.cp

const std = @import("std");
import { Base64 } from "cap:encoding";
import { PostgresTransactionStatus } from "./protocol";

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
        if (code == 0) return PostgresAuthKind.Ok;
        if (code == 2) return PostgresAuthKind.KerberosV5;
        if (code == 3) return PostgresAuthKind.CleartextPassword;
        if (code == 5) return PostgresAuthKind.MD5Password;
        if (code == 6) return PostgresAuthKind.SCMCredential;
        if (code == 7) return PostgresAuthKind.GSS;
        if (code == 8) return PostgresAuthKind.GSSContinue;
        if (code == 9) return PostgresAuthKind.SSPI;
        if (code == 10) return PostgresAuthKind.SASL;
        if (code == 11) return PostgresAuthKind.SASLContinue;
        if (code == 12) return PostgresAuthKind.SASLFinal;
        return PostgresAuthKind.Unknown;
    }
}

pub enum PostgresBackendTag: u8 {
    Authentication = 'R',
    BackendKeyData = 'K',
    BindComplete = '2',
    CloseComplete = '3',
    CommandComplete = 'C',
    DataRow = 'D',
    EmptyQueryResponse = 'I',
    ErrorResponse = 'E',
    NoData = 'n',
    NoticeResponse = 'N',
    ParameterDescription = 't',
    ParameterStatus = 'S',
    ParseComplete = '1',
    PortalSuspended = 's',
    ReadyForQuery = 'Z',
    RowDescription = 'T',
    Unknown = 0,

    pub static fn fromByte(b: u8): PostgresBackendTag {
        return switch (b) {
            'R' => PostgresBackendTag.Authentication,
            'K' => PostgresBackendTag.BackendKeyData,
            '2' => PostgresBackendTag.BindComplete,
            '3' => PostgresBackendTag.CloseComplete,
            'C' => PostgresBackendTag.CommandComplete,
            'D' => PostgresBackendTag.DataRow,
            'I' => PostgresBackendTag.EmptyQueryResponse,
            'E' => PostgresBackendTag.ErrorResponse,
            'n' => PostgresBackendTag.NoData,
            'N' => PostgresBackendTag.NoticeResponse,
            't' => PostgresBackendTag.ParameterDescription,
            'S' => PostgresBackendTag.ParameterStatus,
            '1' => PostgresBackendTag.ParseComplete,
            's' => PostgresBackendTag.PortalSuspended,
            'Z' => PostgresBackendTag.ReadyForQuery,
            'T' => PostgresBackendTag.RowDescription,
            else => PostgresBackendTag.Unknown,
        };
    }
}

pub struct PostgresBackendFrame {
    pub tag: PostgresBackendTag = PostgresBackendTag.Unknown,
    pub raw_tag: u8 = 0,
    pub payload: []const u8 = "",
    pub total_bytes: usize = 0,

    /// Decodes a complete 5-byte header + payload PostgreSQL backend frame from `buf`.
    pub static fn decode(buf: []const u8): PostgresBackendFrame? {
        if (buf.len < 5) return null;
        const msg_len = ((buf[1] as u32) << 24) |
            ((buf[2] as u32) << 16) |
            ((buf[3] as u32) << 8) |
            (buf[4] as u32);
        if (msg_len < 4) return null;
        const total = 1 + (msg_len as usize);
        if (buf.len < total) return null;
        return PostgresBackendFrame{
            tag: PostgresBackendTag.fromByte(buf[0]),
            raw_tag: buf[0],
            payload: buf[5..total],
            total_bytes: total,
        };
    }
}

pub struct PostgresAuthChallenge {
    pub kind: PostgresAuthKind = PostgresAuthKind.Unknown;
    pub md5_salt: [4]u8 = [0, 0, 0, 0];
    pub sasl_mechanism: []const u8 = "";
    pub sasl_data: []const u8 = "";

    /// Parses the payload of a PostgreSQL `'R'` Authentication backend message (after the 5-byte tag+length header).
    pub static fn parse(payload: []const u8): PostgresAuthChallenge? {
        if (payload.len < 4) return null;
        const code = ((payload[0] as u32) << 24) |
            ((payload[1] as u32) << 16) |
            ((payload[2] as u32) << 8) |
            (payload[3] as u32);
        const kind = PostgresAuthKind.fromCode(code);
        let challenge = PostgresAuthChallenge{
            kind: kind,
        };

        if (kind == PostgresAuthKind.MD5Password) {
            if (payload.len < 8) return null;
            challenge.md5_salt[0] = payload[4];
            challenge.md5_salt[1] = payload[5];
            challenge.md5_salt[2] = payload[6];
            challenge.md5_salt[3] = payload[7];
        } else if (kind == PostgresAuthKind.SASL) {
            let end: usize = 4;
            while (end < payload.len && payload[end] != 0) : (end += 1) {}
            challenge.sasl_mechanism = payload[4..end];
        } else if (kind == PostgresAuthKind.SASLContinue || kind == PostgresAuthKind.SASLFinal) {
            if (payload.len > 4) {
                challenge.sasl_data = payload[4..payload.len];
            }
        }

        return challenge;
    }
}

pub struct PostgresParameterStatus {
    pub name: []const u8 = "",
    pub value: []const u8 = "",

    pub static fn parse(payload: []const u8): PostgresParameterStatus? {
        const nul1 = std.mem.indexOfScalar(u8, payload, 0) ?? return null;
        const name = payload[0..nul1];
        if (nul1 + 1 > payload.len) return null;
        const rest = payload[nul1 + 1 ..];
        const nul2 = std.mem.indexOfScalar(u8, rest, 0) ?? rest.len;
        return PostgresParameterStatus{
            name: name,
            value: rest[0..nul2],
        };
    }
}

pub struct PostgresBackendKeyData {
    pub process_id: u32 = 0,
    pub secret_key: u32 = 0,

    pub static fn parse(payload: []const u8): PostgresBackendKeyData? {
        if (payload.len < 8) return null;
        const pid = ((payload[0] as u32) << 24) |
            ((payload[1] as u32) << 16) |
            ((payload[2] as u32) << 8) |
            (payload[3] as u32);
        const key = ((payload[4] as u32) << 24) |
            ((payload[5] as u32) << 16) |
            ((payload[6] as u32) << 8) |
            (payload[7] as u32);
        return PostgresBackendKeyData{
            process_id: pid,
            secret_key: key,
        };
    }
}

pub struct PostgresFieldDesc {
    pub name: []const u8 = "",
    pub table_oid: u32 = 0,
    pub column_attr: u16 = 0,
    pub type_oid: u32 = 0,
    pub type_size: i16 = 0,
    pub type_modifier: i32 = 0,
    pub format_code: u16 = 0,
}

pub const MAX_PG_FIELDS: usize = 64;

pub struct PostgresRowDescription {
    pub fields: [MAX_PG_FIELDS]PostgresFieldDesc = undefined,
    pub field_count: usize = 0,

    pub static fn parse(payload: []const u8): PostgresRowDescription? {
        if (payload.len < 2) return null;
        const count = (((payload[0] as u16) << 8) | (payload[1] as u16)) as usize;
        let desc = PostgresRowDescription{
            fields: undefined,
            field_count: 0,
        };
        let i: usize = 0;
        while (i < MAX_PG_FIELDS) : (i += 1) {
            desc.fields[i] = PostgresFieldDesc{};
        }

        let pos: usize = 2;
        let idx: usize = 0;
        while (idx < count) : (idx += 1) {
            const nul = std.mem.indexOfScalarPos(u8, payload, pos, 0) ?? return null;
            const col_name = payload[pos..nul];
            pos = nul + 1;
            if (pos + 18 > payload.len) return null;

            const table_oid = ((payload[pos] as u32) << 24) | ((payload[pos + 1] as u32) << 16) | ((payload[pos + 2] as u32) << 8) | (payload[pos + 3] as u32);
            const col_attr = ((payload[pos + 4] as u16) << 8) | (payload[pos + 5] as u16);
            const type_oid = ((payload[pos + 6] as u32) << 24) | ((payload[pos + 7] as u32) << 16) | ((payload[pos + 8] as u32) << 8) | (payload[pos + 9] as u32);
            const type_sz_u = ((payload[pos + 10] as u16) << 8) | (payload[pos + 11] as u16);
            const type_mod_u = ((payload[pos + 12] as u32) << 24) | ((payload[pos + 13] as u32) << 16) | ((payload[pos + 14] as u32) << 8) | (payload[pos + 15] as u32);
            const fmt_code = ((payload[pos + 16] as u16) << 8) | (payload[pos + 17] as u16);
            pos += 18;

            if (desc.field_count < MAX_PG_FIELDS) {
                desc.fields[desc.field_count] = PostgresFieldDesc{
                    name: col_name,
                    table_oid: table_oid,
                    column_attr: col_attr,
                    type_oid: type_oid,
                    type_size: @bitCast(type_sz_u),
                    type_modifier: @bitCast(type_mod_u),
                    format_code: fmt_code,
                };
                desc.field_count += 1;
            }
        }
        return desc;
    }

    pub fn getField(idx: usize): PostgresFieldDesc? {
        if (idx >= _.field_count) return null;
        return _.fields[idx];
    }

    pub fn findField(name: []const u8): PostgresFieldDesc? {
        let i: usize = 0;
        while (i < _.field_count) : (i += 1) {
            if (std.mem.eql(u8, _.fields[i].name, name)) {
                return _.fields[i];
            }
        }
        return null;
    }
}

pub struct PostgresReadyForQuery {
    pub status: PostgresTransactionStatus = PostgresTransactionStatus.Idle,

    pub static fn parse(payload: []const u8): PostgresReadyForQuery? {
        if (payload.len < 1) return null;
        return PostgresReadyForQuery{
            status: PostgresTransactionStatus.fromByte(payload[0]),
        };
    }
}

pub struct PostgresErrorField {
    pub code: u8 = 0,
    pub value: []const u8 = "",
}

pub struct PostgresErrorResponse {
    pub severity: []const u8 = "",
    pub sql_state: []const u8 = "",
    pub message: []const u8 = "",
    pub detail: []const u8 = "",
    pub hint: []const u8 = "",
    pub position: []const u8 = "",
    pub constraint: []const u8 = "",
    pub fields: [16]PostgresErrorField = undefined,
    pub field_count: usize = 0,

    pub static fn parse(payload: []const u8): PostgresErrorResponse {
        let err_resp = PostgresErrorResponse{
            fields: undefined,
            field_count: 0,
        };
        let i: usize = 0;
        while (i < 16) : (i += 1) {
            err_resp.fields[i] = PostgresErrorField{};
        }

        let pos: usize = 0;
        while (pos < payload.len) {
            const code = payload[pos];
            pos += 1;
            if (code == 0) break;
            const nul = std.mem.indexOfScalarPos(u8, payload, pos, 0) ?? payload.len;
            const val = payload[pos..nul];
            pos = if (nul < payload.len) nul + 1 else payload.len;

            if (err_resp.field_count < 16) {
                err_resp.fields[err_resp.field_count] = PostgresErrorField{ code: code, value: val };
                err_resp.field_count += 1;
            }
            if (code == 'S' || code == 'V') {
                if (code == 'S' || err_resp.severity.len == 0) err_resp.severity = val;
            } else if (code == 'C') {
                err_resp.sql_state = val;
            } else if (code == 'M') {
                err_resp.message = val;
            } else if (code == 'D') {
                err_resp.detail = val;
            } else if (code == 'H') {
                err_resp.hint = val;
            } else if (code == 'P') {
                err_resp.position = val;
            } else if (code == 'n') {
                err_resp.constraint = val;
            }
        }
        return err_resp;
    }

    pub fn getField(code: u8): ([]const u8)? {
        let i: usize = 0;
        while (i < _.field_count) : (i += 1) {
            if (_.fields[i].code == code) return _.fields[i].value;
        }
        return null;
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
        if (payload.len < 2) return null;
        const cols = ((payload[0] as u16) << 8) | (payload[1] as u16);
        return PostgresDataRow{
            payload: payload,
            column_count: cols,
            cursor: 2,
            current_col: 0,
        };
    }

    pub fn nextColumn(*_): PostgresColumnSlice? {
        if (_.current_col >= _.column_count || _.cursor + 4 > _.payload.len) return null;
        const b0 = _.payload[_.cursor] as u32;
        const b1 = _.payload[_.cursor + 1] as u32;
        const b2 = _.payload[_.cursor + 2] as u32;
        const b3 = _.payload[_.cursor + 3] as u32;
        const raw_len = (b0 << 24) | (b1 << 16) | (b2 << 8) | b3;
        _.cursor += 4;
        _.current_col += 1;

        // 0xFFFFFFFF (-1 in two's complement i32) indicates a SQL NULL column value
        if (raw_len == 0xFFFFFFFF) {
            return PostgresColumnSlice{
                is_null: true,
                data: "",
            };
        }

        const col_len = raw_len as usize;
        if (_.cursor + col_len > _.payload.len) return null;
        const slice = _.payload[_.cursor .. _.cursor + col_len];
        _.cursor += col_len;
        return PostgresColumnSlice{
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
                return PostgresCommandComplete{
                    tag: clean,
                    rows_affected: acc,
                };
            }
        }

        return PostgresCommandComplete{
            tag: clean,
            rows_affected: 0,
        };
    }
}

// ============================================================================
// RFC 5802 / RFC 7677 SCRAM-SHA-256 Client State Machine
// ============================================================================

pub struct ScramServerFirst {
    pub nonce: []const u8 = "",
    pub salt_b64: []const u8 = "",
    pub salt: [64]u8 = undefined,
    pub salt_len: usize = 0,
    pub iterations: u32 = 4096,
}

pub struct ScramSha256Client {
    pub client_nonce: [64]u8 = undefined,
    pub client_nonce_len: usize = 0,
    pub client_first_bare: [128]u8 = undefined,
    pub client_first_bare_len: usize = 0,
    pub expected_server_sig: [32]u8 = undefined,
    pub has_expected_sig: bool = false,

    pub static fn init(): ScramSha256Client {
        return ScramSha256Client{};
    }

    /// Generates `n,,n=<user>,r=<client_nonce>` and stores `client-first-bare` for final AuthMessage computation.
    pub fn buildClientFirst(*_, user: []const u8, nonce: []const u8, out: []u8): ([]const u8)! {
        if (nonce.len == 0 || nonce.len > 64) return error.InvalidNonce;
        @memcpy(_.client_nonce[0..nonce.len], nonce);
        _.client_nonce_len = nonce.len;

        const bare = std.fmt.bufPrint(&_.client_first_bare, "n={s},r={s}", .{ user, nonce }) catch return error.BufferTooSmall;
        _.client_first_bare_len = bare.len;

        return std.fmt.bufPrint(out, "n,,{s}", .{bare}) catch return error.BufferTooSmall;
    }

    /// Parses server-first-message `r=<combined_nonce>,s=<base64_salt>,i=<iterations>`.
    pub static fn parseServerFirst(server_first: []const u8): ScramServerFirst! {
        let res = ScramServerFirst{};
        let has_r = false;
        let has_s = false;
        let has_i = false;

        let iter = std.mem.splitScalar(u8, server_first, ',');
        while (part := iter.next()) {
            if (part.len < 2 || part[1] != '=') continue;
            const key = part[0];
            const val = part[2..];
            if (key == 'r') {
                res.nonce = val;
                has_r = true;
            } else if (key == 's') {
                res.salt_b64 = val;
                const dec_len = Base64.decode(&res.salt, val) catch return error.InvalidSalt;
                res.salt_len = dec_len;
                has_s = true;
            } else if (key == 'i') {
                res.iterations = std.fmt.parseInt(u32, val, 10) catch return error.InvalidIterations;
                has_i = true;
            }
        }
        if (!has_r || !has_s || !has_i || res.iterations == 0) {
            return error.InvalidScramServerFirst;
        }
        return res;
    }

    /// Computes RFC 7677 `client-final-message`: `c=biws,r=<combined_nonce>,p=<base64_client_proof>`.
    pub fn buildClientFinal(*_, password: []const u8, server_first: []const u8, out: []u8): ([]const u8)! {
        const sf = parseServerFirst(server_first)?;
        // Verify server nonce starts with our client nonce
        const cn = _.client_nonce[0.._.client_nonce_len];
        if (!std.mem.startsWith(u8, sf.nonce, cn)) {
            return error.ScramNonceMismatch;
        }

        // 1. SaltedPassword := PBKDF2-HMAC-SHA-256(password, salt, iterations)
        let salted_pw: [32]u8 = undefined;
        std.crypto.pwhash.pbkdf2(&salted_pw, password, sf.salt[0..sf.salt_len], sf.iterations, std.crypto.auth.hmac.sha2.HmacSha256) catch {
            return error.Pbkdf2Failed;
        };

        // 2. ClientKey := HMAC(SaltedPassword, "Client Key")
        let client_key: [32]u8 = undefined;
        std.crypto.auth.hmac.sha2.HmacSha256.create(&client_key, "Client Key", salted_pw[0..32]);

        // 3. StoredKey := SHA256(ClientKey)
        let stored_key: [32]u8 = undefined;
        std.crypto.hash.sha2.Sha256.hash(client_key[0..32], &stored_key, .{});

        // 4. ServerKey := HMAC(SaltedPassword, "Server Key")
        let server_key: [32]u8 = undefined;
        std.crypto.auth.hmac.sha2.HmacSha256.create(&server_key, "Server Key", salted_pw[0..32]);

        // 5. AuthMessage := client-first-bare + "," + server-first + "," + client-final-without-proof
        let without_proof_buf: [192]u8 = undefined;
        const without_proof = std.fmt.bufPrint(&without_proof_buf, "c=biws,r={s}", .{sf.nonce}) catch return error.BufferTooSmall;

        let auth_msg_buf: [512]u8 = undefined;
        const auth_msg = std.fmt.bufPrint(
            &auth_msg_buf,
            "{s},{s},{s}",
            .{ _.client_first_bare[0.._.client_first_bare_len], server_first, without_proof },
        ) catch return error.BufferTooSmall;

        // 6. ClientSignature := HMAC(StoredKey, AuthMessage)
        let client_sig: [32]u8 = undefined;
        std.crypto.auth.hmac.sha2.HmacSha256.create(&client_sig, auth_msg, stored_key[0..32]);

        // 7. ClientProof := ClientKey XOR ClientSignature
        let client_proof: [32]u8 = undefined;
        let k: usize = 0;
        while (k < 32) : (k += 1) {
            client_proof[k] = client_key[k] ^ client_sig[k];
        }

        // 8. Expected ServerSignature := HMAC(ServerKey, AuthMessage)
        std.crypto.auth.hmac.sha2.HmacSha256.create(&_.expected_server_sig, auth_msg, server_key[0..32]);
        _.has_expected_sig = true;

        let proof_b64: [64]u8 = undefined;
        const b64_len = Base64.encode(&proof_b64, &client_proof);

        return std.fmt.bufPrint(out, "{s},p={s}", .{ without_proof, proof_b64[0..b64_len] }) catch return error.BufferTooSmall;
    }

    /// Verifies server-final-message `v=<base64_server_signature>`.
    pub fn verifyServerFinal(server_final: []const u8): bool {
        if (!_.has_expected_sig) return false;
        if (!std.mem.startsWith(u8, server_final, "v=")) return false;
        const sig_b64 = server_final[2..];
        let decoded: [64]u8 = undefined;
        const dec_len = Base64.decode(&decoded, sig_b64) catch return false;
        if (dec_len != 32) return false;
        let diff: u8 = 0;
        let i: usize = 0;
        while (i < 32) : (i += 1) {
            diff = diff | (decoded[i] ^ _.expected_server_sig[i]);
        }
        return diff == 0;
    }
}
