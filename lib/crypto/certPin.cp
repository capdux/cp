// Origin: Pure-CP Stdlib (Zero-Alloc TLS SPKI Public-Key Pinning & Backup Pin Rotation Engine) | Created: 2026-11-23
// Pure CP zero-heap-allocation TLS SubjectPublicKeyInfo (SPKI) SHA-256 Pinning Engine (RFC 7469 style):
//   - Fixed 32-host pin policy table supporting exact hostname & `include_subdomains` longest-suffix inheritance
//   - Up to 8 32-byte SHA-256 SPKI hashes per host, `expires_epoch_s`, and `enforce | report_only` mode
//   - Safe rotation guard (`require_backup_pin`): enforces >= 2 distinct non-zero SPKI pins before activation
//     or single-pin removal to prevent single-pin expiry / rotation bricking
//   - Constant-time certificate chain verification (`verifyChain`) returning `matched | no_pin_policy |
//     pin_expired | mismatch_blocked | mismatch_reported | invalid_input`
//   - Zero-alloc canonical RFC 4648 Base64 & `pin-sha256="..."` directive parser/formatter + telemetry snapshot

// ============================================================================
// 1. Constants, Enums & Data Models
// ============================================================================

/// Maximum number of host pinning policies stored inline.
pub const CERT_PIN_MAX_POLICIES: usize = 32;

/// Maximum number of SHA-256 SPKI pins per host policy.
pub const CERT_PIN_MAX_PINS_PER_HOST: usize = 8;

/// SHA-256 SPKI digest length in bytes.
pub const CERT_PIN_HASH_LEN: usize = 32;

/// Maximum normalized DNS hostname length in bytes.
pub const CERT_PIN_HOST_MAX_LEN: usize = 128;

/// Standard Base64 encoded length of a 32-byte SHA-256 digest (`ceil(32/3)*4 = 44`).
pub const CERT_PIN_B64_LEN: usize = 44;

/// Formatted length of `pin-sha256="<44B>"` (`12 + 44 + 1 = 57`).
pub const CERT_PIN_DIRECTIVE_LEN: usize = 57;

/// Enforcement mode for a host pinning policy.
pub enum CertPinMode {
    enforce,
    report_only,
}

/// Outcome of policy / pin mutation operations.
pub enum CertPinOpStatus {
    ok,
    invalid_host,
    invalid_pin_list,
    backup_pin_required,
    duplicate_pin,
    host_exists,
    not_found,
    capacity_full,
}

/// Verification verdict returned by `CertPinStore.verifyChain`.
pub enum CertPinVerifyStatus {
    matched,
    no_pin_policy,
    pin_expired,
    mismatch_blocked,
    mismatch_reported,
    invalid_input,
}

/// Inline TLS SPKI pinning policy for a single host (and optionally its subdomains).
pub struct CertPinPolicy {
    active: bool = false;
    host_buf: [CERT_PIN_HOST_MAX_LEN]u8 = [0; CERT_PIN_HOST_MAX_LEN];
    host_len: usize = 0;
    include_subdomains: bool = false;
    require_backup_pin: bool = true;
    mode: CertPinMode = CertPinMode.enforce;
    expires_epoch_s: i64 = 0;
    pin_hashes: [CERT_PIN_MAX_PINS_PER_HOST][CERT_PIN_HASH_LEN]u8 = [[0; CERT_PIN_HASH_LEN]; CERT_PIN_MAX_PINS_PER_HOST];
    pin_count: usize = 0;

    pub fn hostSlice(): []const u8 {
        return host_buf[0..host_len];
    }
}

/// Detailed verification verdict returned by `CertPinStore.verifyChain`.
pub struct CertPinDecision {
    status: CertPinVerifyStatus = CertPinVerifyStatus.no_pin_policy;
    allowed: bool = true;
    matched_policy_slot: usize = 0;
    has_policy: bool = false;
    matched_chain_index: usize = 0;
    matched_pin_index: usize = 0;
    via_subdomain: bool = false;
}

