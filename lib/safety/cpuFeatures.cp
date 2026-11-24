// Source: Absorbed from /data/dev/bun/src/crash_handler/CPUFeatures.cp | Synced: 2026-11-23T21:08:14Z
// lib/safety/cpu_features.cp
// Hardware CPU Feature Detection (x86_64 CPUID + Target Features / aarch64 Target Features)

const std = @import("std");

zig {
    const CpuidRegs = struct {
        eax: u32,
        ebx: u32,
        ecx: u32,
        edx: u32,
    };

    fn cpuidQuery(leaf: u32, subleaf: u32) CpuidRegs {
        if (comptime @import("builtin").cpu.arch != .x86_64) {
            return .{ .eax = 0, .ebx = 0, .ecx = 0, .edx = 0 };
        }
        var eax: u32 = undefined;
        var ebx: u32 = undefined;
        var ecx: u32 = undefined;
        var edx: u32 = undefined;
        asm volatile (
            \\xchgq %%rbx, %[ebx_out]
            \\cpuid
            \\xchgq %%rbx, %[ebx_out]
            : [eax_out] "={eax}" (eax),
              [ebx_out] "=&r" (ebx),
              [ecx_out] "={ecx}" (ecx),
              [edx_out] "={edx}" (edx),
            : [leaf_in] "{eax}" (leaf),
              [subleaf_in] "{ecx}" (subleaf),
            : .{ .cc = true }
        );
        return .{ .eax = eax, .ebx = ebx, .ecx = ecx, .edx = edx };
    }

    fn xgetbv0() u64 {
        if (comptime @import("builtin").cpu.arch != .x86_64) {
            return 0;
        }
        var lo: u32 = undefined;
        var hi: u32 = undefined;
        asm volatile ("xgetbv"
            : [lo_out] "={eax}" (lo),
              [hi_out] "={edx}" (hi),
            : [xcr] "{ecx}" (@as(u32, 0)),
        );
        return (@as(u64, hi) << 32) | @as(u64, lo);
    }

    fn detectX86Runtime(out: *CpuFeatures) void {
        const leaf0 = cpuidQuery(0, 0);
        const max_leaf = leaf0.eax;
        if (max_leaf >= 1) {
            const leaf1 = cpuidQuery(1, 0);
            if ((leaf1.edx & (@as(u32, 1) << 26)) != 0) out.sse2 = true;
            if ((leaf1.ecx & (@as(u32, 1) << 20)) != 0) out.sse4_2 = true;
            if ((leaf1.ecx & (@as(u32, 1) << 23)) != 0) out.popcnt = true;
            if ((leaf1.ecx & (@as(u32, 1) << 25)) != 0) out.aes = true;
            if ((leaf1.ecx & (@as(u32, 1) << 12)) != 0) out.fma = true;

            const osxsave = (leaf1.ecx & (@as(u32, 1) << 27)) != 0;
            const avx_bit = (leaf1.ecx & (@as(u32, 1) << 28)) != 0;
            var os_avx = false;
            var os_avx512 = false;
            if (osxsave) {
                const xcr0 = xgetbv0();
                os_avx = (xcr0 & 0x6) == 0x6;
                os_avx512 = (xcr0 & 0xE6) == 0xE6;
            }
            if (avx_bit and os_avx) {
                out.avx = true;
            }
            if (max_leaf >= 7) {
                const leaf7 = cpuidQuery(7, 0);
                if ((leaf7.ebx & (@as(u32, 1) << 3)) != 0) out.bmi1 = true;
                if ((leaf7.ebx & (@as(u32, 1) << 8)) != 0) out.bmi2 = true;
                if ((leaf7.ebx & (@as(u32, 1) << 29)) != 0) out.sha = true;
                if (os_avx and (leaf7.ebx & (@as(u32, 1) << 5)) != 0) out.avx2 = true;
                if (os_avx512 and (leaf7.ebx & (@as(u32, 1) << 16)) != 0) out.avx512f = true;
            }
        }
    }
}

pub struct CpuFeatures {
    // x86_64 SIMD & ISA extensions
    sse2: bool = false,
    sse4_2: bool = false,
    avx: bool = false,
    avx2: bool = false,
    avx512f: bool = false,
    aes: bool = false,
    sha: bool = false,
    popcnt: bool = false,
    bmi1: bool = false,
    bmi2: bool = false,
    fma: bool = false,

    // aarch64 extensions
    neon: bool = false,
    sha2: bool = false,
    crc32: bool = false,

    arch_name: []const u8 = @tagName(@import("builtin").cpu.arch),

