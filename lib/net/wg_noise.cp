// Source: cp/lib/wireguard/noise.cp (WireGuard Noise_IKpsk2 handshake, KDF & transport AEAD)

const std = @import("std");
const X25519 = std.crypto.dh.X25519;
const Blake2s256 = std.crypto.hash.blake2.Blake2s256;
const Blake2s128 = std.crypto.hash.blake2.Blake2s(128);
const ChaCha20Poly1305 = std.crypto.aead.chacha_poly.ChaCha20Poly1305;

import { WgError, Tai64n, ReplayWindow } from "./wg_replay.cp";

pub const MSG_INITIATION_TYPE: u8 = 1;
pub const MSG_RESPONSE_TYPE: u8 = 2;
pub const MSG_TRANSPORT_TYPE: u8 = 4;

pub const INITIATION_LEN: usize = 148;
pub const RESPONSE_LEN: usize = 92;
pub const TRANSPORT_HEADER_LEN: usize = 16;
pub const AEAD_TAG_LEN: usize = 16;

const CONSTRUCTION: []const u8 = "Noise_IKpsk2_25519_ChaChaPoly_BLAKE2s";
const IDENTIFIER: []const u8 = "WireGuard v1 zx2c4 Jason@zx2c4.com";
const LABEL_MAC1: []const u8 = "mac1----";

pub struct WgKeyPair {
    pub private_key: [32]u8,
    pub public_key: [32]u8,

    pub fn generate() -> !WgKeyPair {
        const kp = X25519.KeyPair.generate();
        return WgKeyPair {
            .private_key = kp.secret_key,
            .public_key = kp.public_key,
        };
    }

    pub fn fromPrivate(priv_key: [32]u8) -> !WgKeyPair {
        const pub_k = X25519.recoverPublicKey(priv_key) catch return WgError.InvalidKey;
        return WgKeyPair {
            .private_key = priv_key,
            .public_key = pub_k,
        };
    }
}

pub struct WgSessionKeys {
    pub send_key: [32]u8,
    pub recv_key: [32]u8,
    pub local_index: u32,
    pub remote_index: u32,
}

pub struct HandshakeInitiation {
    pub sender_index: u32,
    pub ephemeral: [32]u8,
    pub encrypted_static: [48]u8,
    pub encrypted_timestamp: [28]u8,
    pub mac1: [16]u8,
    pub mac2: [16]u8,

    pub fn encode() -> [148]u8 {
        let out: [148]u8 = [0; 148];
        out[0] = MSG_INITIATION_TYPE;
        std.mem.writeInt(u32, out[4..8], _.sender_index, .little);
        @memcpy(out[8..40], &_.ephemeral);
        @memcpy(out[40..88], &_.encrypted_static);
        @memcpy(out[88..116], &_.encrypted_timestamp);
        @memcpy(out[116..132], &_.mac1);
        @memcpy(out[132..148], &_.mac2);
        return out;
    }

    pub fn decode(data: []const u8) -> !HandshakeInitiation {
        if (data.len != INITIATION_LEN) {
            return WgError.InvalidLength;
        }
        if (data[0] != MSG_INITIATION_TYPE or data[1] != 0 or data[2] != 0 or data[3] != 0) {
            return WgError.InvalidMessageType;
        }
        let msg = HandshakeInitiation {
            .sender_index = std.mem.readInt(u32, data[4..8], .little),
            .ephemeral = [0; 32],
            .encrypted_static = [0; 48],
            .encrypted_timestamp = [0; 28],
            .mac1 = [0; 16],
            .mac2 = [0; 16],
        };
        @memcpy(&msg.ephemeral, data[8..40]);
        @memcpy(&msg.encrypted_static, data[40..88]);
        @memcpy(&msg.encrypted_timestamp, data[88..116]);
        @memcpy(&msg.mac1, data[116..132]);
        @memcpy(&msg.mac2, data[132..148]);
        return msg;
    }
}

pub struct HandshakeResponse {
    pub sender_index: u32,
    pub receiver_index: u32,
    pub ephemeral: [32]u8,
    pub encrypted_nothing: [16]u8,
    pub mac1: [16]u8,
    pub mac2: [16]u8,

