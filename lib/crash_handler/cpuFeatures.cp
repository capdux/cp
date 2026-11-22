// Source: Absorbed from /data/dev/bun/src/crash_handler/CPUFeatures.cp | Synced: 2026-11-22T15:23:33Z
// Zero-allocation cross-architecture CPU feature flags (x86_64 & aarch64) for crash diagnostics.

/// Recommended caller buffer capacity for formatting space-separated CPU feature lists.
pub const CPU_FEATURES_FORMAT_BUF_CAP: usize = 64;

/// x86_64 feature bit positions (bit-exact parity with Bun `@packed struct X86Flags`, bit 0 = none).
pub const X86_BIT_SSE42: u8 = 1 << 1;
pub const X86_BIT_POPCNT: u8 = 1 << 2;
pub const X86_BIT_AVX: u8 = 1 << 3;
pub const X86_BIT_AVX2: u8 = 1 << 4;
pub const X86_BIT_AVX512: u8 = 1 << 5;

/// aarch64 feature bit positions (bit-exact parity with Bun `@packed struct Aarch64Flags`, bit 0 = none).
pub const AARCH64_BIT_NEON: u8 = 1 << 1;
pub const AARCH64_BIT_FP: u8 = 1 << 2;
pub const AARCH64_BIT_AES: u8 = 1 << 3;
pub const AARCH64_BIT_CRC32: u8 = 1 << 4;
pub const AARCH64_BIT_ATOMICS: u8 = 1 << 5;
pub const AARCH64_BIT_SVE: u8 = 1 << 6;

/// Error set for CPU feature formatting and parsing.
pub err CpuFeaturesError {
    BufferTooSmall,
    UnknownFeatureToken,
}

/// Target CPU architecture family supported by crash diagnostics.
pub enum CpuArchKind: u8 {
    X86_64 = 0,
    Aarch64 = 1,
}

fn tokenEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    for (i of 0..a.len) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn appendFeature(out: []u8, pos: *usize, is_first: *bool, name: []const u8): usize ! CpuFeaturesError {
    const need_space: usize = if (is_first.*) 0 else 1;
    if (pos.* + need_space + name.len > out.len) {
        return err.BufferTooSmall;
    }
    if (!is_first.*) {
        out[pos.*] = ' ';
        pos.* += 1;
    }
    for (i of 0..name.len) {
        out[pos.* + i] = name[i];
    }
    pos.* += name.len;
    is_first.* = false;
    return pos.*;
}

/// x86_64 CPU instruction-set flags (SSE4.2 / POPCNT / AVX / AVX2 / AVX-512).
pub struct X86CpuFlags {
    sse42: bool = false,
    popcnt: bool = false,
    avx: bool = false,
    avx2: bool = false,
    avx512: bool = false,

    /// Decode from Bun's 8-bit `X86Flags` bitmask (ignoring bit 0 `none` and bits 6..7 padding).
    static fn fromMask(mask: u8): X86CpuFlags {
        return X86CpuFlags{
            sse42: (mask & X86_BIT_SSE42) != 0,
            popcnt: (mask & X86_BIT_POPCNT) != 0,
            avx: (mask & X86_BIT_AVX) != 0,
            avx2: (mask & X86_BIT_AVX2) != 0,
            avx512: (mask & X86_BIT_AVX512) != 0,
        };
    }

    /// Encode to Bun's 8-bit `X86Flags` bitmask.
    fn toMask(): u8 {
        let m: u8 = 0;
        if (_.sse42) m = m | X86_BIT_SSE42;
        if (_.popcnt) m = m | X86_BIT_POPCNT;
        if (_.avx) m = m | X86_BIT_AVX;
        if (_.avx2) m = m | X86_BIT_AVX2;
        if (_.avx512) m = m | X86_BIT_AVX512;
        return m;
    }

    fn isEmpty(): bool {
        return _.toMask() == 0;
    }

