// cap/test/test_sql_redis_hpack.cp
// End-to-End Regression Test for Deepened SQL (Postgres v3 & MySQL), Redis/Valkey Commands & Pipeline, and HTTP/2 HPACK

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    PostgresAuthKind,
    PostgresAuthChallenge,
    PostgresDataRow,
    PostgresCommandComplete,
    MySqlLenEnc,
    MySqlCapabilities,
    MySqlOkPacket,
    MySqlErrPacket
} from "cap:sql";
import {
    RESPType,
    RESPParser,
    RedisCommand,
    RedisPipeline
} from "cap:redis";
import {
    HpackCodec
} from "cap:http";

fn main() {
    test("1. cap:sql — PostgreSQL v3 Authentication (MD5 & SASL), DataRow Iterator & CommandComplete", () => {
        // 1. MD5Password challenge: code = 5 (0x00000005), 4-byte salt = [0xDE, 0xAD, 0xBE, 0xEF]
        const md5_payload: [8]u8 = [0, 0, 0, 5, 0xDE, 0xAD, 0xBE, 0xEF];
        const md5_auth = PostgresAuthChallenge.parse(md5_payload[0..8]).?;
        expect_eq(md5_auth.kind == PostgresAuthKind.MD5Password, true);
        expect_eq(md5_auth.md5_salt[0], 0xDE);
        expect_eq(md5_auth.md5_salt[3], 0xEF);

        // 2. SASL challenge: code = 10, "SCRAM-SHA-256\0"
        const sasl_payload = "\x00\x00\x00\x0ASCRAM-SHA-256\x00";
        const sasl_auth = PostgresAuthChallenge.parse(sasl_payload).?;
        expect_eq(sasl_auth.kind == PostgresAuthKind.SASL, true);
        expect_eq(std.mem.eql(u8, sasl_auth.sasl_mechanism, "SCRAM-SHA-256"), true);

        // 3. DataRow iterator: 2 columns -> col1 = "alice" (len 5), col2 = NULL (-1 / 0xFFFFFFFF)
        const row_payload = "\x00\x02\x00\x00\x00\x05alice\xFF\xFF\xFF\xFF";
        let row = PostgresDataRow.init(row_payload).?;
        expect_eq(row.column_count, 2);

        const col1 = row.nextColumn().?;
        expect_eq(col1.is_null, false);
        expect_eq(std.mem.eql(u8, col1.data, "alice"), true);

        const col2 = row.nextColumn().?;
        expect_eq(col2.is_null, true);
        expect_eq(row.nextColumn() == null, true);

        // 4. CommandComplete row count extraction
        const cc1 = PostgresCommandComplete.parse("INSERT 0 42\x00");
        expect_eq(cc1.rows_affected, 42);
        const cc2 = PostgresCommandComplete.parse("UPDATE 7");
        expect_eq(cc2.rows_affected, 7);
    });

    test("2. cap:sql — MySQL Length-Encoded Integer, Capabilities, OK_Packet & ERR_Packet", () => {
        // 1. lenenc_int 1-byte and 3-byte (0xFC + u16 LE)
        let buf: [16]u8 = [0; 16];
        expect_eq(MySqlLenEnc.encode(buf[0..16], 250), 1);
        const dec1 = MySqlLenEnc.decode(buf[0..1]).?;
        expect_eq(dec1.value, 250);
        expect_eq(dec1.bytes_used, 1);

        expect_eq(MySqlLenEnc.encode(buf[0..16], 1024), 3);
        expect_eq(buf[0], 0xFC);
        const dec2 = MySqlLenEnc.decode(buf[0..3]).?;
        expect_eq(dec2.value, 1024);
        expect_eq(dec2.bytes_used, 3);

        // 2. Capabilities bitmask
        let caps = MySqlCapabilities.defaultClient();
        expect_eq(caps.has(MySqlCapabilities.flagProtocol41()), true);
        expect_eq(caps.has(MySqlCapabilities.flagSsl()), false);
        caps.enable(MySqlCapabilities.flagSsl());
        expect_eq(caps.has(MySqlCapabilities.flagSsl()), true);
        caps.disable(MySqlCapabilities.flagSsl());
        expect_eq(caps.has(MySqlCapabilities.flagSsl()), false);

        // 3. OK_Packet: 0x00, affected_rows=3, last_insert_id=99, status=0x0002, warnings=0
        const ok_raw: [7]u8 = [0x00, 3, 99, 0x02, 0x00, 0x00, 0x00];
        const ok_pkt = MySqlOkPacket.parse(ok_raw[0..7]).?;
        expect_eq(ok_pkt.affected_rows, 3);
        expect_eq(ok_pkt.last_insert_id, 99);
        expect_eq(ok_pkt.status_flags, 2);

        // 4. ERR_Packet: 0xFF, code=1045 (0x0415 LE -> 0x15, 0x04), "#28000Access denied"
        const err_raw = "\xFF\x15\x04#28000Access denied";
        const err_pkt = MySqlErrPacket.parse(err_raw).?;
        expect_eq(err_pkt.error_code, 1045);
        expect_eq(std.mem.eql(u8, err_pkt.sql_state, "28000"), true);
        expect_eq(std.mem.eql(u8, err_pkt.message, "Access denied"), true);
    });

    test("3. cap:redis — RedisCommand Builders & RedisPipeline Batching", () => {
        let cmd_buf: [128]u8 = [0; 128];
        const hset_len = RedisCommand.encodeHSet(cmd_buf[0..128], "user:1", "role", "admin");
        expect_eq(std.mem.eql(u8, cmd_buf[0..hset_len], "*4\r\n$4\r\nHSET\r\n$6\r\nuser:1\r\n$4\r\nrole\r\n$5\r\nadmin\r\n"), true);

        const exp_len = RedisCommand.encodeExpire(cmd_buf[0..128], "session:9", 3600);
        expect_eq(std.mem.eql(u8, cmd_buf[0..exp_len], "*3\r\n$6\r\nEXPIRE\r\n$9\r\nsession:9\r\n$4\r\n3600\r\n"), true);

        let pipe = RedisPipeline.init();
        expect_eq(pipe.ping(), true);
        expect_eq(pipe.set("k1", "v1"), true);
        expect_eq(pipe.get("k1"), true);
        expect_eq(pipe.command_count, 3);

        const batch = pipe.bytes();
        const f1 = RESPParser.parseFrame(batch).?;
        expect_eq(f1.kind == RESPType.Array, true);
        expect_eq(f1.array_count, 1);
    });

    test("4. cap:http — HTTP/2 HPACK Prefix Int, Static Table & Header Block Codec", () => {
        // 1. RFC 7541 §C.1.1 / §C.1.2 Prefix integer roundtrip (value = 1337 with 5-bit prefix)
        let int_buf: [8]u8 = [0; 8];
        const written = HpackCodec.encodePrefixInt(int_buf[0..8], 1337, 5, 0x00);
        expect_eq(written, 3);
        expect_eq(int_buf[0], 31);
        expect_eq(int_buf[1], 154);
        expect_eq(int_buf[2], 10);

        const dec_int = HpackCodec.decodePrefixInt(int_buf[0..written], 5).?;
        expect_eq(dec_int.value, 1337);
        expect_eq(dec_int.bytes_read, 3);

        // 2. Indexed static header representation (`:method: GET` -> index 2 -> single byte 0x82)
        let hdr_buf: [64]u8 = [0; 64];
        const idx_len = HpackCodec.encodeHeader(hdr_buf[0..64], ":method", "GET", false);
        expect_eq(idx_len, 1);
        expect_eq(hdr_buf[0], 0x82);

        const dec_idx = HpackCodec.decodeHeader(hdr_buf[0..idx_len]).?;
        expect_eq(std.mem.eql(u8, dec_idx.name, ":method"), true);
        expect_eq(std.mem.eql(u8, dec_idx.value, "GET"), true);

        // 3. Literal header field (never indexed `authorization: secret-token`)
        const lit_len = HpackCodec.encodeHeader(hdr_buf[0..64], "x-custom-key", "cp-runtime", true);
        const dec_lit = HpackCodec.decodeHeader(hdr_buf[0..lit_len]).?;
        expect_eq(dec_lit.never_index, true);
        expect_eq(std.mem.eql(u8, dec_lit.name, "x-custom-key"), true);
        expect_eq(std.mem.eql(u8, dec_lit.value, "cp-runtime"), true);
    });
}