    pub fn encode() -> [92]u8 {
        let out: [92]u8 = [0; 92];
        out[0] = MSG_RESPONSE_TYPE;
        std.mem.writeInt(u32, out[4..8], _.sender_index, .little);
        std.mem.writeInt(u32, out[8..12], _.receiver_index, .little);
        @memcpy(out[12..44], &_.ephemeral);
        @memcpy(out[44..60], &_.encrypted_nothing);
        @memcpy(out[60..76], &_.mac1);
        @memcpy(out[76..92], &_.mac2);
        return out;
    }

    pub fn decode(data: []const u8) -> !HandshakeResponse {
        if (data.len != RESPONSE_LEN) {
            return WgError.InvalidLength;
        }
        if (data[0] != MSG_RESPONSE_TYPE or data[1] != 0 or data[2] != 0 or data[3] != 0) {
            return WgError.InvalidMessageType;
        }
        let msg = HandshakeResponse {
            .sender_index = std.mem.readInt(u32, data[4..8], .little),
            .receiver_index = std.mem.readInt(u32, data[8..12], .little),
            .ephemeral = [0; 32],
            .encrypted_nothing = [0; 16],
            .mac1 = [0; 16],
            .mac2 = [0; 16],
        };
        @memcpy(&msg.ephemeral, data[12..44]);
        @memcpy(&msg.encrypted_nothing, data[44..60]);
        @memcpy(&msg.mac1, data[60..76]);
        @memcpy(&msg.mac2, data[76..92]);
        return msg;
    }
}

pub struct TransportPacket {
    pub receiver_index: u32,
    pub counter: u64,
    pub plaintext_len: usize,
}

pub struct NoiseInitiatorState {
    pub chaining_key: [32]u8,
    pub hash: [32]u8,
    pub ephemeral_private: [32]u8,
    pub sender_index: u32,
    pub remote_static_public: [32]u8,
    pub psk: [32]u8,
}

pub struct InitiationResult {
    pub msg: HandshakeInitiation,
    pub state: NoiseInitiatorState,
}

pub struct ResponderResult {
    pub msg: HandshakeResponse,
    pub keys: WgSessionKeys,
    pub peer_static: [32]u8,
    pub timestamp: Tai64n,
}

fn blake2sHash(data: []const u8) -> [32]u8 {
    let h = Blake2s256.init(.{});
    h.update(data);
    let out: [32]u8 = [0; 32];
    h.final(&out);
    return out;
}

fn blake2sHash2(a: []const u8, b: []const u8) -> [32]u8 {
    let h = Blake2s256.init(.{});
    h.update(a);
    h.update(b);
    let out: [32]u8 = [0; 32];
    h.final(&out);
    return out;
}

fn hmacBlake2s(key: []const u8, data: []const u8) -> [32]u8 {
    let k_block: [64]u8 = [0; 64];
    if (key.len > 64) {
        const hk = blake2sHash(key);
        @memcpy(k_block[0..32], &hk);
    } else {
        @memcpy(k_block[0..key.len], key);
    }

    let i_pad: [64]u8 = [0; 64];
    let o_pad: [64]u8 = [0; 64];
    let i: usize = 0;
    while (i < 64) {
        i_pad[i] = k_block[i] ^ 0x36;
        o_pad[i] = k_block[i] ^ 0x5c;
        i += 1;
    }

    let inner = Blake2s256.init(.{});
    inner.update(&i_pad);
    inner.update(data);
    let inner_hash: [32]u8 = [0; 32];
    inner.final(&inner_hash);

    let outer = Blake2s256.init(.{});
    outer.update(&o_pad);
    outer.update(&inner_hash);
    let out: [32]u8 = [0; 32];
    outer.final(&out);
    return out;
}

pub fn wgKdf1(key: *const [32]u8, input: []const u8) -> [32]u8 {
    const t0 = hmacBlake2s(key, input);
    const one: [1]u8 = [1];
    return hmacBlake2s(&t0, &one);
}

pub fn wgKdf2(key: *const [32]u8, input: []const u8) -> [2][32]u8 {
    const t0 = hmacBlake2s(key, input);
    const one: [1]u8 = [1];
    const t1 = hmacBlake2s(&t0, &one);

    let buf2: [33]u8 = [0; 33];
    @memcpy(buf2[0..32], &t1);
    buf2[32] = 2;
    const t2 = hmacBlake2s(&t0, &buf2);
    return [t1, t2];
}

