import std from "std";
import { test, expect } from "cap:test";
import {
    CAP_LONG_PASSWORD,
    CAP_PROTOCOL_41,
    CAP_SECURE_CONNECTION,
    CAP_PLUGIN_AUTH,
    CAP_CONNECT_WITH_DB,
    CAP_CONNECT_ATTRS,
    ConnectAttr,
    HandshakeResponse41,
    encodeHandshakeResponse41,
    decodeHandshakeResponse41,
} from "cap:mysql_handshake";

test("mysql_handshake: full HandshakeResponse41 encode/decode roundtrip with attrs", () => {
    const attrs: [2]ConnectAttr = [
        ConnectAttr{ key: "_client_name", value: "cap-mysql" },
        ConnectAttr{ key: "_os", value: "linux" },
    ];
    const req = HandshakeResponse41{
        username: "root",
        auth_response: "scramble-20-bytes!!",
        database: "app_db",
        auth_plugin_name: "caching_sha2_password",
        connect_attrs: attrs[0..],
        sequence_id: 1,
    };

    let enc = encodeHandshakeResponse41(std.testing.allocator, req) catch unreachable;
    const parsed = decodeHandshakeResponse41(enc.slice()) catch unreachable;

    expect(parsed.sequence_id).toBe(1);
    expect((parsed.capability_flags & CAP_CONNECT_WITH_DB) != 0).toBeTrue();
    expect((parsed.capability_flags & CAP_CONNECT_ATTRS) != 0).toBeTrue();
    expect(parsed.username).toBe("root");
    expect(parsed.auth_response).toBe("scramble-20-bytes!!");
    expect(parsed.database).toBe("app_db");
    expect(parsed.auth_plugin_name).toBe("caching_sha2_password");
    expect(parsed.attrs_raw.len > 0).toBeTrue();
});

test("mysql_handshake: minimal packet without db/attrs and truncated error rejection", () => {
    const req = HandshakeResponse41{
        capability_flags: CAP_LONG_PASSWORD | CAP_PROTOCOL_41 | CAP_SECURE_CONNECTION | CAP_PLUGIN_AUTH,
        username: "guest",
        auth_response: "",
        database: "",
        auth_plugin_name: "mysql_native_password",
        sequence_id: 2,
    };

    let enc = encodeHandshakeResponse41(std.testing.allocator, req) catch unreachable;
    const parsed = decodeHandshakeResponse41(enc.slice()) catch unreachable;
    expect(parsed.sequence_id).toBe(2);
    expect((parsed.capability_flags & CAP_CONNECT_WITH_DB) == 0).toBeTrue();
    expect((parsed.capability_flags & CAP_CONNECT_ATTRS) == 0).toBeTrue();
    expect(parsed.username).toBe("guest");
    expect(parsed.database).toBe("");
    expect(parsed.auth_plugin_name).toBe("mysql_native_password");

    const short10: [10]u8 = [0; 10];
    const bad = decodeHandshakeResponse41(short10[0..]) catch null;
    expect(bad == null).toBeTrue();
});
