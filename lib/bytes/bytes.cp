// @cap/bytes — Pure CP, zero-dependency, zero-heap-allocation byte size parser & formatter
// Clean-room MIT implementation inspired by npm `bytes` and Rust `bytesize` / `humansize`.
// Features:
//   - Zero heap allocation, zero `unsafe`, exact integer arithmetic (u64 / u128)
//   - Binary IEC (KiB/MiB/GiB/TiB/PiB, 1024-based) and Decimal SI (KB/MB/GB/TB/PB, 1000-based)
//   - Case-insensitive parsing with optional whitespace, decimal fractions, and strict u64 overflow guards
//   - Configurable formatting (mode, decimals 0..4, space toggle, automatic trailing zero trimming, carry promotion)

pub err BytesError {
    EmptyInput,
    InvalidFormat,
    UnknownUnit,
    Overflow,
    BufferTooSmall,
}

// Binary IEC constants (powers of 1024)
pub const KIB: u64 = 1024;
pub const MIB: u64 = 1048576;
pub const GIB: u64 = 1073741824;
pub const TIB: u64 = 1099511627776;
pub const PIB: u64 = 1125899906842624;

// Decimal SI constants (powers of 1000)
pub const KB: u64 = 1000;
pub const MB: u64 = 1000000;
pub const GB: u64 = 1000000000;
pub const TB: u64 = 1000000000000;
pub const PB: u64 = 1000000000000000;

const U64_MAX: u64 = (u64(0xFFFFFFFF) << 32) | u64(0xFFFFFFFF);
const MAX_FRAC_DIGITS: usize = 18;
const MAX_DECIMALS: u8 = 4;

pub enum ByteUnitMode {
    BinaryIec,
    DecimalSi,
}

pub struct FormatOptions {
    mode: ByteUnitMode = ByteUnitMode.BinaryIec;
    decimals: u8 = 2;
    space: bool = true;
}

fn isAsciiSpace(b: u8): bool {
    return b == ' ' || b == '\t' || b == '\r' || b == '\n';
}

fn isAsciiDigit(b: u8): bool {
    return b >= '0' && b <= '9';
}

fn toAsciiLower(b: u8): u8 {
    if (b >= 'A' && b <= 'Z') {
        return b + 32;
    }
    return b;
}

fn sliceEqlLower(a: []const u8, b: string): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (toAsciiLower(a[i]) != b[i]) {
            return false;
        }
    }
    return true;
}

fn resolveUnitMultiplier(unit: []const u8, si_default: bool): u64 ! BytesError {
    if (unit.len == 0) {
        return 1;
    }
    if (unit.len > 5) {
        return BytesError.UnknownUnit;
    }

    // Bytes
    if (sliceEqlLower(unit, "b") || sliceEqlLower(unit, "byte") || sliceEqlLower(unit, "bytes")) {
        return 1;
    }

    // Explicit IEC binary units (always 1024-based)
    if (sliceEqlLower(unit, "kib") || sliceEqlLower(unit, "ki")) {
        return KIB;
    }
    if (sliceEqlLower(unit, "mib") || sliceEqlLower(unit, "mi")) {
        return MIB;
    }
    if (sliceEqlLower(unit, "gib") || sliceEqlLower(unit, "gi")) {
        return GIB;
    }
    if (sliceEqlLower(unit, "tib") || sliceEqlLower(unit, "ti")) {
        return TIB;
    }
    if (sliceEqlLower(unit, "pib") || sliceEqlLower(unit, "pi")) {
        return PIB;
    }

    // Standard units: 1000-based in SI mode, 1024-based in Binary mode (npm `bytes` compatible)
    if (sliceEqlLower(unit, "kb") || sliceEqlLower(unit, "k")) {
        if (si_default) { return KB; }
        return KIB;
    }
    if (sliceEqlLower(unit, "mb") || sliceEqlLower(unit, "m")) {
        if (si_default) { return MB; }
        return MIB;
    }
    if (sliceEqlLower(unit, "gb") || sliceEqlLower(unit, "g")) {
        if (si_default) { return GB; }
        return GIB;
    }
    if (sliceEqlLower(unit, "tb") || sliceEqlLower(unit, "t")) {
        if (si_default) { return TB; }
        return TIB;
    }
    if (sliceEqlLower(unit, "pb") || sliceEqlLower(unit, "p")) {
        if (si_default) { return PB; }
        return PIB;
    }

    return BytesError.UnknownUnit;
}