pub fn wgKdf3(key: *const [32]u8, input: []const u8) -> [3][32]u8 {
    const t0 = hmacBlake2s(key, input);
    const one: [1]u8 = [1];
    const t1 = hmacBlake2s(&t0, &one);

    let buf2: [33]u8 = [0; 33];
    @memcpy(buf2[0..32], &t1);
    buf2[32] = 2;
    const t2 = hmacBlake2s(&t0, &buf2);

    let buf3: [33]u8 = [0; 33];
    @memcpy(buf3[0..32], &t2);
    buf3[32] = 3;
    const t3 = hmacBlake2s(&t0, &buf3);
    return [t1, t2, t3];
}

pub fn blake2sMac16(key: *const [32]u8, input: []const u8) -> [16]u8 {
    let h = Blake2s128.init(.{ .key = key });
    h.update(input);
    let out: [16]u8 = [0; 16];
    h.final(&out);
    return out;
}

fn makeNonce(counter: u64) -> [12]u8 {
    let nonce: [12]u8 = [0; 12];
    std.mem.writeInt(u64, nonce[4..12], counter, .little);
    return nonce;
}

fn aeadEncrypt(
    key: *const [32]u8,
    counter: u64,
    plaintext: []const u8,
    ad: []const u8,
    out_ct: []u8,
    out_tag: *[16]u8,
) {
    const nonce = makeNonce(counter);
    ChaCha20Poly1305.encrypt(out_ct, out_tag, plaintext, ad, nonce, key.*);
}

fn aeadDecrypt(
    key: *const [32]u8,
    counter: u64,
    ct: []const u8,
    tag: [16]u8,
    ad: []const u8,
    out_pt: []u8,
) -> !void {
    const nonce = makeNonce(counter);
    ChaCha20Poly1305.decrypt(out_pt, ct, tag, ad, nonce, key.*) catch return WgError.DecryptFailed;
}

pub fn createInitiation(
    local_static: WgKeyPair,
    remote_static_pub: [32]u8,
    psk: [32]u8,
    sender_index: u32,
    timestamp: Tai64n,
) -> !InitiationResult {
    let c = blake2sHash(CONSTRUCTION);
    let h = blake2sHash2(&c, IDENTIFIER);
    h = blake2sHash2(&h, &remote_static_pub);

    const eph = try WgKeyPair.generate();
    c = wgKdf1(&c, &eph.public_key);
    h = blake2sHash2(&h, &eph.public_key);

    const dh1 = X25519.scalarmult(eph.private_key, remote_static_pub) catch return WgError.InvalidKey;
    const k1 = wgKdf2(&c, &dh1);
    c = k1[0];
    const key1 = k1[1];

    let enc_static: [48]u8 = [0; 48];
    let tag1: [16]u8 = [0; 16];
    aeadEncrypt(&key1, 0, &local_static.public_key, &h, enc_static[0..32], &tag1);
    @memcpy(enc_static[32..48], &tag1);
    h = blake2sHash2(&h, &enc_static);

    const dh2 = X25519.scalarmult(local_static.private_key, remote_static_pub) catch return WgError.InvalidKey;
    const k2 = wgKdf2(&c, &dh2);
    c = k2[0];
    const key2 = k2[1];

    const ts_bytes = timestamp.encode();
    let enc_ts: [28]u8 = [0; 28];
    let tag2: [16]u8 = [0; 16];
    aeadEncrypt(&key2, 0, &ts_bytes, &h, enc_ts[0..12], &tag2);
    @memcpy(enc_ts[12..28], &tag2);
    h = blake2sHash2(&h, &enc_ts);

    let msg = HandshakeInitiation {
        .sender_index = sender_index,
        .ephemeral = eph.public_key,
        .encrypted_static = enc_static,
        .encrypted_timestamp = enc_ts,
        .mac1 = [0; 16],
        .mac2 = [0; 16],
    };

    const raw_prefix = msg.encode();
    const mac1_key = blake2sHash2(LABEL_MAC1, &remote_static_pub);
    msg.mac1 = blake2sMac16(&mac1_key, raw_prefix[0..116]);

    return InitiationResult {
        .msg = msg,
        .state = NoiseInitiatorState {
            .chaining_key = c,
            .hash = h,
            .ephemeral_private = eph.private_key,
            .sender_index = sender_index,
            .remote_static_public = remote_static_pub,
            .psk = psk,
        },
    };
}