    /// True if any AVX extension (`avx`, `avx2`, or `avx512`) is present.
    fn hasAnyAvx(): bool {
        return _.avx || _.avx2 || _.avx512;
    }

    /// True if CPU meets x64-baseline (Nehalem) requirements (SSE4.2 + POPCNT).
    fn meetsBaseline(): bool {
        return _.sse42 && _.popcnt;
    }

    /// True if CPU meets standard x64 (Haswell) requirements (SSE4.2 + POPCNT + AVX + AVX2).
    fn meetsHaswell(): bool {
        return _.sse42 && _.popcnt && _.avx && _.avx2;
    }

    /// Format active flags as space-separated names into `out` (e.g. `"sse42 popcnt avx avx2"`).
    fn formatToBuf(out: []u8): usize ! CpuFeaturesError {
        let pos: usize = 0;
        let is_first = true;
        if (_.sse42) { * = appendFeature(out, *pos, *is_first, "sse42")?; }
        if (_.popcnt) { * = appendFeature(out, *pos, *is_first, "popcnt")?; }
        if (_.avx) { * = appendFeature(out, *pos, *is_first, "avx")?; }
        if (_.avx2) { * = appendFeature(out, *pos, *is_first, "avx2")?; }
        if (_.avx512) { * = appendFeature(out, *pos, *is_first, "avx512")?; }
        return pos;
    }

    /// Parse space-separated x86_64 feature tokens (strictly rejects unknown tokens).
    static fn parseFromStr(s: []const u8): X86CpuFlags ! CpuFeaturesError {
        let res = X86CpuFlags{};
        let i: usize = 0;
        while (i < s.len) {
            while (i < s.len && (s[i] == ' ' || s[i] == '\t')) {
                i += 1;
            }
            if (i >= s.len) break;
            const start = i;
            while (i < s.len && s[i] != ' ' && s[i] != '\t') {
                i += 1;
            }
            const tok = s[start..i];
            if (tokenEql(tok, "sse42")) {
                res.sse42 = true;
            } else if (tokenEql(tok, "popcnt")) {
                res.popcnt = true;
            } else if (tokenEql(tok, "avx")) {
                res.avx = true;
            } else if (tokenEql(tok, "avx2")) {
                res.avx2 = true;
            } else if (tokenEql(tok, "avx512")) {
                res.avx512 = true;
            } else {
                return err.UnknownFeatureToken;
            }
        }
        return res;
    }

    fn eql(other: X86CpuFlags): bool {
        return _.toMask() == other.toMask();
    }
}

/// aarch64 CPU instruction-set flags (NEON / FP / AES / CRC32 / Atomics / SVE).
pub struct Aarch64CpuFlags {
    neon: bool = false,
    fp: bool = false,
    aes: bool = false,
    crc32: bool = false,
    atomics: bool = false,
    sve: bool = false,

    /// Decode from Bun's 8-bit `Aarch64Flags` bitmask (ignoring bit 0 `none` and bit 7 padding).
    static fn fromMask(mask: u8): Aarch64CpuFlags {
        return Aarch64CpuFlags{
            neon: (mask & AARCH64_BIT_NEON) != 0,
            fp: (mask & AARCH64_BIT_FP) != 0,
            aes: (mask & AARCH64_BIT_AES) != 0,
            crc32: (mask & AARCH64_BIT_CRC32) != 0,
            atomics: (mask & AARCH64_BIT_ATOMICS) != 0,
            sve: (mask & AARCH64_BIT_SVE) != 0,
        };
    }

    /// Encode to Bun's 8-bit `Aarch64Flags` bitmask.
    fn toMask(): u8 {
        let m: u8 = 0;
        if (_.neon) m = m | AARCH64_BIT_NEON;
        if (_.fp) m = m | AARCH64_BIT_FP;
        if (_.aes) m = m | AARCH64_BIT_AES;
        if (_.crc32) m = m | AARCH64_BIT_CRC32;
        if (_.atomics) m = m | AARCH64_BIT_ATOMICS;
        if (_.sve) m = m | AARCH64_BIT_SVE;
        return m;
    }

