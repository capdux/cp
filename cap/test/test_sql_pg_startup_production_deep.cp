import std from "std";
import { test, expect_eq } from "cap:test";
import {
    PG_PROTOCOL_V3_0,
    PG_SSL_REQUEST_CODE,
    PG_CANCEL_REQUEST_CODE,
    PG_MAX_STARTUP_PARAMS,
    PgStartupCodecError,
    PgStartupParam,
    PgStartupConfig,
    PgStartupParamIter,
    PgDecodedStartup,
    encodePgStartup,
    decodePgStartup,
    encodePgSslRequestFrame,
    isPgSslRequestFrame,
    encodePgCancelFrame,
} from "cap:sql";

test("1. encodePgStartup defaults database to user and client_encoding to UTF8", () => {
    let buf: [128]u8 = [0; 128];
    const wire = encodePgStartup(buf[0..], PgStartupConfig{
        user: "alice",
    }) catch unreachable;

    // 末尾应为 NUL 终止符
    expect_eq(wire[wire.len - 1], u8(0));

    const decoded = decodePgStartup(wire) catch unreachable;
    expect_eq(decoded.protocol_version, PG_PROTOCOL_V3_0);
    expect_eq(decoded.total_len, wire.len);
    expect_eq(decoded.param_count, usize(3));
    expect_eq(std.mem.eql(u8, decoded.user, "alice"), true);
    expect_eq(std.mem.eql(u8, decoded.database, "alice"), true);
    expect_eq(std.mem.eql(u8, decoded.client_encoding, "UTF8"), true);
    expect_eq(decoded.application_name.len, usize(0));
});

test("2. encodePgStartup with explicit database, application_name, and extra_params iteration", () => {
    let buf: [256]u8 = [0; 256];
    const extras: [2]PgStartupParam = [
        PgStartupParam{ key: "TimeZone", value: "UTC" },
        PgStartupParam{ key: "search_path", value: "app,public" },
    ];
    const wire = encodePgStartup(buf[0..], PgStartupConfig{
        user: "svc_user",
        database: "prod_db",
        client_encoding: "UTF8",
        application_name: "cap-worker-01",
        extra_params: extras[0..],
    }) catch unreachable;

    const decoded = decodePgStartup(wire) catch unreachable;
    expect_eq(decoded.param_count, usize(6));
    expect_eq(std.mem.eql(u8, decoded.user, "svc_user"), true);
    expect_eq(std.mem.eql(u8, decoded.database, "prod_db"), true);
    expect_eq(std.mem.eql(u8, decoded.application_name, "cap-worker-01"), true);

    let it = decoded.iterParams();
    let found_tz = false;
    let found_sp = false;
    let count: usize = 0;
    while (p := it.next() catch unreachable) {
        count += 1;
        if (std.mem.eql(u8, p.key, "TimeZone") && std.mem.eql(u8, p.value, "UTC")) {
            found_tz = true;
        }
        if (std.mem.eql(u8, p.key, "search_path") && std.mem.eql(u8, p.value, "app,public")) {
            found_sp = true;
        }
    }
    expect_eq(count, usize(6));
    expect_eq(found_tz, true);
    expect_eq(found_sp, true);
});

test("3. SSLRequest 8-byte frame encode and validation", () => {
    let buf: [16]u8 = [0; 16];
    const frame = encodePgSslRequestFrame(buf[0..]) catch unreachable;
    expect_eq(frame.len, usize(8));
    expect_eq(isPgSslRequestFrame(frame), true);
    expect_eq(PG_SSL_REQUEST_CODE, u32(80877103));

    // 截断或篡改魔数应返回 false
    expect_eq(isPgSslRequestFrame(frame[0..7]), false);
    let bad: [8]u8 = [0, 0, 0, 8, 0, 3, 0, 0];
    expect_eq(isPgSslRequestFrame(bad[0..]), false);
});

