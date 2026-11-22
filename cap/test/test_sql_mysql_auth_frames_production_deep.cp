// cap/test/test_sql_mysql_auth_frames_production_deep.cp
// Deep production tests for cap:sql MySQL AuthSwitch, SSLRequest & LocalInfileRequest Codec (mysql_auth_frames.cp)

import { test, expect_eq, expect_true, expect_false } from "cap:test";
import {
    MYSQL_AUTH_SWITCH_HEADER,
    MYSQL_LOCAL_INFILE_HEADER,
    MYSQL_SSL_REQUEST_PAYLOAD_LEN,
    MYSQL_CAP_CLIENT_SSL,
    MySqlSslRequestView,
    encodeMySqlAuthSwitchRequest,
    decodeMySqlAuthSwitchRequest,
    encodeMySqlAuthSwitchResponse,
    decodeMySqlAuthSwitchResponse,
    encodeMySqlSslRequestFrame,
    decodeMySqlSslRequestFrame,
    encodeMySqlLocalInfileRequest,
    decodeMySqlLocalInfileRequest,
} from "cap:sql";

test("mysql_auth_frames: AuthSwitchRequest & AuthSwitchResponse roundtrip", () => {
    expect_eq(MYSQL_AUTH_SWITCH_HEADER, 0xfe);
    expect_eq(MYSQL_LOCAL_INFILE_HEADER, 0xfb);
    expect_eq(MYSQL_SSL_REQUEST_PAYLOAD_LEN, 32);

    let buf: [128]u8 = [0; 128];
    const req_wire = encodeMySqlAuthSwitchRequest(
        buf[0..],
        2,
        "caching_sha2_password",
        "12345678901234567890",
    ) catch { expect_true(false); return; };
    expect_eq(req_wire[3], 2);
    expect_eq(req_wire[4], MYSQL_AUTH_SWITCH_HEADER);

    const req = decodeMySqlAuthSwitchRequest(req_wire) catch { expect_true(false); return; };
    expect_eq(req.sequence_id, 2);
    expect_eq(req.plugin_name, "caching_sha2_password");
    expect_eq(req.plugin_data, "12345678901234567890");

    // Empty plugin_data (e.g. mysql_clear_password)
    const req2_wire = encodeMySqlAuthSwitchRequest(buf[0..], 4, "mysql_clear_password", "") catch {
        expect_true(false);
        return;
    };
    const req2 = decodeMySqlAuthSwitchRequest(req2_wire) catch { expect_true(false); return; };
    expect_eq(req2.plugin_name, "mysql_clear_password");
    expect_eq(req2.plugin_data, "");

    // AuthSwitchResponse roundtrip (binary token & empty token)
    const token: [4]u8 = [0xde, 0xad, 0xbe, 0xef];
    const resp_wire = encodeMySqlAuthSwitchResponse(buf[0..], 3, token[0..]) catch {
        expect_true(false);
        return;
    };
    expect_eq(resp_wire.len, 8);
    const resp = decodeMySqlAuthSwitchResponse(resp_wire) catch { expect_true(false); return; };
    expect_eq(resp.sequence_id, 3);
    expect_eq(resp.auth_response.len, 4);
    expect_eq(resp.auth_response[0], 0xde);
    expect_eq(resp.auth_response[3], 0xef);

    const empty_resp_wire = encodeMySqlAuthSwitchResponse(buf[0..], 5, "") catch {
        expect_true(false);
        return;
    };
    expect_eq(empty_resp_wire.len, 4);
    const empty_resp = decodeMySqlAuthSwitchResponse(empty_resp_wire) catch { expect_true(false); return; };
    expect_eq(empty_resp.auth_response, "");
});

test("mysql_auth_frames: SSLRequest 36-byte fixed frame roundtrip", () => {
    let buf: [64]u8 = [0; 64];
    const ssl_in = MySqlSslRequestView{
        sequence_id: 1,
        capabilities: MYSQL_CAP_CLIENT_SSL | 0x0008a205,
        max_packet_size: 16777216,
        character_set: 45,
    };
    expect_true(ssl_in.hasSslFlag());

    const wire = encodeMySqlSslRequestFrame(buf[0..], ssl_in) catch {
        expect_true(false);
        return;
    };
    expect_eq(wire.len, 36);
    expect_eq(wire[0], 32);
    expect_eq(wire[3], 1);
    // Verify 23-byte reserved area is zeroed
    expect_eq(wire[13], 0);
    expect_eq(wire[35], 0);

    const ssl_out = decodeMySqlSslRequestFrame(wire) catch { expect_true(false); return; };
    expect_eq(ssl_out.sequence_id, 1);
    expect_eq(ssl_out.capabilities, ssl_in.capabilities);
    expect_eq(ssl_out.max_packet_size, 16777216);
    expect_eq(ssl_out.character_set, 45);
    expect_true(ssl_out.hasSslFlag());
});