    /// Detects CPU ISA capabilities via compile-time target features and runtime hardware CPUID (on x86_64).
    pub static fn detect(): CpuFeatures {
        let f = CpuFeatures {
            arch_name: @tagName(@import("builtin").cpu.arch),
        };

        if (comptime @import("builtin").cpu.arch == .x86_64) {
            if (std.Target.x86.featureSetHas(@import("builtin").cpu.features, .sse2)) f.sse2 = true;
            if (std.Target.x86.featureSetHas(@import("builtin").cpu.features, .sse4_2)) f.sse4_2 = true;
            if (std.Target.x86.featureSetHas(@import("builtin").cpu.features, .avx)) f.avx = true;
            if (std.Target.x86.featureSetHas(@import("builtin").cpu.features, .avx2)) f.avx2 = true;
            if (std.Target.x86.featureSetHas(@import("builtin").cpu.features, .avx512f)) f.avx512f = true;
            if (std.Target.x86.featureSetHas(@import("builtin").cpu.features, .aes)) f.aes = true;
            if (std.Target.x86.featureSetHas(@import("builtin").cpu.features, .sha)) f.sha = true;
            if (std.Target.x86.featureSetHas(@import("builtin").cpu.features, .popcnt)) f.popcnt = true;
            if (std.Target.x86.featureSetHas(@import("builtin").cpu.features, .bmi)) f.bmi1 = true;
            if (std.Target.x86.featureSetHas(@import("builtin").cpu.features, .bmi2)) f.bmi2 = true;
            if (std.Target.x86.featureSetHas(@import("builtin").cpu.features, .fma)) f.fma = true;
            detectX86Runtime(&f);
        } else if (comptime @import("builtin").cpu.arch == .aarch64) {
            f.neon = std.Target.aarch64.featureSetHas(@import("builtin").cpu.features, .neon);
            f.aes = std.Target.aarch64.featureSetHas(@import("builtin").cpu.features, .aes);
            f.sha2 = std.Target.aarch64.featureSetHas(@import("builtin").cpu.features, .sha2);
            f.crc32 = std.Target.aarch64.featureSetHas(@import("builtin").cpu.features, .crc);
        }

        return f;
    }

    pub fn hasSimd128(): bool {
        return _.sse2 || _.sse4_2 || _.neon;
    }

    pub fn hasSimd256(): bool {
        return _.avx || _.avx2;
    }

    pub fn hasHardwareAes(): bool {
        return _.aes;
    }

    /// Formats CPU architecture and detected feature flags into `buf` without heap allocation.
    pub fn formatSummary(buf: []u8): []const u8 {
        if (buf.len == 0) return "";
        let pos: usize = 0;

        pos = appendBytes(buf, pos, _.arch_name);
        pos = appendBytes(buf, pos, " [");

        let count: usize = 0;
        if (_.sse2) { pos = appendFlag(buf, pos, "sse2", &count); }
        if (_.sse4_2) { pos = appendFlag(buf, pos, "sse4_2", &count); }
        if (_.avx) { pos = appendFlag(buf, pos, "avx", &count); }
        if (_.avx2) { pos = appendFlag(buf, pos, "avx2", &count); }
        if (_.avx512f) { pos = appendFlag(buf, pos, "avx512f", &count); }
        if (_.aes) { pos = appendFlag(buf, pos, "aes", &count); }
        if (_.sha) { pos = appendFlag(buf, pos, "sha", &count); }
        if (_.popcnt) { pos = appendFlag(buf, pos, "popcnt", &count); }
        if (_.bmi1) { pos = appendFlag(buf, pos, "bmi1", &count); }
        if (_.bmi2) { pos = appendFlag(buf, pos, "bmi2", &count); }
        if (_.fma) { pos = appendFlag(buf, pos, "fma", &count); }
        if (_.neon) { pos = appendFlag(buf, pos, "neon", &count); }
        if (_.sha2) { pos = appendFlag(buf, pos, "sha2", &count); }
        if (_.crc32) { pos = appendFlag(buf, pos, "crc32", &count); }

        if (count == 0) {
            pos = appendBytes(buf, pos, "none");
        }
        pos = appendBytes(buf, pos, "]");
        return buf[0..pos];
    }
}

fn appendBytes(buf: []u8, pos: usize, s: []const u8): usize {
    if (pos >= buf.len) return buf.len;
    const avail = buf.len - pos;
    const n = if (s.len < avail) s.len else avail;
    @memcpy(buf[pos .. pos + n], s[0..n]);
    return pos + n;
}

fn appendFlag(buf: []u8, pos: usize, name: []const u8, count: *usize): usize {
    let p = pos;
    if (count.* > 0) {
        p = appendBytes(buf, p, " ");
    }
    p = appendBytes(buf, p, name);
    count.* += 1;
    return p;
}
