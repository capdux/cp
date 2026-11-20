// Source: cp/lib/webauthn/auth_data.cp (WebAuthn AuthenticatorData binary parser)
const std = @import("std");
import { CborReader, CoseKey, decodeCoseKey } from "cap:cose";
import {
    WebAuthnError,
    AuthFlags,
    AttestedCredData,
    AuthenticatorData,
} from "./types.cp";

pub fn parseAuthenticatorData(alloc: std.mem.Allocator, raw: []const u8): WebAuthnError!AuthenticatorData {
    if (raw.len < 37) {
        return WebAuthnError.InvalidLength;
    }

    let rp_id_hash: [32]u8 = undefined;
    @memcpy(&rp_id_hash, raw[0..32]);

    const flags = AuthFlags.fromByte(raw[32]);
    const sign_count = std.mem.readInt(u32, raw[33..37], .big);

    let pos: usize = 37;
    let acd_opt: AttestedCredData? = null;
    errdefer {
        if (*acd := acd_opt) {
            acd.deinit(alloc);
        }
    }

    if (flags.at) {
        if (pos + 18 > raw.len) {
            return WebAuthnError.InvalidLength;
        }
        let aaguid: [16]u8 = undefined;
        @memcpy(&aaguid, raw[pos .. pos + 16]);
        pos += 16;

        const cred_id_len: usize = @intCast(std.mem.readInt(u16, raw[pos..][0..2], .big));
        pos += 2;

        if (pos + cred_id_len > raw.len) {
            return WebAuthnError.InvalidLength;
        }
        const cred_id = alloc.dupe(u8, raw[pos .. pos + cred_id_len]) catch return WebAuthnError.OutOfMemory;
        errdefer alloc.free(cred_id);
        pos += cred_id_len;

        if (pos >= raw.len) {
            return WebAuthnError.InvalidCoseKey;
        }

        let cose_reader = CborReader.init(raw[pos..]);
        cose_reader.skipValue() catch return WebAuthnError.InvalidCbor;
        const cose_len = cose_reader.pos;

        let cose_key = decodeCoseKey(alloc, raw[pos .. pos + cose_len]) catch return WebAuthnError.InvalidCoseKey;
        errdefer cose_key.deinit(alloc);
        pos += cose_len;

        acd_opt = AttestedCredData{
            .aaguid = aaguid,
            .credential_id = cred_id,
            .credential_public_key = cose_key,
        };
    }

    let ext_opt: ([]u8)? = null;
    errdefer {
        if (ext := ext_opt) {
            alloc.free(ext);
        }
    }

    if (flags.ed) {
        if (pos >= raw.len) {
            return WebAuthnError.InvalidLength;
        }
        let ext_reader = CborReader.init(raw[pos..]);
        ext_reader.skipValue() catch return WebAuthnError.InvalidCbor;
        const ext_len = ext_reader.pos;
        const ext_bytes = alloc.dupe(u8, raw[pos .. pos + ext_len]) catch return WebAuthnError.OutOfMemory;
        ext_opt = ext_bytes;
        pos += ext_len;
    }

    if (pos != raw.len) {
        return WebAuthnError.TrailingBytes;
    }

    return AuthenticatorData{
        .rp_id_hash = rp_id_hash,
        .flags = flags,
        .sign_count = sign_count,
        .attested_credential_data = acd_opt,
        .extensions = ext_opt,
    };
}
