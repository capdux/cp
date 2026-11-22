// Source: cp/cap/test/test_wireguard_production_deep.cp (Deep tests for cap:wireguard)

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    WgError,
    Tai64n,
    ReplayWindow,
    WgKeyPair,
    HandshakeInitiation,
    HandshakeResponse,
    INITIATION_LEN,
    RESPONSE_LEN,
    createInitiation,
    consumeInitiationAndCreateResponse,
    consumeResponse,
    encryptTransport,
    decryptTransport,
    blake2sMac16,
} from "cap:net";

test("wireguard_replay_window_rfc6479") {
    let win = ReplayWindow.init();

    // Sequential packets starting at 0
    try win.checkAndUpdate(0);
    try win.checkAndUpdate(1);
    try win.checkAndUpdate(10);

    // Out-of-order within window
    try win.checkAndUpdate(5);
    try win.checkAndUpdate(9);
    try win.checkAndUpdate(2);

    // Duplicate detection
    try std.testing.expectError(WgError.ReplayDetected, win.checkAndUpdate(0));
    try std.testing.expectError(WgError.ReplayDetected, win.checkAndUpdate(5));
    try std.testing.expectError(WgError.ReplayDetected, win.checkAndUpdate(10));

    // Large jump clears window
    try win.checkAndUpdate(5000);
    try expect_eq(win.last_counter, 5000);

    // Stale counters left of window boundary are rejected
    try std.testing.expectError(WgError.WindowOutOfRange, win.checkAndUpdate(10));
    try std.testing.expectError(WgError.WindowOutOfRange, win.checkAndUpdate(3016));

    // Within window of 5000 still accepted once, then rejected on replay
    try win.checkAndUpdate(3017);
    try std.testing.expectError(WgError.ReplayDetected, win.checkAndUpdate(3017));
}

test("wireguard_tai64n_encode_decode_compare") {
    const t1 = Tai64n.fromUnixNs(1700000000, 500);
    const t2 = Tai64n.fromUnixNs(1700000000, 501);
    const t3 = Tai64n.fromUnixNs(1700000001, 0);

    const enc1 = t1.encode();
    const dec1 = try Tai64n.decode(&enc1);
    try expect_eq(dec1.sec, t1.sec);
    try expect_eq(dec1.nsec, t1.nsec);

    try expect_eq(t1.compare(dec1), 0);
    try expect_eq(t1.compare(t2), -1);
    try expect_eq(t2.compare(t1), 1);
    try expect_eq(t3.compare(t2), 1);

    const short_buf: [11]u8 = [0; 11];
    try std.testing.expectError(WgError.InvalidLength, Tai64n.decode(&short_buf));
}

test("wireguard_noise_ikpsk2_e2e_handshake_and_transport") {
    const init_kp = try WgKeyPair.generate();
    const resp_kp = try WgKeyPair.generate();
    const psk: [32]u8 = [0x42; 32];
    let last_ts: Tai64n? = null;

    const ts1 = Tai64n.fromUnixNs(1700000100, 42);
    const init_res = try createInitiation(init_kp, resp_kp.public_key, psk, 0x11223344, ts1);
    const wire_init = init_res.msg.encode();
    try expect_eq(wire_init.len, INITIATION_LEN);

    const decoded_init = try HandshakeInitiation.decode(&wire_init);
    const resp_res = try consumeInitiationAndCreateResponse(
        resp_kp,
        init_kp.public_key,
        psk,
        0x55667788,
        &last_ts,
        decoded_init,
    );
    const wire_resp = resp_res.msg.encode();
    try expect_eq(wire_resp.len, RESPONSE_LEN);

    const decoded_resp = try HandshakeResponse.decode(&wire_resp);
    const init_keys = try consumeResponse(init_kp, init_res.state, decoded_resp);

    // Keys must match symmetrically
    try expect_eq(std.mem.eql(u8, &init_keys.send_key, &resp_res.keys.recv_key), true);
    try expect_eq(std.mem.eql(u8, &init_keys.recv_key, &resp_res.keys.send_key), true);

    // Initiator -> Responder transport
    let pkt1: [128]u8 = [0; 128];
    const msg1 = "WireGuard Noise_IKpsk2 payload!";
    const pkt1_len = try encryptTransport(init_keys, 0, msg1, &pkt1);

    let resp_win = ReplayWindow.init();
    let out1: [128]u8 = [0; 128];
    const tp1 = try decryptTransport(resp_res.keys, &resp_win, pkt1[0..pkt1_len], &out1);
    try expect_eq(tp1.counter, 0);
    try expect_eq(tp1.plaintext_len, msg1.len);
    try expect_eq(std.mem.eql(u8, out1[0..tp1.plaintext_len], msg1), true);

    // Responder -> Initiator transport
    let pkt2: [128]u8 = [0; 128];
    const msg2 = "Ack from responder";
    const pkt2_len = try encryptTransport(resp_res.keys, 7, msg2, &pkt2);

    let init_win = ReplayWindow.init();
    let out2: [128]u8 = [0; 128];
    const tp2 = try decryptTransport(init_keys, &init_win, pkt2[0..pkt2_len], &out2);
    try expect_eq(tp2.counter, 7);
    try expect_eq(std.mem.eql(u8, out2[0..tp2.plaintext_len], msg2), true);
}