fn parseBytesCore(input: []const u8, si_default: bool): u64 ! BytesError {
    // 1. Trim leading and trailing ASCII whitespace
    let start: usize = 0;
    let end: usize = input.len;
    while (start < end && isAsciiSpace(input[start])) : (start += 1) {}
    while (end > start && isAsciiSpace(input[end - 1])) : (end -= 1) {}

    if (start >= end) {
        return BytesError.EmptyInput;
    }

    let pos: usize = start;

    // 2. Parse integer part
    let int_part: u64 = 0;
    let has_int_digits: bool = false;
    while (pos < end && isAsciiDigit(input[pos])) : (pos += 1) {
        has_int_digits = true;
        const digit: u64 = u64(input[pos] - '0');
        if (int_part > (U64_MAX - digit) / 10) {
            return BytesError.Overflow;
        }
        int_part = int_part * 10 + digit;
    }

    // 3. Parse optional fractional part
    let frac_val: u64 = 0;
    let frac_scale: u64 = 1;
    let has_frac_digits: bool = false;
    if (pos < end && input[pos] == '.') {
        pos += 1;
        let frac_digits_kept: usize = 0;
        while (pos < end && isAsciiDigit(input[pos])) : (pos += 1) {
            has_frac_digits = true;
            if (frac_digits_kept < MAX_FRAC_DIGITS) {
                const digit: u64 = u64(input[pos] - '0');
                frac_val = frac_val * 10 + digit;
                frac_scale = frac_scale * 10;
                frac_digits_kept += 1;
            }
        }
        // Reject stray second decimal point (e.g. "1.2.3MB")
        if (pos < end && input[pos] == '.') {
            return BytesError.InvalidFormat;
        }
    }

    if (!has_int_digits && !has_frac_digits) {
        return BytesError.InvalidFormat;
    }

    // 4. Skip optional whitespace between number and unit
    while (pos < end && isAsciiSpace(input[pos])) : (pos += 1) {}

    // 5. Resolve unit multiplier
    const unit_slice = input[pos..end];
    const multiplier = resolveUnitMultiplier(unit_slice, si_default)?;

    // 6. Combine integer + fractional parts in u128 with strict u64 overflow check
    const int_bytes: u128 = u128(int_part) * u128(multiplier);
    let frac_bytes: u128 = 0;
    if (frac_scale > 1) {
        frac_bytes = (u128(frac_val) * u128(multiplier) + u128(frac_scale) / 2) / u128(frac_scale);
    }

    const total: u128 = int_bytes + frac_bytes;
    if (total > u128(U64_MAX)) {
        return BytesError.Overflow;
    }
    return total as u64;
}

/// Parse a human-readable byte string using binary (1024) base for K/KB/M/MB/G/GB/T/TB/P/PB
/// as well as IEC KiB/MiB/GiB/TiB/PiB units (compatible with npm `bytes`).
pub fn parseBytes(input: []const u8): u64 ! BytesError {
    return parseBytesCore(input, false);
}

/// Parse a human-readable byte string using binary (1024) base for all units (compatible with npm `bytes`).
pub fn parseBytesBinary(input: []const u8): u64 ! BytesError {
    return parseBytesCore(input, false);
}

/// Parse a human-readable byte string using SI decimal (1000) base for K/KB/M/MB/G/GB/T/TB/P/PB,
/// while explicit IEC suffixes (KiB/MiB/GiB/TiB/PiB) still use 1024 base.
pub fn parseBytesSi(input: []const u8): u64 ! BytesError {
    return parseBytesCore(input, true);
}

fn pow10U64(exp: u8): u64 {
    let result: u64 = 1;
    let i: u8 = 0;
    while (i < exp) : (i += 1) {
        result *= 10;
    }
    return result;
}

fn unitDivisor(idx: usize, is_si: bool): u64 {
    if (is_si) {
        if (idx == 1) { return KB; }
        if (idx == 2) { return MB; }
        if (idx == 3) { return GB; }
        if (idx == 4) { return TB; }
        if (idx == 5) { return PB; }
        return 1;
    }
    if (idx == 1) { return KIB; }
    if (idx == 2) { return MIB; }
    if (idx == 3) { return GIB; }
    if (idx == 4) { return TIB; }
    if (idx == 5) { return PIB; }
    return 1;
}

fn unitLabel(idx: usize, is_si: bool): string {
    if (is_si) {
        if (idx == 1) { return "KB"; }
        if (idx == 2) { return "MB"; }
        if (idx == 3) { return "GB"; }
        if (idx == 4) { return "TB"; }
        if (idx == 5) { return "PB"; }
        return "B";
    }
    if (idx == 1) { return "KiB"; }
    if (idx == 2) { return "MiB"; }
    if (idx == 3) { return "GiB"; }
    if (idx == 4) { return "TiB"; }
    if (idx == 5) { return "PiB"; }
    return "B";
}

fn appendBytes(out: []u8, pos: usize, src: []const u8): usize ! BytesError {
    if (pos + src.len > out.len) {
        return BytesError.BufferTooSmall;
    }
    let i: usize = 0;
    while (i < src.len) : (i += 1) {
        out[pos + i] = src[i];
    }
    return pos + src.len;
}