pub fn consumeInitiationAndCreateResponse(
    responder_static: WgKeyPair,
    expected_peer_static: ([32]u8)?,
    psk: [32]u8,
    responder_index: u32,
    last_timestamp: *Tai64n?,
    msg: HandshakeInitiation,
) -> !ResponderResult {
    const raw_msg = msg.encode();
    const mac1_key = blake2sHash2(LABEL_MAC1, &responder_static.public_key);
    const expected_mac1 = blake2sMac16(&mac1_key, raw_msg[0..116]);
    if (!std.crypto.timing_safe.eql([16]u8, msg.mac1, expected_mac1)) {
        return WgError.InvalidMac1;
    }

    let c = blake2sHash(CONSTRUCTION);
    let h = blake2sHash2(&c, IDENTIFIER);
    h = blake2sHash2(&h, &responder_static.public_key);

    c = wgKdf1(&c, &msg.ephemeral);
    h = blake2sHash2(&h, &msg.ephemeral);

    const dh1 = X25519.scalarmult(responder_static.private_key, msg.ephemeral) catch return WgError.InvalidKey;
    const k1 = wgKdf2(&c, &dh1);
    c = k1[0];
    const key1 = k1[1];

    let peer_static: [32]u8 = [0; 32];
    let tag1: [16]u8 = [0; 16];
    @memcpy(&tag1, msg.encrypted_static[32..48]);
    try aeadDecrypt(&key1, 0, msg.encrypted_static[0..32], tag1, &h, &peer_static);
    h = blake2sHash2(&h, &msg.encrypted_static);

    if (exp := expected_peer_static) {
        if (!std.crypto.timing_safe.eql([32]u8, peer_static, exp)) {
            return WgError.InvalidKey;
        }
    }

    const dh2 = X25519.scalarmult(responder_static.private_key, peer_static) catch return WgError.InvalidKey;
    const k2 = wgKdf2(&c, &dh2);
    c = k2[0];
    const key2 = k2[1];

    let ts_bytes: [12]u8 = [0; 12];
    let tag2: [16]u8 = [0; 16];
    @memcpy(&tag2, msg.encrypted_timestamp[12..28]);
    try aeadDecrypt(&key2, 0, msg.encrypted_timestamp[0..12], tag2, &h, &ts_bytes);
    h = blake2sHash2(&h, &msg.encrypted_timestamp);

    const ts = try Tai64n.decode(&ts_bytes);
    if (prev_ts := last_timestamp.*) {
        if (ts.compare(prev_ts) <= 0) {
            return WgError.StaleTimestamp;
        }
    }
    last_timestamp.* = ts;

    // Build response
    const eph_r = try WgKeyPair.generate();
    c = wgKdf1(&c, &eph_r.public_key);
    h = blake2sHash2(&h, &eph_r.public_key);

    const dh_ee = X25519.scalarmult(eph_r.private_key, msg.ephemeral) catch return WgError.InvalidKey;
    c = wgKdf1(&c, &dh_ee);

    const dh_es = X25519.scalarmult(eph_r.private_key, peer_static) catch return WgError.InvalidKey;
    c = wgKdf1(&c, &dh_es);

    const k3 = wgKdf3(&c, &psk);
    c = k3[0];
    const tau = k3[1];
    const key3 = k3[2];
    h = blake2sHash2(&h, &tau);

    let enc_nothing: [16]u8 = [0; 16];
    let empty_buf: [1]u8 = [0; 1];
    aeadEncrypt(&key3, 0, "", &h, empty_buf[0..0], &enc_nothing);
    h = blake2sHash2(&h, &enc_nothing);

    let resp = HandshakeResponse {
        .sender_index = responder_index,
        .receiver_index = msg.sender_index,
        .ephemeral = eph_r.public_key,
        .encrypted_nothing = enc_nothing,
        .mac1 = [0; 16],
        .mac2 = [0; 16],
    };

    const resp_prefix = resp.encode();
    const resp_mac1_key = blake2sHash2(LABEL_MAC1, &peer_static);
    resp.mac1 = blake2sMac16(&resp_mac1_key, resp_prefix[0..60]);

    const session_k = wgKdf2(&c, "");

    return ResponderResult {
        .msg = resp,
        .keys = WgSessionKeys {
            .recv_key = session_k[0],
            .send_key = session_k[1],
            .local_index = responder_index,
            .remote_index = msg.sender_index,
        },
        .peer_static = peer_static,
        .timestamp = ts,
    };
}

