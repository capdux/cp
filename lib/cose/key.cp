// Source: cp/lib/cose/key.cp (RFC 9052 §7 & RFC 9053 COSE_Key encoding/decoding)
const std = @import("std");
import { CoseError, CoseKeyType, CoseCurve } from "./types.cp";
import { CborWriter, CborReader } from "./cbor.cp";

pub const CoseKey = struct {
    kty: CoseKeyType,
    alg: ?i32 = null,
    kid: ?[]const u8 = null,
    crv: ?CoseCurve = null,
    x: ?[]const u8 = null,
    y: ?[]const u8 = null,
    d: ?[]const u8 = null,
    k: ?[]const u8 = null,

    pub fn deinit(self: *CoseKey, alloc: std.mem.Allocator) void {
        if (self.kid) |v| {
            alloc.free(v);
            self.kid = null;
        }
        if (self.x) |v| {
            alloc.free(v);
            self.x = null;
        }
        if (self.y) |v| {
            alloc.free(v);
            self.y = null;
        }
        if (self.d) |v| {
            alloc.free(v);
            self.d = null;
        }
        if (self.k) |v| {
            alloc.free(v);
            self.k = null;
        }
    }
};

fn validateKey(key: CoseKey): CoseError!void {
    switch (key.kty) {
        .OKP => {
            const crv = key.crv orelse return CoseError.MissingField;
            if (crv != .Ed25519 and crv != .X25519 and crv != .Ed448 and crv != .X448) {
                return CoseError.InvalidCurve;
            }
            const x = key.x orelse return CoseError.MissingField;
            if (crv == .Ed25519 or crv == .X25519) {
                if (x.len != 32) return CoseError.InvalidKeyLength;
                if (d := key.d) {
                    if (d.len != 32) return CoseError.InvalidKeyLength;
                }
            }
        },
        .EC2 => {
            const crv = key.crv orelse return CoseError.MissingField;
            if (crv != .P256 and crv != .P384 and crv != .P521) {
                return CoseError.InvalidCurve;
            }
            const x = key.x orelse return CoseError.MissingField;
            const y = key.y orelse return CoseError.MissingField;
            if (crv == .P256) {
                if (x.len != 32 or y.len != 32) return CoseError.InvalidKeyLength;
                if (d := key.d) {
                    if (d.len != 32) return CoseError.InvalidKeyLength;
                }
            }
        },
        .Symmetric => {
            const k = key.k orelse return CoseError.MissingField;
            if (k.len == 0) return CoseError.InvalidKeyLength;
        },
        .RSA => {},
    }
}

pub fn encodeCoseKey(alloc: std.mem.Allocator, key: CoseKey): CoseError![]u8 {
    try validateKey(key);

    let count: usize = 1; // kty (1)
    if (key.kid != null) count += 1; // kid (2)
    if (key.alg != null) count += 1; // alg (3)
    if (key.crv != null) count += 1; // crv (-1)
    if (key.x != null) count += 1; // x (-2)
    if (key.y != null) count += 1; // y (-3)
    if (key.d != null or (key.kty == .Symmetric and key.k != null)) count += 1; // d (-4) or k (-1)

    // Note: For Symmetric keys, k is label -1 (RFC 9052 §7.1 / Table 6: kty=4, k=-1)
    let w = CborWriter.init();
    defer w.deinit(alloc);

    try w.writeMapHeader(alloc, count);

    // Positive keys first in deterministic CBOR: 1, 2, 3
    try w.writeInt(alloc, 1);
    try w.writeInt(alloc, key.kty.toInt());

    if (kid := key.kid) {
        try w.writeInt(alloc, 2);
        try w.writeBstr(alloc, kid);
    }

    if (alg := key.alg) {
        try w.writeInt(alloc, 3);
        try w.writeInt(alloc, alg);
    }

    // Negative keys next: -1, -2, -3, -4
    if (key.kty == .Symmetric) {
        if (k := key.k) {
            try w.writeInt(alloc, -1);
            try w.writeBstr(alloc, k);
        }
    } else {
        if (crv := key.crv) {
            try w.writeInt(alloc, -1);
            try w.writeInt(alloc, crv.toInt());
        }
        if (x := key.x) {
            try w.writeInt(alloc, -2);
            try w.writeBstr(alloc, x);
        }
        if (y := key.y) {
            try w.writeInt(alloc, -3);
            try w.writeBstr(alloc, y);
        }
        if (d := key.d) {
            try w.writeInt(alloc, -4);
            try w.writeBstr(alloc, d);
        }
    }

    return try w.toOwnedSlice(alloc);
}

pub fn decodeCoseKey(alloc: std.mem.Allocator, cbor_bytes: []const u8): CoseError!CoseKey {
    let r = CborReader.init(cbor_bytes);
    const map_len = try r.readMapHeader();

    let kty_opt: CoseKeyType? = null;
    let alg_opt: i32? = null;
    let kid_opt: ([]const u8)? = null;
    let crv_opt: CoseCurve? = null;
    let x_opt: ([]const u8)? = null;
    let y_opt: ([]const u8)? = null;
    let d_opt: ([]const u8)? = null;
    let neg1_bstr_opt: ([]const u8)? = null;

    errdefer {
        if (v := kid_opt) alloc.free(v);
        if (v := x_opt) alloc.free(v);
        if (v := y_opt) alloc.free(v);
        if (v := d_opt) alloc.free(v);
        if (v := neg1_bstr_opt) alloc.free(v);
    }

    let i: usize = 0;
    while (i < map_len) : (i += 1) {
        const label = try r.readInt();
        switch (label) {
            1 => {
                const v = try r.readInt();
                kty_opt = try CoseKeyType.fromInt(v);
            },
            2 => {
                const b = try r.readBstr();
                kid_opt = alloc.dupe(u8, b) catch return CoseError.OutOfMemory;
            },
            3 => {
                const v = try r.readInt();
                if (v < std.math.minInt(i32) or v > std.math.maxInt(i32)) return CoseError.InvalidAlgorithm;
                alg_opt = @as(i32, @intCast(v));
            },
            -1 => {
                const major = try r.peekMajor();
                if (major == 2) {
                    const b = try r.readBstr();
                    neg1_bstr_opt = alloc.dupe(u8, b) catch return CoseError.OutOfMemory;
                } else {
                    const v = try r.readInt();
                    crv_opt = try CoseCurve.fromInt(v);
                }
            },
            -2 => {
                const b = try r.readBstr();
                x_opt = alloc.dupe(u8, b) catch return CoseError.OutOfMemory;
            },
            -3 => {
                const b = try r.readBstr();
                y_opt = alloc.dupe(u8, b) catch return CoseError.OutOfMemory;
            },
            -4 => {
                const b = try r.readBstr();
                d_opt = alloc.dupe(u8, b) catch return CoseError.OutOfMemory;
            },
            else => {
                try r.skipValue();
            },
        }
    }

    const kty = kty_opt orelse return CoseError.MissingField;
    let k_opt: ([]const u8)? = null;
    if (kty == .Symmetric) {
        k_opt = neg1_bstr_opt;
        neg1_bstr_opt = null;
    } else {
        if (v := neg1_bstr_opt) {
            alloc.free(v);
            neg1_bstr_opt = null;
            return CoseError.UnexpectedType;
        }
    }

    const key = CoseKey{
        .kty = kty,
        .alg = alg_opt,
        .kid = kid_opt,
        .crv = crv_opt,
        .x = x_opt,
        .y = y_opt,
        .d = d_opt,
        .k = k_opt,
    };

    try validateKey(key);
    return key;
}
