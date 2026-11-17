// Source: Zig Standard Library: std.hash
// ============================================================================
// lib/hash/index.cp — High-Speed Non-Cryptographic Hashing Matrix (cap:hash)
// Absorbed from Zig Standard Library: std.hash (XxHash3, Murmur, CityHash, CRC32, Adler32, SipHash)
// ============================================================================

const std = @import("std");

/// Unified High-Performance Non-Cryptographic Hash Algorithms
pub struct Hash {
    /// State-of-the-art SIMD-accelerated 64-bit hash (30~50 GB/s on modern CPUs).
    pub static fn xxhash3(buf: []const u8, seed: u64 = 0): u64 {
        let res: u64 = 0;
        zig {
            res = @import("std").hash.XxHash3.hash(seed, buf);
        }
        return res;
    }

    /// 128-bit ultra-low collision hash based on dual-seeded XxHash3 passes.
    pub static fn xxhash128(buf: []const u8, seed: u64 = 0): u128 {
        let res: u128 = 0;
        zig {
            const h1: u128 = @import("std").hash.XxHash3.hash(seed, buf);
            const h2: u128 = @import("std").hash.XxHash3.hash(seed ^ 0x9e3779b97f4a7c15, buf);
            res = (h2 << 64) | h1;
        }
        return res;
    }

    /// MurmurHash3 32-bit algorithm for general-purpose key hashing and Bloom filters.
    pub static fn murmur3(buf: []const u8, seed: u32 = 0): u32 {
        let res: u32 = 0;
        zig {
            res = @import("std").hash.Murmur3_32.hashWithSeed(buf, seed);
        }
        return res;
    }

    /// Google's CityHash64 optimized for short and medium string hashing.
    pub static fn cityhash64(buf: []const u8): u64 {
        let res: u64 = 0;
        zig {
            res = @import("std").hash.CityHash64.hash(buf);
        }
        return res;
    }

    /// IEEE 802.3 standard CRC32 checksum with direct hardware instruction acceleration.
    pub static fn crc32(buf: []const u8): u32 {
        let res: u32 = 0;
        zig {
            res = @import("std").hash.Crc32.hash(buf);
        }
        return res;
    }

    /// RFC 1950 Adler32 checksum algorithm used in Zlib compression.
    pub static fn adler32(buf: []const u8): u32 {
        let res: u32 = 0;
        zig {
            res = @import("std").hash.Adler32.hash(buf);
        }
        return res;
    }

    /// SipHash-2-4 64-bit PRF hash for protecting hash tables against Hash-DoS attacks.
    pub static fn siphash(buf: []const u8, key: [16]u8): u64 {
        let res: u64 = 0;
        zig {
            res = @import("std").hash.SipHash64(2, 4).toInt(buf, &key);
        }
        return res;
    }

    /// SipHash-2-4 128-bit PRF hash for secure short-message authentication.
    pub static fn siphash128(buf: []const u8, key: [16]u8): u128 {
        let res: u128 = 0;
        zig {
            res = @import("std").hash.SipHash128(2, 4).toInt(buf, &key);
        }
        return res;
    }

    /// Fowler-Noll-Vo 64-bit FNV-1a hash algorithm.
    pub static fn fnv1a(buf: []const u8): u64 {
        let res: u64 = 0;
        zig {
            res = @import("std").hash.Fnv1a_64.hash(buf);
        }
        return res;
    }

    /// Zero-allocation integer avalanche permutation for 64-bit integer keys.
    pub static fn hashInt(val: u64): u64 {
        let res: u64 = 0;
        zig {
            res = @import("std").hash.int(val);
        }
        return res;
    }
}

/// Incremental streaming handle for SIMD-accelerated XxHash3.
pub struct XxHash3Stream {
    inner: std.hash.XxHash3 = undefined;

    pub static fn init(seed: u64 = 0): XxHash3Stream {
        let stream = XxHash3Stream {};
        zig {
            stream.inner = @import("std").hash.XxHash3.init(seed);
        }
        return stream;
    }

    pub fn update(*_, chunk: []const u8) {
        zig {
            self.inner.update(chunk);
        }
    }

    pub fn digest(*_): u64 {
        let res: u64 = 0;
        zig {
            res = self.inner.final();
        }
        return res;
    }
}