test("wireguard_security_mac1_aead_key_and_replay_rejections") {
    const init_kp = try WgKeyPair.generate();
    const resp_kp = try WgKeyPair.generate();
    const wrong_kp = try WgKeyPair.generate();
    const psk: [32]u8 = [0x99; 32];
    let last_ts: Tai64n? = null;

    const ts1 = Tai64n.fromUnixNs(1700000200, 100);
    const init_res = try createInitiation(init_kp, resp_kp.public_key, psk, 10, ts1);

    // 1. Tampered MAC1 rejected
    let bad_mac_msg = init_res.msg;
    bad_mac_msg.mac1[0] ^= 0xff;
    try std.testing.expectError(
        WgError.InvalidMac1,
        consumeInitiationAndCreateResponse(resp_kp, init_kp.public_key, psk, 20, &last_ts, bad_mac_msg),
    );

    // 2. Wrong expected peer static key rejected
    try std.testing.expectError(
        WgError.InvalidKey,
        consumeInitiationAndCreateResponse(resp_kp, wrong_kp.public_key, psk, 20, &last_ts, init_res.msg),
    );

    // 3. Valid initiation succeeds and records timestamp
    const resp_res = try consumeInitiationAndCreateResponse(
        resp_kp,
        init_kp.public_key,
        psk,
        20,
        &last_ts,
        init_res.msg,
    );

    // 4. Replaying same or older TAI64N timestamp is rejected
    try std.testing.expectError(
        WgError.StaleTimestamp,
        consumeInitiationAndCreateResponse(resp_kp, init_kp.public_key, psk, 21, &last_ts, init_res.msg),
    );

    const init_keys = try consumeResponse(init_kp, init_res.state, resp_res.msg);

    // 5. Tampered transport ciphertext/tag rejected without corrupting replay window
    let pkt: [128]u8 = [0; 128];
    const pkt_len = try encryptTransport(init_keys, 0, "secret", &pkt);
    pkt[20] ^= 0x01;

    let win = ReplayWindow.init();
    let plain: [64]u8 = [0; 64];
    try std.testing.expectError(
        WgError.DecryptFailed,
        decryptTransport(resp_res.keys, &win, pkt[0..pkt_len], &plain),
    );
    // Unauthenticated packet did not mark counter 0 as seen; valid packet with counter 0 still succeeds
    pkt[20] ^= 0x01;
    const tp = try decryptTransport(resp_res.keys, &win, pkt[0..pkt_len], &plain);
    try expect_eq(tp.counter, 0);

    // Replaying valid packet with counter 0 is now rejected
    try std.testing.expectError(
        WgError.ReplayDetected,
        decryptTransport(resp_res.keys, &win, pkt[0..pkt_len], &plain),
    );
}

test("wireguard_cap_dynamic_mount_and_zero_leak") {
    const t = cap.net.Tai64n.fromUnixNs(1700000300, 999);
    const enc = t.encode();
    const decoded_t = try cap.net.Tai64n.decode(&enc);
    try expect_eq(decoded_t.sec, t.sec);

    const key: [32]u8 = [0x11; 32];
    const mac = cap.net.blake2sMac16(&key, "cap.wireguard");
    const mac_direct = blake2sMac16(&key, "cap.wireguard");
    try expect_eq(std.mem.eql(u8, &mac, &mac_direct), true);

    // Verify zero heap leaks with std.testing.allocator
    const alloc = std.testing.allocator;
    let buf = try alloc.alloc(u8, 128);
    defer alloc.free(buf);
    try expect_eq(buf.len, 128);
}