/// Zero-heap global telemetry snapshot for `CertPinStore`.
pub struct CertPinStats {
    active_policies: usize = 0;
    policy_capacity: usize = CERT_PIN_MAX_POLICIES;
    total_verifications: u64 = 0;
    matched_count: u64 = 0;
    no_policy_count: u64 = 0;
    expired_count: u64 = 0;
    mismatch_blocked_count: u64 = 0;
    mismatch_reported_count: u64 = 0;
    invalid_input_count: u64 = 0;
}

// ============================================================================
// 2. Constant-Time Comparison, DNS Normalization & Base64 `pin-sha256` Helpers
// ============================================================================

/// Constant-time 32-byte digest comparison (never short-circuits on mismatch).
pub fn certPinConstantTimeEq(a: [CERT_PIN_HASH_LEN]u8, b: [CERT_PIN_HASH_LEN]u8): bool {
    let diff: u8 = 0;
    let i: usize = 0;
    while (i < CERT_PIN_HASH_LEN) : (i += 1) {
        diff = diff | (a[i] ^ b[i]);
    }
    return diff == 0;
}

fn isAllZeroHash(h: [CERT_PIN_HASH_LEN]u8): bool {
    let acc: u8 = 0;
    let i: usize = 0;
    while (i < CERT_PIN_HASH_LEN) : (i += 1) {
        acc = acc | h[i];
    }
    return acc == 0;
}

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

/// Normalize a DNS hostname to lowercase without trailing root dot; rejects invalid syntax.
fn normalizeHost(raw: []const u8, out_buf: *[]u8): usize? {
    if (raw.len == 0) return null;
    let end = raw.len;
    if (raw[end - 1] == '.') {
        end -= 1;
    }
    if (end == 0 or end > CERT_PIN_HOST_MAX_LEN or end > out_buf.len) return null;

    let label_len: usize = 0;
    let i: usize = 0;
    while (i < end) : (i += 1) {
        let b = raw[i];
        if (b >= 'A' and b <= 'Z') {
            b = b + 32;
        }
        if (b == '.') {
            if (label_len == 0 or label_len > 63) return null;
            if (out_buf[i - 1] == '-') return null;
            out_buf[i] = '.';
            label_len = 0;
        } else {
            const is_lower = b >= 'a' and b <= 'z';
            const is_digit = b >= '0' and b <= '9';
            const is_hyphen = b == '-';
            if (!is_lower and !is_digit and !is_hyphen) return null;
            if (label_len == 0 and is_hyphen) return null;
            out_buf[i] = b;
            label_len += 1;
        }
    }
    if (label_len == 0 or label_len > 63) return null;
    if (out_buf[end - 1] == '-') return null;
    return end;
}

/// Strict DNS subdomain check: returns true iff `norm_host` is a strict subdomain of `norm_parent`
/// (i.e. `norm_host` ends with `".<norm_parent>"`).
fn isStrictSubdomainOf(norm_host: []const u8, norm_parent: []const u8): bool {
    if (norm_host.len <= norm_parent.len + 1) return false;
    const dot_idx = norm_host.len - norm_parent.len - 1;
    if (norm_host[dot_idx] != '.') return false;
    return bytesEq(norm_host[dot_idx + 1 ..], norm_parent);
}

const B64_ALPHABET: []const u8 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