fn appendU64(out: []u8, pos: usize, value: u64): usize ! BytesError {
    let tmp: [20]u8 = [0; 20];
    let len: usize = 0;
    let v: u64 = value;
    if (v == 0) {
        tmp[0] = '0';
        len = 1;
    } else {
        while (v > 0) {
            tmp[len] = ('0' as u8) + ((v % 10) as u8);
            len += 1;
            v = v / 10;
        }
    }
    if (pos + len > out.len) {
        return BytesError.BufferTooSmall;
    }
    let i: usize = 0;
    while (i < len) : (i += 1) {
        out[pos + i] = tmp[len - 1 - i];
    }
    return pos + len;
}

/// Format a byte count `n` into caller-provided `out` buffer according to `opts`.
/// Supports binary IEC (`KiB`..`PiB`) and decimal SI (`KB`..`PB`), 0..4 decimal places
/// with automatic trailing-zero trimming and rounding carry promotion.
pub fn formatBytes(n: u64, opts: FormatOptions, out: []u8): ([]const u8) ! BytesError {
    const is_si: bool = opts.mode == ByteUnitMode.DecimalSi;
    let base: u64 = KIB;
    if (is_si) {
        base = KB;
    }

    let decimals: u8 = opts.decimals;
    if (decimals > MAX_DECIMALS) {
        decimals = MAX_DECIMALS;
    }

    // Select highest unit whose divisor <= n (up to PiB / PB, index 5)
    let unit_idx: usize = 0;
    while (unit_idx < 5) {
        const next_div = unitDivisor(unit_idx + 1, is_si);
        if (n >= next_div) {
            unit_idx += 1;
        } else {
            break;
        }
    }

    let pos: usize = 0;

    if (unit_idx == 0) {
        pos = appendU64(out, pos, n)?;
    } else {
        const scale: u64 = pow10U64(decimals);
        let div: u64 = unitDivisor(unit_idx, is_si);
        let scaled: u128 = (u128(n) * u128(scale) + u128(div) / 2) / u128(div);
        let int_part: u64 = (scaled / u128(scale)) as u64;
        let frac_part: u64 = (scaled % u128(scale)) as u64;

        // Handle rounding carry promotion (e.g. 1023.999 KiB -> 1 MiB)
        if (int_part >= base && unit_idx < 5) {
            unit_idx += 1;
            div = unitDivisor(unit_idx, is_si);
            scaled = (u128(n) * u128(scale) + u128(div) / 2) / u128(div);
            int_part = (scaled / u128(scale)) as u64;
            frac_part = (scaled % u128(scale)) as u64;
        }

        pos = appendU64(out, pos, int_part)?;

        if (decimals > 0 && frac_part > 0) {
            // Format fixed-width fractional digits then strip trailing zeros
            let frac_buf: [4]u8 = [0; 4];
            let f: u64 = frac_part;
            let idx: usize = usize(decimals);
            while (idx > 0) {
                idx -= 1;
                frac_buf[idx] = ('0' as u8) + ((f % 10) as u8);
                f = f / 10;
            }
            let trim_len: usize = usize(decimals);
            while (trim_len > 0 && frac_buf[trim_len - 1] == '0') {
                trim_len -= 1;
            }
            if (trim_len > 0) {
                pos = appendBytes(out, pos, ".")?;
                pos = appendBytes(out, pos, frac_buf[0..trim_len])?;
            }
        }
    }

    if (opts.space) {
        pos = appendBytes(out, pos, " ")?;
    }

    const label = unitLabel(unit_idx, is_si);
    pos = appendBytes(out, pos, label)?;
    return out[0..pos];
}

/// Format bytes using binary IEC units (`B`, `KiB`, `MiB`, `GiB`, `TiB`, `PiB`), 2 decimals, with space.
pub fn formatBinary(n: u64, out: []u8): ([]const u8) ! BytesError {
    const opts = FormatOptions {
        mode: ByteUnitMode.BinaryIec,
        decimals: 2,
        space: true,
    };
    return formatBytes(n, opts, out);
}

/// Format bytes using decimal SI units (`B`, `KB`, `MB`, `GB`, `TB`, `PB`), 2 decimals, with space.
pub fn formatDecimal(n: u64, out: []u8): ([]const u8) ! BytesError {
    const opts = FormatOptions {
        mode: ByteUnitMode.DecimalSi,
        decimals: 2,
        space: true,
    };
    return formatBytes(n, opts, out);
}

/// Format bytes in compact no-space SI format (e.g. `"512B"`, `"1.5MB"`, `"2.5GB"`).
pub fn formatCompact(n: u64, out: []u8): ([]const u8) ! BytesError {
    const opts = FormatOptions {
        mode: ByteUnitMode.DecimalSi,
        decimals: 2,
        space: false,
    };
    return formatBytes(n, opts, out);
}