    fn isEmpty(): bool {
        return _.toMask() == 0;
    }

    fn hasCryptoAndCrc(): bool {
        return _.aes && _.crc32;
    }

    /// Format active flags as space-separated names into `out` (e.g. `"neon fp aes crc32 atomics"`).
    fn formatToBuf(out: []u8): usize ! CpuFeaturesError {
        let pos: usize = 0;
        let is_first = true;
        if (_.neon) { * = appendFeature(out, *pos, *is_first, "neon")?; }
        if (_.fp) { * = appendFeature(out, *pos, *is_first, "fp")?; }
        if (_.aes) { * = appendFeature(out, *pos, *is_first, "aes")?; }
        if (_.crc32) { * = appendFeature(out, *pos, *is_first, "crc32")?; }
        if (_.atomics) { * = appendFeature(out, *pos, *is_first, "atomics")?; }
        if (_.sve) { * = appendFeature(out, *pos, *is_first, "sve")?; }
        return pos;
    }

    /// Parse space-separated aarch64 feature tokens (strictly rejects unknown tokens).
    static fn parseFromStr(s: []const u8): Aarch64CpuFlags ! CpuFeaturesError {
        let res = Aarch64CpuFlags{};
        let i: usize = 0;
        while (i < s.len) {
            while (i < s.len && (s[i] == ' ' || s[i] == '\t')) {
                i += 1;
            }
            if (i >= s.len) break;
            const start = i;
            while (i < s.len && s[i] != ' ' && s[i] != '\t') {
                i += 1;
            }
            const tok = s[start..i];
            if (tokenEql(tok, "neon")) {
                res.neon = true;
            } else if (tokenEql(tok, "fp")) {
                res.fp = true;
            } else if (tokenEql(tok, "aes")) {
                res.aes = true;
            } else if (tokenEql(tok, "crc32")) {
                res.crc32 = true;
            } else if (tokenEql(tok, "atomics")) {
                res.atomics = true;
            } else if (tokenEql(tok, "sve")) {
                res.sve = true;
            } else {
                return err.UnknownFeatureToken;
            }
        }
        return res;
    }

    fn eql(other: Aarch64CpuFlags): bool {
        return _.toMask() == other.toMask();
    }
}

/// Architecture-tagged CPU feature descriptor for crash reports and diagnostics.
pub struct CpuFeatures {
    arch: CpuArchKind = .X86_64,
    x86: X86CpuFlags = X86CpuFlags{},
    aarch64: Aarch64CpuFlags = Aarch64CpuFlags{},

    static fn fromX86(flags: X86CpuFlags): CpuFeatures {
        return CpuFeatures{
            arch: .X86_64,
            x86: flags,
        };
    }

    static fn fromAarch64(flags: Aarch64CpuFlags): CpuFeatures {
        return CpuFeatures{
            arch: .Aarch64,
            aarch64: flags,
        };
    }

    static fn fromRawMask(arch: CpuArchKind, mask: u8): CpuFeatures {
        if (arch == .X86_64) {
            return CpuFeatures.fromX86(X86CpuFlags.fromMask(mask));
        }
        return CpuFeatures.fromAarch64(Aarch64CpuFlags.fromMask(mask));
    }

    fn toRawMask(): u8 {
        if (_.arch == .X86_64) return _.x86.toMask();
        return _.aarch64.toMask();
    }

    fn isEmpty(): bool {
        if (_.arch == .X86_64) return _.x86.isEmpty();
        return _.aarch64.isEmpty();
    }

    fn hasAnyAvx(): bool {
        if (_.arch == .X86_64) return _.x86.hasAnyAvx();
        return false;
    }

    fn formatToBuf(out: []u8): usize ! CpuFeaturesError {
        if (_.arch == .X86_64) {
            return _.x86.formatToBuf(out);
        }
        return _.aarch64.formatToBuf(out);
    }
}