test("4. CancelRequest 16-byte frame encodes exact BE length, magic, pid, and secret key", () => {
    let buf: [16]u8 = [0; 16];
    const frame = encodePgCancelFrame(buf[0..], 0x01020304, 0xAABBCCDD) catch unreachable;
    expect_eq(frame.len, usize(16));
    expect_eq(PG_CANCEL_REQUEST_CODE, u32(80877102));

    // length = 16
    expect_eq(frame[3], u8(16));
    // magic = 0x04D2162E
    expect_eq(frame[4], u8(0x04));
    expect_eq(frame[5], u8(0xD2));
    expect_eq(frame[6], u8(0x16));
    expect_eq(frame[7], u8(0x2E));
    // pid = 0x01020304
    expect_eq(frame[8], u8(0x01));
    expect_eq(frame[11], u8(0x04));
    // secret = 0xAABBCCDD
    expect_eq(frame[12], u8(0xAA));
    expect_eq(frame[15], u8(0xDD));
});

test("5. Encoder guards: MissingUser, BufferTooSmall, and TooManyParams", () => {
    let buf: [128]u8 = [0; 128];
    let e_user = false;
    * = encodePgStartup(buf[0..], PgStartupConfig{ user: "" }) catch |e| {
        if (e == PgStartupCodecError.MissingUser) e_user = true;
    };
    expect_eq(e_user, true);

    let tiny: [10]u8 = [0; 10];
    let e_small = false;
    * = encodePgStartup(tiny[0..], PgStartupConfig{ user: "postgres" }) catch |e| {
        if (e == PgStartupCodecError.BufferTooSmall) e_small = true;
    };
    expect_eq(e_small, true);

    expect_eq(PG_MAX_STARTUP_PARAMS, usize(32));
    const many: [31]PgStartupParam = [PgStartupParam{ key: "k", value: "v" }; 31];
    let big: [512]u8 = [0; 512];
    let e_many = false;
    * = encodePgStartup(big[0..], PgStartupConfig{
        user: "u",
        extra_params: many[0..],
    }) catch |e| {
        if (e == PgStartupCodecError.TooManyParams) e_many = true;
    };
    expect_eq(e_many, true);
});

test("6. Decoder malformed packet guards: short, invalid length, truncated, wrong version, unterminated", () => {
    // 1. 过短报文
    const short_pkt: [4]u8 = [0, 0, 0, 4];
    let e1 = false;
    * = decodePgStartup(short_pkt[0..]) catch |e| {
        if (e == PgStartupCodecError.InvalidMessage) e1 = true;
    };
    expect_eq(e1, true);

    // 2. 声明长度 < 9
    const bad_len: [10]u8 = [0, 0, 0, 8, 0, 3, 0, 0, 0, 0];
    let e2 = false;
    * = decodePgStartup(bad_len[0..]) catch |e| {
        if (e == PgStartupCodecError.InvalidLength) e2 = true;
    };
    expect_eq(e2, true);

    // 3. 声明长度超过缓冲区长度
    const trunc_pkt: [10]u8 = [0, 0, 0, 50, 0, 3, 0, 0, 0, 0];
    let e3 = false;
    * = decodePgStartup(trunc_pkt[0..]) catch |e| {
        if (e == PgStartupCodecError.TruncatedPayload) e3 = true;
    };
    expect_eq(e3, true);

    // 4. 非 3.0 协议版本 (2.0 = 0x00020000)
    const old_ver: [10]u8 = [0, 0, 0, 10, 0, 2, 0, 0, 0, 0];
    let e4 = false;
    * = decodePgStartup(old_ver[0..]) catch |e| {
        if (e == PgStartupCodecError.UnsupportedProtocol) e4 = true;
    };
    expect_eq(e4, true);

    // 5. 缺少参数终止 NUL
    const unterm: [12]u8 = [0, 0, 0, 12, 0, 3, 0, 0, 'u', 's', 'e', 'r'];
    let e5 = false;
    * = decodePgStartup(unterm[0..]) catch |e| {
        if (e == PgStartupCodecError.UnterminatedParam) e5 = true;
    };
    expect_eq(e5, true);
});