fn b64DecodeChar(c: u8): u8? {
    if (c >= 'A' and c <= 'Z') return c - 'A';
    if (c >= 'a' and c <= 'z') return c - 'a' + 26;
    if (c >= '0' and c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return null;
}

/// Zero-alloc standard RFC 4648 Base64 encoding of a 32-byte SHA-256 SPKI hash (writes 44 bytes).
pub fn encodePinSha256Base64(hash: [CERT_PIN_HASH_LEN]u8, out_buf: *[]u8): usize? {
    if (out_buf.len < CERT_PIN_B64_LEN) return null;
    let src_i: usize = 0;
    let dst_i: usize = 0;

    // 10 full 3-byte blocks -> 40 Base64 chars (30 bytes)
    while (src_i < 30) : (src_i += 3) {
        const b0 = @as(u32, hash[src_i]);
        const b1 = @as(u32, hash[src_i + 1]);
        const b2 = @as(u32, hash[src_i + 2]);
        const triple = (b0 << 16) | (b1 << 8) | b2;
        out_buf[dst_i] = B64_ALPHABET[@as(usize, (triple >> 18) & 0x3F)];
        out_buf[dst_i + 1] = B64_ALPHABET[@as(usize, (triple >> 12) & 0x3F)];
        out_buf[dst_i + 2] = B64_ALPHABET[@as(usize, (triple >> 6) & 0x3F)];
        out_buf[dst_i + 3] = B64_ALPHABET[@as(usize, triple & 0x3F)];
        dst_i += 4;
    }

    // Final 2 bytes (hash[30], hash[31]) -> 3 Base64 chars + '='
    const rem0 = @as(u32, hash[30]);
    const rem1 = @as(u32, hash[31]);
    const tail = (rem0 << 16) | (rem1 << 8);
    out_buf[40] = B64_ALPHABET[@as(usize, (tail >> 18) & 0x3F)];
    out_buf[41] = B64_ALPHABET[@as(usize, (tail >> 12) & 0x3F)];
    out_buf[42] = B64_ALPHABET[@as(usize, (tail >> 6) & 0x3F)];
    out_buf[43] = '=';
    return CERT_PIN_B64_LEN;
}

/// Zero-alloc canonical RFC 4648 Base64 decoder for a 32-byte SHA-256 SPKI hash (strict 44 chars).
/// Rejects non-canonical trailing padding bits or invalid alphabet characters.
pub fn decodePinSha256Base64(b64: []const u8): ([CERT_PIN_HASH_LEN]u8)? {
    if (b64.len != CERT_PIN_B64_LEN) return null;
    if (b64[43] != '=') return null;

    let out: [CERT_PIN_HASH_LEN]u8 = [0; CERT_PIN_HASH_LEN];
    let src_i: usize = 0;
    let dst_i: usize = 0;

    while (src_i < 40) : (src_i += 4) {
        const v0 = b64DecodeChar(b64[src_i]) ?? return null;
        const v1 = b64DecodeChar(b64[src_i + 1]) ?? return null;
        const v2 = b64DecodeChar(b64[src_i + 2]) ?? return null;
        const v3 = b64DecodeChar(b64[src_i + 3]) ?? return null;
        const triple = (@as(u32, v0) << 18) | (@as(u32, v1) << 12) | (@as(u32, v2) << 6) | @as(u32, v3);
        out[dst_i] = @as(u8, @intCast((triple >> 16) & 0xFF));
        out[dst_i + 1] = @as(u8, @intCast((triple >> 8) & 0xFF));
        out[dst_i + 2] = @as(u8, @intCast(triple & 0xFF));
        dst_i += 3;
    }

    const t0 = b64DecodeChar(b64[40]) ?? return null;
    const t1 = b64DecodeChar(b64[41]) ?? return null;
    const t2 = b64DecodeChar(b64[42]) ?? return null;
    // Enforce canonical encoding: low 2 bits of 43rd char must be 0.
    if ((t2 & 0x03) != 0) return null;
    const tail = (@as(u32, t0) << 18) | (@as(u32, t1) << 12) | (@as(u32, t2) << 6);
    out[30] = @as(u8, @intCast((tail >> 16) & 0xFF));
    out[31] = @as(u8, @intCast((tail >> 8) & 0xFF));
    return out;
}

/// Zero-alloc formatter for RFC 7469 `pin-sha256="<44B Base64>"` directive (writes 57 bytes).
pub fn formatPinDirective(hash: [CERT_PIN_HASH_LEN]u8, out_buf: *[]u8): usize? {
    if (out_buf.len < CERT_PIN_DIRECTIVE_LEN) return null;
    const prefix = "pin-sha256=\"";
    let i: usize = 0;
    while (i < prefix.len) : (i += 1) {
        out_buf[i] = prefix[i];
    }
    let b64_slice: []u8 = out_buf[12 .. 12 + CERT_PIN_B64_LEN];
    const n = encodePinSha256Base64(hash, *b64_slice) ?? return null;
    if (n != CERT_PIN_B64_LEN) return null;
    out_buf[12 + CERT_PIN_B64_LEN] = '"';
    return CERT_PIN_DIRECTIVE_LEN;
}

/// Zero-alloc parser for RFC 7469 `pin-sha256="<44B Base64>"` directive (trims surrounding ASCII spaces).
pub fn parsePinDirective(directive: []const u8): ([CERT_PIN_HASH_LEN]u8)? {
    let start: usize = 0;
    let end: usize = directive.len;
    while (start < end and (directive[start] == ' ' or directive[start] == '\t')) : (start += 1) {}
    while (end > start and (directive[end - 1] == ' ' or directive[end - 1] == '\t')) : (end -= 1) {}

    const trimmed = directive[start..end];
    if (trimmed.len != CERT_PIN_DIRECTIVE_LEN) return null;
    if (!bytesEq(trimmed[0..12], "pin-sha256=\"")) return null;
    if (trimmed[CERT_PIN_DIRECTIVE_LEN - 1] != '"') return null;
    return decodePinSha256Base64(trimmed[12 .. 12 + CERT_PIN_B64_LEN]);
}

// ============================================================================
// 3. CertPinStore — Zero-Heap TLS SPKI Pinning & Backup Rotation Store
// ============================================================================

/// Zero-heap-allocation TLS SPKI public-key pinning policy store & constant-time verifier.
pub struct CertPinStore {
    policies: [CERT_PIN_MAX_POLICIES]CertPinPolicy = [CertPinPolicy{}; CERT_PIN_MAX_POLICIES];
    policy_count: usize = 0;

    total_verifications: u64 = 0;
    matched_count: u64 = 0;
    no_policy_count: u64 = 0;
    expired_count: u64 = 0;
    mismatch_blocked_count: u64 = 0;
    mismatch_reported_count: u64 = 0;
    invalid_input_count: u64 = 0;

    /// Initialize an empty SPKI pin policy store.
    static fn init(): CertPinStore {
        return CertPinStore{};
    }

    /// Clear all host policies and reset telemetry counters.
    pub fn clear(*_) {
        let i: usize = 0;
        while (i < CERT_PIN_MAX_POLICIES) : (i += 1) {
            policies[i] = CertPinPolicy{};
        }
        policy_count = 0;
        resetStats();
    }

    /// Validate a candidate pin slice against count, zero-hash, duplicate, and backup-pin rules.
    fn validatePinSet(pins: []const [CERT_PIN_HASH_LEN]u8, require_backup_pin: bool): CertPinOpStatus {
        if (pins.len == 0 or pins.len > CERT_PIN_MAX_PINS_PER_HOST) {
            return CertPinOpStatus.invalid_pin_list;
        }
        let i: usize = 0;
        while (i < pins.len) : (i += 1) {
            if (isAllZeroHash(pins[i])) {
                return CertPinOpStatus.invalid_pin_list;
            }
            let j: usize = i + 1;
            while (j < pins.len) : (j += 1) {
                if (certPinConstantTimeEq(pins[i], pins[j])) {
                    return CertPinOpStatus.duplicate_pin;
                }
            }
        }
        if (require_backup_pin and pins.len < 2) {
            return CertPinOpStatus.backup_pin_required;
        }
        return CertPinOpStatus.ok;
    }

    /// Find exact normalized host policy slot, or `null` if absent.
    pub fn findExactHost(host: []const u8): usize? {
        let norm_buf: [CERT_PIN_HOST_MAX_LEN]u8 = undefined;
        let norm_s: []u8 = norm_buf[0..];
        const norm_len = normalizeHost(host, *norm_s) ?? return null;
        const norm = norm_buf[0..norm_len];

        let i: usize = 0;
        while (i < CERT_PIN_MAX_POLICIES) : (i += 1) {
            if (policies[i].active and bytesEq(policies[i].hostSlice(), norm)) {
                return i;
            }
        }
        return null;
    }

    /// Return a read-only copy of the policy at `slot`, or `null` if inactive/out of bounds.
    pub fn getPolicy(slot: usize): CertPinPolicy? {
        if (slot >= CERT_PIN_MAX_POLICIES) return null;
        if (!policies[slot].active) return null;
        return policies[slot];
    }

    /// Register a new host SPKI pin policy.
    /// When `require_backup_pin` is true, at least 2 distinct non-zero pins are required.
    pub fn addPolicy(
        *_,
        host: []const u8,
        pins: []const [CERT_PIN_HASH_LEN]u8,
        include_subdomains: bool,
        require_backup_pin: bool,
        mode: CertPinMode,
        expires_epoch_s: i64,
    ): CertPinOpStatus {
        let norm_buf: [CERT_PIN_HOST_MAX_LEN]u8 = undefined;
        let norm_s: []u8 = norm_buf[0..];
        const norm_len = normalizeHost(host, *norm_s) ?? return CertPinOpStatus.invalid_host;
        const norm = norm_buf[0..norm_len];

        const pin_check = validatePinSet(pins, require_backup_pin);
        if (pin_check != CertPinOpStatus.ok) return pin_check;

        let free_slot: usize = CERT_PIN_MAX_POLICIES;
        let i: usize = 0;
        while (i < CERT_PIN_MAX_POLICIES) : (i += 1) {
            if (policies[i].active) {
                if (bytesEq(policies[i].hostSlice(), norm)) {
                    return CertPinOpStatus.host_exists;
                }
            } else if (free_slot == CERT_PIN_MAX_POLICIES) {
                free_slot = i;
            }
        }
        if (free_slot == CERT_PIN_MAX_POLICIES) return CertPinOpStatus.capacity_full;

        let pol = CertPinPolicy{};
        pol.active = true;
        let k: usize = 0;
        while (k < norm_len) : (k += 1) {
            pol.host_buf[k] = norm_buf[k];
        }
        pol.host_len = norm_len;
        pol.include_subdomains = include_subdomains;
        pol.require_backup_pin = require_backup_pin;
        pol.mode = mode;
        pol.expires_epoch_s = expires_epoch_s;
        pol.pin_count = pins.len;
        let p: usize = 0;
        while (p < pins.len) : (p += 1) {
            pol.pin_hashes[p] = pins[p];
        }

        policies[free_slot] = pol;
        policy_count += 1;
        return CertPinOpStatus.ok;
    }

    /// Replace the pin set and expiration of an existing host policy (enforces `require_backup_pin`).
    pub fn rotatePins(
        *_,
        host: []const u8,
        new_pins: []const [CERT_PIN_HASH_LEN]u8,
        new_expires_epoch_s: i64,
    ): CertPinOpStatus {
        const slot = findExactHost(host) ?? return CertPinOpStatus.not_found;
        const pin_check = validatePinSet(new_pins, policies[slot].require_backup_pin);
        if (pin_check != CertPinOpStatus.ok) return pin_check;

        policies[slot].pin_count = new_pins.len;
        let p: usize = 0;
        while (p < new_pins.len) : (p += 1) {
            policies[slot].pin_hashes[p] = new_pins[p];
        }
        policies[slot].expires_epoch_s = new_expires_epoch_s;
        return CertPinOpStatus.ok;
    }

    /// Append a new backup SPKI pin to an existing host policy (rejects zero/duplicate/full).
    pub fn addPin(*_, host: []const u8, new_pin: [CERT_PIN_HASH_LEN]u8): CertPinOpStatus {
        if (isAllZeroHash(new_pin)) return CertPinOpStatus.invalid_pin_list;
        const slot = findExactHost(host) ?? return CertPinOpStatus.not_found;
        const cnt = policies[slot].pin_count;
        if (cnt >= CERT_PIN_MAX_PINS_PER_HOST) return CertPinOpStatus.capacity_full;

        let i: usize = 0;
        while (i < cnt) : (i += 1) {
            if (certPinConstantTimeEq(policies[slot].pin_hashes[i], new_pin)) {
                return CertPinOpStatus.duplicate_pin;
            }
        }
        policies[slot].pin_hashes[cnt] = new_pin;
        policies[slot].pin_count = cnt + 1;
        return CertPinOpStatus.ok;
    }

    /// Remove a retired SPKI pin from an existing host policy.
    /// Refuses removal with `backup_pin_required` if dropping below 2 pins while `require_backup_pin` is active.
    pub fn removePin(*_, host: []const u8, old_pin: [CERT_PIN_HASH_LEN]u8): CertPinOpStatus {
        const slot = findExactHost(host) ?? return CertPinOpStatus.not_found;
        const cnt = policies[slot].pin_count;

        let found_idx: usize = CERT_PIN_MAX_PINS_PER_HOST;
        let i: usize = 0;
        while (i < cnt) : (i += 1) {
            if (certPinConstantTimeEq(policies[slot].pin_hashes[i], old_pin)) {
                found_idx = i;
            }
        }
        if (found_idx == CERT_PIN_MAX_PINS_PER_HOST) return CertPinOpStatus.not_found;

        const min_required: usize = if (policies[slot].require_backup_pin) 2 else 1;
        if (cnt <= min_required) {
            return CertPinOpStatus.backup_pin_required;
        }

        // Compact remaining pins
        let j = found_idx;
        while (j + 1 < cnt) : (j += 1) {
            policies[slot].pin_hashes[j] = policies[slot].pin_hashes[j + 1];
        }
        policies[slot].pin_hashes[cnt - 1] = [0; CERT_PIN_HASH_LEN];
        policies[slot].pin_count = cnt - 1;
        return CertPinOpStatus.ok;
    }

    /// Remove an entire host policy.
    pub fn removePolicy(*_, host: []const u8): CertPinOpStatus {
        const slot = findExactHost(host) ?? return CertPinOpStatus.not_found;
        policies[slot] = CertPinPolicy{};
        if (policy_count > 0) policy_count -= 1;
        return CertPinOpStatus.ok;
    }

    /// Verify a peer TLS certificate chain's SPKI SHA-256 hashes against the matching host policy
    /// using constant-time digest comparisons.
    /// Exact host policy takes precedence over `include_subdomains` longest-parent inheritance.
    pub fn verifyChain(
        *_,
        host: []const u8,
        chain_spki_hashes: []const [CERT_PIN_HASH_LEN]u8,
        now_epoch_s: i64,
    ): CertPinDecision {
        total_verifications += 1;

        if (chain_spki_hashes.len == 0) {
            invalid_input_count += 1;
            return CertPinDecision{
                status: CertPinVerifyStatus.invalid_input,
                allowed: false,
            };
        }

        let norm_buf: [CERT_PIN_HOST_MAX_LEN]u8 = undefined;
        let norm_s: []u8 = norm_buf[0..];
        const norm_len_opt = normalizeHost(host, *norm_s);
        if (norm_len_opt == null) {
            invalid_input_count += 1;
            return CertPinDecision{
                status: CertPinVerifyStatus.invalid_input,
                allowed: false,
            };
        }
        const norm = norm_buf[0 .. (norm_len_opt ?? 0)];

        // 1. Resolve policy: exact match first, then longest matching `include_subdomains` parent.
        let matched_slot: usize = CERT_PIN_MAX_POLICIES;
        let matched_via_sub = false;
        let best_sub_len: usize = 0;

        let i: usize = 0;
        while (i < CERT_PIN_MAX_POLICIES) : (i += 1) {
            if (!policies[i].active) continue;
            const p_host = policies[i].hostSlice();
            if (bytesEq(norm, p_host)) {
                matched_slot = i;
                matched_via_sub = false;
                break;
            }
            if (policies[i].include_subdomains and isStrictSubdomainOf(norm, p_host)) {
                if (p_host.len > best_sub_len) {
                    best_sub_len = p_host.len;
                    matched_slot = i;
                    matched_via_sub = true;
                }
            }
        }

        if (matched_slot == CERT_PIN_MAX_POLICIES) {
            no_policy_count += 1;
            return CertPinDecision{
                status: CertPinVerifyStatus.no_pin_policy,
                allowed: true,
                has_policy: false,
            };
        }

        // 2. Expiration check (RFC 7469: expired pins fail open with `pin_expired` telemetry).
        const exp_s = policies[matched_slot].expires_epoch_s;
        if (exp_s > 0 and now_epoch_s > exp_s) {
            expired_count += 1;
            return CertPinDecision{
                status: CertPinVerifyStatus.pin_expired,
                allowed: true,
                matched_policy_slot: matched_slot,
                has_policy: true,
                via_subdomain: matched_via_sub,
            };
        }

        // 3. Constant-time scan over every (chain_cert, policy_pin) pair.
        let hit = false;
        let hit_chain_idx: usize = 0;
        let hit_pin_idx: usize = 0;
        const pin_cnt = policies[matched_slot].pin_count;

        let c_idx: usize = 0;
        while (c_idx < chain_spki_hashes.len) : (c_idx += 1) {
            let p_idx: usize = 0;
            while (p_idx < pin_cnt) : (p_idx += 1) {
                const eq = certPinConstantTimeEq(chain_spki_hashes[c_idx], policies[matched_slot].pin_hashes[p_idx]);
                if (eq and !hit) {
                    hit = true;
                    hit_chain_idx = c_idx;
                    hit_pin_idx = p_idx;
                }
            }
        }

        if (hit) {
            matched_count += 1;
            return CertPinDecision{
                status: CertPinVerifyStatus.matched,
                allowed: true,
                matched_policy_slot: matched_slot,
                has_policy: true,
                matched_chain_index: hit_chain_idx,
                matched_pin_index: hit_pin_idx,
                via_subdomain: matched_via_sub,
            };
        }

        if (policies[matched_slot].mode == CertPinMode.report_only) {
            mismatch_reported_count += 1;
            return CertPinDecision{
                status: CertPinVerifyStatus.mismatch_reported,
                allowed: true,
                matched_policy_slot: matched_slot,
                has_policy: true,
                via_subdomain: matched_via_sub,
            };
        }

        mismatch_blocked_count += 1;
        return CertPinDecision{
            status: CertPinVerifyStatus.mismatch_blocked,
            allowed: false,
            matched_policy_slot: matched_slot,
            has_policy: true,
            via_subdomain: matched_via_sub,
        };
    }

    /// Return a zero-heap global telemetry snapshot.
    pub fn stats(): CertPinStats {
        return CertPinStats{
            active_policies: policy_count,
            policy_capacity: CERT_PIN_MAX_POLICIES,
            total_verifications: total_verifications,
            matched_count: matched_count,
            no_policy_count: no_policy_count,
            expired_count: expired_count,
            mismatch_blocked_count: mismatch_blocked_count,
            mismatch_reported_count: mismatch_reported_count,
            invalid_input_count: invalid_input_count,
        };
    }

    /// Reset telemetry counters while preserving active host policies.
    pub fn resetStats(*_) {
        total_verifications = 0;
        matched_count = 0;
        no_policy_count = 0;
        expired_count = 0;
        mismatch_blocked_count = 0;
        mismatch_reported_count = 0;
        invalid_input_count = 0;
    }
}