pub fn consumeResponse(
    local_static: WgKeyPair,
    state: NoiseInitiatorState,
    msg: HandshakeResponse,
) -> !WgSessionKeys {
    if (msg.receiver_index != state.sender_index) {
        return WgError.InvalidState;
    }

    const raw_resp = msg.encode();
    const mac1_key = blake2sHash2(LABEL_MAC1, &local_static.public_key);
    const expected_mac1 = blake2sMac16(&mac1_key, raw_resp[0..60]);
    if (!std.crypto.timing_safe.eql([16]u8, msg.mac1, expected_mac1)) {
        return WgError.InvalidMac1;
    }

    let c = state.chaining_key;
    let h = state.hash;

    c = wgKdf1(&c, &msg.ephemeral);
    h = blake2sHash2(&h, &msg.ephemeral);

    const dh_ee = X25519.scalarmult(state.ephemeral_private, msg.ephemeral) catch return WgError.InvalidKey;
    c = wgKdf1(&c, &dh_ee);

    const dh_se = X25519.scalarmult(local_static.private_key, msg.ephemeral) catch return WgError.InvalidKey;
    c = wgKdf1(&c, &dh_se);

    const k3 = wgKdf3(&c, &state.psk);
    c = k3[0];
    const tau = k3[1];
    const key3 = k3[2];
    h = blake2sHash2(&h, &tau);

    let empty_buf: [1]u8 = [0; 1];
    try aeadDecrypt(&key3, 0, "", msg.encrypted_nothing, &h, empty_buf[0..0]);

    const session_k = wgKdf2(&c, "");

    return WgSessionKeys {
        .send_key = session_k[0],
        .recv_key = session_k[1],
        .local_index = state.sender_index,
        .remote_index = msg.sender_index,
    };
}

pub fn encryptTransport(
    keys: WgSessionKeys,
    counter: u64,
    plaintext: []const u8,
    out_buf: []u8,
) -> !usize {
    const total_len = TRANSPORT_HEADER_LEN + plaintext.len + AEAD_TAG_LEN;
    if (out_buf.len < total_len) {
        return WgError.InvalidLength;
    }

    out_buf[0] = MSG_TRANSPORT_TYPE;
    out_buf[1] = 0;
    out_buf[2] = 0;
    out_buf[3] = 0;
    std.mem.writeInt(u32, out_buf[4..8], keys.remote_index, .little);
    std.mem.writeInt(u64, out_buf[8..16], counter, .little);

    const ct_start = TRANSPORT_HEADER_LEN;
    const ct_end = ct_start + plaintext.len;
    let tag: [16]u8 = [0; 16];
    aeadEncrypt(&keys.send_key, counter, plaintext, "", out_buf[ct_start..ct_end], &tag);
    @memcpy(out_buf[ct_end..total_len], &tag);
    return total_len;
}

pub fn decryptTransport(
    keys: WgSessionKeys,
    window: *ReplayWindow,
    packet: []const u8,
    out_plaintext: []u8,
) -> !TransportPacket {
    if (packet.len < TRANSPORT_HEADER_LEN + AEAD_TAG_LEN) {
        return WgError.InvalidLength;
    }
    if (packet[0] != MSG_TRANSPORT_TYPE or packet[1] != 0 or packet[2] != 0 or packet[3] != 0) {
        return WgError.InvalidMessageType;
    }

    const recv_idx = std.mem.readInt(u32, packet[4..8], .little);
    if (recv_idx != keys.local_index) {
        return WgError.InvalidState;
    }

    const counter = std.mem.readInt(u64, packet[8..16], .little);
    const pt_len = packet.len - TRANSPORT_HEADER_LEN - AEAD_TAG_LEN;
    if (out_plaintext.len < pt_len) {
        return WgError.InvalidLength;
    }

    const ct_slice = packet[TRANSPORT_HEADER_LEN .. TRANSPORT_HEADER_LEN + pt_len];
    let tag: [16]u8 = [0; 16];
    @memcpy(&tag, packet[TRANSPORT_HEADER_LEN + pt_len .. packet.len]);
    try aeadDecrypt(&keys.recv_key, counter, ct_slice, tag, "", out_plaintext[0..pt_len]);

    try window.checkAndUpdate(counter);

    return TransportPacket {
        .receiver_index = recv_idx,
        .counter = counter,
        .plaintext_len = pt_len,
    };
}