test("mysql_auth_frames: LOCAL_INFILE_Request roundtrip & path traversal guard", () => {
    let buf: [128]u8 = [0; 128];
    const safe_wire = encodeMySqlLocalInfileRequest(buf[0..], 1, "data/users.csv") catch {
        expect_true(false);
        return;
    };
    expect_eq(safe_wire[4], MYSQL_LOCAL_INFILE_HEADER);
    const safe_req = decodeMySqlLocalInfileRequest(safe_wire) catch { expect_true(false); return; };
    expect_eq(safe_req.sequence_id, 1);
    expect_eq(safe_req.filename, "data/users.csv");
    expect_true(safe_req.isSafeRelativeFilename());

    // Absolute path and parent traversal flagged unsafe by isSafeRelativeFilename()
    const abs_wire = encodeMySqlLocalInfileRequest(buf[0..], 1, "/etc/passwd") catch {
        expect_true(false);
        return;
    };
    const abs_req = decodeMySqlLocalInfileRequest(abs_wire) catch { expect_true(false); return; };
    expect_eq(abs_req.filename, "/etc/passwd");
    expect_false(abs_req.isSafeRelativeFilename());

    const trav_wire = encodeMySqlLocalInfileRequest(buf[0..], 1, "uploads/../../secret.key") catch {
        expect_true(false);
        return;
    };
    const trav_req = decodeMySqlLocalInfileRequest(trav_wire) catch { expect_true(false); return; };
    expect_false(trav_req.isSafeRelativeFilename());
});

test("mysql_auth_frames: SSLRequest & AuthSwitch missing capability/terminator guards", () => {
    let buf: [64]u8 = [0; 64];

    // SSLRequest without CLIENT_SSL flag rejected on encode
    let f1 = false;
    * = encodeMySqlSslRequestFrame(buf[0..], MySqlSslRequestView{
        sequence_id: 1,
        capabilities: 0x00000200, // missing CLIENT_SSL
        max_packet_size: 16777216,
        character_set: 45,
    }) catch { f1 = true; };
    expect_true(f1);

    // SSLRequest with wrong payload length (16 instead of 32) rejected on decode
    const short_ssl: [20]u8 = [
        16, 0, 0, 1,
        0x00, 0x08, 0, 0,
        0, 0, 0, 1,
        45, 0, 0, 0,
        0, 0, 0, 0,
    ];
    let f2 = false;
    * = decodeMySqlSslRequestFrame(short_ssl[0..]) catch { f2 = true; };
    expect_true(f2);

    // AuthSwitchRequest missing NUL terminator after plugin_name
    const no_nul: [8]u8 = [4, 0, 0, 2, 0xfe, 'a', 'b', 'c'];
    let f3 = false;
    * = decodeMySqlAuthSwitchRequest(no_nul[0..]) catch { f3 = true; };
    expect_true(f3);

    // AuthSwitchRequest wrong header (0x00 instead of 0xfe)
    const bad_hdr: [8]u8 = [4, 0, 0, 2, 0x00, 'a', 0, 'x'];
    let f4 = false;
    * = decodeMySqlAuthSwitchRequest(bad_hdr[0..]) catch { f4 = true; };
    expect_true(f4);
});

test("mysql_auth_frames: LocalInfile & buffer boundary error guards", () => {
    let buf: [32]u8 = [0; 32];

    // Empty filename rejected on encode
    let f1 = false;
    * = encodeMySqlLocalInfileRequest(buf[0..], 1, "") catch { f1 = true; };
    expect_true(f1);

    // Empty filename (payload len = 1, only 0xFB) rejected on decode
    const empty_li: [5]u8 = [1, 0, 0, 1, 0xfb];
    let f2 = false;
    * = decodeMySqlLocalInfileRequest(empty_li[0..]) catch { f2 = true; };
    expect_true(f2);

    // Wrong header on LocalInfile
    const bad_li: [7]u8 = [3, 0, 0, 1, 0xfe, 'a', 'b'];
    let f3 = false;
    * = decodeMySqlLocalInfileRequest(bad_li[0..]) catch { f3 = true; };
    expect_true(f3);

    // BufferTooSmall
    let tiny: [3]u8 = [0; 3];
    let f4 = false;
    * = encodeMySqlAuthSwitchRequest(tiny[0..], 1, "p", "d") catch { f4 = true; };
    expect_true(f4);

    // Trailing bytes rejected
    const trailing_li: [8]u8 = [2, 0, 0, 1, 0xfb, 'a', 0xaa, 0xbb];
    let f5 = false;
    * = decodeMySqlLocalInfileRequest(trailing_li[0..]) catch { f5 = true; };
    expect_true(f5);
});
