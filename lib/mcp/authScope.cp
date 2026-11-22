// Origin: Pure-CP Stdlib (Zero-Alloc MCP OAuth 2.1 Resource Server Auth Scope & RFC 9728 Challenge Engine) | Created: 2026-11-23
// Pure CP zero-heap-allocation Model Context Protocol (MCP 2025-03-26) OAuth 2.1 Resource Server
// Scope Policy Engine & RFC 9728 / RFC 6750 WWW-Authenticate Challenge Formatter:
//   - Fixed 64-bit named scope registry (up to 64 scopes) with zero-alloc RFC 6749 space-delimited
//     scope parsing (`parseScopesToMask`) and reverse bitmask formatting (`formatScopeMask`)
//   - Fixed 64-rule method/tool scope policy table mapping MCP methods/tools to `required_scopes: u64`,
//     `require_all: bool` (AND vs ANY semantics), and expected resource audience URI (`expected_audience`)
//   - Strict OAuth 2.1 token claims validator (`authorizeCall`): audience binding (`aud == expected_audience`)
//     against Confused-Deputy token passthrough, overflow-safe `nbf`/`exp` time window validation with
//     configurable `clock_skew_s`, and scope bitmask coverage check returning exact `missing_scopes`
//   - Zero-alloc RFC 9728 / RFC 6750 `WWW-Authenticate: Bearer ...` header formatter with strict
//     CRLF / quote / backslash / control-character injection rejection
//   - Zero-heap global telemetry snapshot (`McpAuthScopeStats`)

// ============================================================================
// 1. Constants, Enums & Data Models
// ============================================================================

/// Maximum number of MCP method/tool authorization policies stored inline.
pub const MCP_AUTH_MAX_POLICIES: usize = 64;

/// Maximum number of distinct OAuth 2.1 scopes mapped onto a 64-bit bitmask.
pub const MCP_AUTH_MAX_SCOPES: usize = 64;

/// Maximum byte length of a registered MCP method or tool name.
pub const MCP_AUTH_METHOD_MAX_LEN: usize = 96;

/// Maximum byte length of an audience URI or RFC 9728 resource metadata URL.
pub const MCP_AUTH_URI_MAX_LEN: usize = 192;

/// Maximum byte length of a single OAuth 2.1 scope token name.
pub const MCP_AUTH_SCOPE_NAME_MAX_LEN: usize = 48;

/// Default clock skew tolerance in seconds for `exp` / `nbf` validation.
pub const MCP_AUTH_DEFAULT_CLOCK_SKEW_S: i64 = 30;

/// Authorization verdict returned by `McpAuthScopeEngine.authorizeCall`.
pub enum McpAuthStatus {
    allowed,
    missing_token,
    invalid_audience,
    token_expired,
    token_not_yet_valid,
    insufficient_scope,
    unknown_policy,
}

/// Mutation outcome returned by policy/scope registration helpers.
pub enum McpAuthOpStatus {
    ok,
    invalid_input,
    already_exists,
    not_found,
    capacity_full,
}

/// Inline named OAuth 2.1 scope entry mapped to bit `(1 << bit_index)`.
pub struct McpScopeDef {
    active: bool = false;
    bit_index: u6 = 0;
    name_buf: [MCP_AUTH_SCOPE_NAME_MAX_LEN]u8 = [0; MCP_AUTH_SCOPE_NAME_MAX_LEN];
    name_len: usize = 0;

    pub fn nameSlice(): []const u8 {
        return name_buf[0..name_len];
    }

    pub fn bitMask(): u64 {
        return (@as(u64, 1)) << bit_index;
    }
}

/// Inline scope authorization policy for a single MCP method or tool.
pub struct McpScopePolicy {
    active: bool = false;
    method_buf: [MCP_AUTH_METHOD_MAX_LEN]u8 = [0; MCP_AUTH_METHOD_MAX_LEN];
    method_len: usize = 0;
    required_scopes: u64 = 0;
    require_all: bool = true;
    audience_buf: [MCP_AUTH_URI_MAX_LEN]u8 = [0; MCP_AUTH_URI_MAX_LEN];
    audience_len: usize = 0;

    pub fn methodSlice(): []const u8 {
        return method_buf[0..method_len];
    }

    pub fn expectedAudienceSlice(): []const u8 {
        return audience_buf[0..audience_len];
    }
}

/// Decoded OAuth 2.1 access token claims supplied by the caller (zero-copy slices).
pub struct McpTokenClaims {
    present: bool = false;
    subject: []const u8 = "";
    audience: []const u8 = "";
    scope_str: []const u8 = "";
    granted_mask: u64 = 0;
    exp_epoch_s: i64 = 0;
    nbf_epoch_s: i64 = 0;

    /// Construct an absent/missing bearer token claim set.
    static fn missing(): McpTokenClaims {
        return McpTokenClaims{ present: false };
    }

    /// Construct a present bearer token claim set from decoded JWT/introspection fields.
    static fn make(
        audience: []const u8,
        scope_str: []const u8,
        exp_epoch_s: i64,
        nbf_epoch_s: i64,
    ): McpTokenClaims {
        return McpTokenClaims{
            present: true,
            subject: "",
            audience: audience,
            scope_str: scope_str,
            granted_mask: 0,
            exp_epoch_s: exp_epoch_s,
            nbf_epoch_s: nbf_epoch_s,
        };
    }

    /// Construct a present bearer token claim set with explicit subject & optional pre-parsed mask.
    static fn makeWithSubject(
        subject: []const u8,
        audience: []const u8,
        scope_str: []const u8,
        granted_mask: u64,
        exp_epoch_s: i64,
        nbf_epoch_s: i64,
    ): McpTokenClaims {
        return McpTokenClaims{
            present: true,
            subject: subject,
            audience: audience,
            scope_str: scope_str,
            granted_mask: granted_mask,
            exp_epoch_s: exp_epoch_s,
            nbf_epoch_s: nbf_epoch_s,
        };
    }
}

/// Detailed authorization decision returned by `authorizeCall`.
pub struct McpAuthDecision {
    status: McpAuthStatus = McpAuthStatus.unknown_policy;
    allowed: bool = false;
    http_status: u16 = 403;
    required_scopes: u64 = 0;
    granted_scopes: u64 = 0;
    missing_scopes: u64 = 0;
    matched_slot: usize = 0;
    has_policy: bool = false;
}

/// Zero-heap global telemetry snapshot for `McpAuthScopeEngine`.
pub struct McpAuthScopeStats {
    registered_scopes: usize = 0;
    active_policies: usize = 0;
    policy_capacity: usize = MCP_AUTH_MAX_POLICIES;
    total_checks: u64 = 0;
    allowed_count: u64 = 0;
    unauthorized_401_count: u64 = 0;
    forbidden_403_count: u64 = 0;
    missing_token_count: u64 = 0;
    invalid_audience_count: u64 = 0;
    token_expired_count: u64 = 0;
    token_not_yet_valid_count: u64 = 0;
    insufficient_scope_count: u64 = 0;
    unknown_policy_count: u64 = 0;
}

// ============================================================================
// 2. Internal Byte & RFC 6749 / RFC 6750 / RFC 9728 Validation Helpers
// ============================================================================

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn copyIntoBuf(dst: []u8, src: []const u8): usize {
    let i: usize = 0;
    while (i < src.len) : (i += 1) {
        dst[i] = src[i];
    }
    return src.len;
}

fn appendBytes(out_buf: *[]u8, pos: *usize, src: []const u8): bool {
    if (pos + src.len > out_buf.len) return false;
    let i: usize = 0;
    while (i < src.len) : (i += 1) {
        out_buf[pos + i] = src[i];
    }
    pos += src.len;
    return true;
}

/// Validate a single RFC 6749 §3.3 `scope-token` (`%x21 / %x23-5B / %x5D-7E`).
/// Rejects whitespace, control chars, double-quote (`"`), backslash (`\`), and non-ASCII bytes.
fn isValidScopeToken(token: []const u8): bool {
    if (token.len == 0 or token.len > MCP_AUTH_SCOPE_NAME_MAX_LEN) return false;
    let i: usize = 0;
    while (i < token.len) : (i += 1) {
        const b = token[i];
        if (b < 0x21 or b > 0x7E) return false;
        if (b == 0x22 or b == 0x5C) return false; // '"' or '\\'
    }
    return true;
}

/// Validate an RFC 6750 `error` code token (ASCII alphanumeric, `_`, `-`).
fn isValidErrorCode(code: []const u8): bool {
    let i: usize = 0;
    while (i < code.len) : (i += 1) {
        const b = code[i];
        const is_lower = b >= 'a' and b <= 'z';
        const is_upper = b >= 'A' and b <= 'Z';
        const is_digit = b >= '0' and b <= '9';
        const is_punct = b == '_' or b == '-';
        if (!is_lower and !is_upper and !is_digit and !is_punct) return false;
    }
    return true;
}

/// Validate that a URI/URL has no whitespace, control characters, quotes, or backslashes.
fn isValidHeaderUri(uri: []const u8): bool {
    if (uri.len == 0 or uri.len > MCP_AUTH_URI_MAX_LEN) return false;
    let i: usize = 0;
    while (i < uri.len) : (i += 1) {
        const b = uri[i];
        if (b <= 0x20 or b >= 0x7F) return false;
        if (b == 0x22 or b == 0x5C) return false; // '"' or '\\'
    }
    return true;
}

/// Validate that a quoted-string parameter value is safe against HTTP header / CRLF / quote injection.
/// Rejects `\r`, `\n`, NUL, ASCII control chars (`< 0x20`), DEL/non-ASCII (`>= 0x7F`), `"` and `\`.
pub fn isSafeHeaderParamValue(s: []const u8): bool {
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        const b = s[i];
        if (b < 0x20 or b >= 0x7F) return false;
        if (b == 0x22 or b == 0x5C) return false; // '"' or '\\'
    }
    return true;
}

/// Overflow-safe check: returns true if `now_s > exp_s + skew_s` (assuming `exp_s > 0` and `skew_s >= 0`).
fn isTokenExpiredAt(now_s: i64, exp_s: i64, skew_s: i64): bool {
    if (exp_s <= 0) return true;
    if (now_s <= exp_s) return false;
    // Both now_s > exp_s > 0, so (now_s - exp_s) cannot overflow i64.
    return (now_s - exp_s) > skew_s;
}

/// Overflow-safe check: returns true if `now_s + skew_s < nbf_s` (assuming `nbf_s > 0` and `skew_s >= 0`).
fn isTokenNotYetValidAt(now_s: i64, nbf_s: i64, skew_s: i64): bool {
    if (nbf_s <= 0) return false;
    if (now_s >= nbf_s) return false;
    // If now_s <= 0, compare nbf_s > skew_s + now_s safely:
    if (now_s <= 0) {
        // nbf_s > 0 and now_s <= 0; check if nbf_s - skew_s > now_s.
        const threshold = nbf_s - skew_s;
        return now_s < threshold;
    }
    // Both 0 < now_s < nbf_s, so (nbf_s - now_s) cannot overflow i64.
    return (nbf_s - now_s) > skew_s;
}

// ============================================================================
// 3. RFC 9728 / RFC 6750 WWW-Authenticate Header Formatter (Zero-Alloc)
// ============================================================================

/// Format a zero-allocation RFC 9728 / RFC 6750 `WWW-Authenticate` HTTP response header value:
///   `Bearer resource_metadata="...", error="...", error_description="...", scope="..."`
///
/// Empty optional fields are omitted. Returns `null` if any parameter fails CRLF / quote /
/// backslash / control-character injection checks, or if `out_buf` is too small.
pub fn formatWwwAuthenticateHeader(
    resource_metadata_url: []const u8,
    error_code: []const u8,
    error_description: []const u8,
    scope_str: []const u8,
    out_buf: *[]u8,
): usize? {
    if (resource_metadata_url.len > 0 and !isValidHeaderUri(resource_metadata_url)) return null;
    if (error_code.len > 0 and !isValidErrorCode(error_code)) return null;
    if (error_description.len > 0 and !isSafeHeaderParamValue(error_description)) return null;
    if (scope_str.len > 0 and !isSafeHeaderParamValue(scope_str)) return null;

    let pos: usize = 0;
    if (!appendBytes(*out_buf, *pos, "Bearer")) return null;

    let has_param = false;

    if (resource_metadata_url.len > 0) {
        if (!appendBytes(*out_buf, *pos, " resource_metadata=\"")) return null;
        if (!appendBytes(*out_buf, *pos, resource_metadata_url)) return null;
        if (!appendBytes(*out_buf, *pos, "\"")) return null;
        has_param = true;
    }

    if (error_code.len > 0) {
        if (has_param) {
            if (!appendBytes(*out_buf, *pos, ", error=\"")) return null;
        } else {
            if (!appendBytes(*out_buf, *pos, " error=\"")) return null;
        }
        if (!appendBytes(*out_buf, *pos, error_code)) return null;
        if (!appendBytes(*out_buf, *pos, "\"")) return null;
        has_param = true;
    }

    if (error_description.len > 0) {
        if (has_param) {
            if (!appendBytes(*out_buf, *pos, ", error_description=\"")) return null;
        } else {
            if (!appendBytes(*out_buf, *pos, " error_description=\"")) return null;
        }
        if (!appendBytes(*out_buf, *pos, error_description)) return null;
        if (!appendBytes(*out_buf, *pos, "\"")) return null;
        has_param = true;
    }

    if (scope_str.len > 0) {
        if (has_param) {
            if (!appendBytes(*out_buf, *pos, ", scope=\"")) return null;
        } else {
            if (!appendBytes(*out_buf, *pos, " scope=\"")) return null;
        }
        if (!appendBytes(*out_buf, *pos, scope_str)) return null;
        if (!appendBytes(*out_buf, *pos, "\"")) return null;
    }

    return pos;
}

// ============================================================================
// 4. McpAuthScopeEngine — Zero-Heap OAuth 2.1 Resource Server Policy Engine
// ============================================================================

/// Zero-heap-allocation MCP 2025-03-26 OAuth 2.1 resource server scope policy engine.
pub struct McpAuthScopeEngine {
    default_audience_buf: [MCP_AUTH_URI_MAX_LEN]u8 = [0; MCP_AUTH_URI_MAX_LEN];
    default_audience_len: usize = 0;
    resource_metadata_buf: [MCP_AUTH_URI_MAX_LEN]u8 = [0; MCP_AUTH_URI_MAX_LEN];
    resource_metadata_len: usize = 0;
    clock_skew_s: i64 = MCP_AUTH_DEFAULT_CLOCK_SKEW_S;

    scopes: [MCP_AUTH_MAX_SCOPES]McpScopeDef = [McpScopeDef{}; MCP_AUTH_MAX_SCOPES];
    scope_count: usize = 0;

    policies: [MCP_AUTH_MAX_POLICIES]McpScopePolicy = [McpScopePolicy{}; MCP_AUTH_MAX_POLICIES];
    policy_count: usize = 0;

    total_checks: u64 = 0;
    allowed_count: u64 = 0;
    unauthorized_401_count: u64 = 0;
    forbidden_403_count: u64 = 0;
    missing_token_count: u64 = 0;
    invalid_audience_count: u64 = 0;
    token_expired_count: u64 = 0;
    token_not_yet_valid_count: u64 = 0;
    insufficient_scope_count: u64 = 0;
    unknown_policy_count: u64 = 0;

    /// Initialize an empty engine with default resource audience, RFC 9728 metadata URL,
    /// and clock skew tolerance in seconds (negative skew is clamped to 0).
    static fn init(
        default_audience: []const u8,
        resource_metadata_url: []const u8,
        clock_skew_s: i64,
    ): McpAuthScopeEngine {
        let eng = McpAuthScopeEngine{};
        eng.clock_skew_s = if (clock_skew_s < 0) 0 else clock_skew_s;
        if (isValidHeaderUri(default_audience)) {
            eng.default_audience_len = copyIntoBuf(eng.default_audience_buf[0..], default_audience);
        }
        if (isValidHeaderUri(resource_metadata_url)) {
            eng.resource_metadata_len = copyIntoBuf(eng.resource_metadata_buf[0..], resource_metadata_url);
        }
        return eng;
    }

    /// Return the configured default expected audience URI slice.
    pub fn defaultAudience(): []const u8 {
        return default_audience_buf[0..default_audience_len];
    }

    /// Return the configured RFC 9728 protected resource metadata URL slice.
    pub fn resourceMetadataUrl(): []const u8 {
        return resource_metadata_buf[0..resource_metadata_len];
    }

    /// Update the configured clock skew tolerance (negative values clamped to 0).
    pub fn setClockSkew(*_, skew_s: i64) {
        clock_skew_s = if (skew_s < 0) 0 else skew_s;
    }

    /// Update the RFC 9728 resource metadata URL (rejects invalid/injected URIs).
    pub fn setResourceMetadataUrl(*_, url: []const u8): McpAuthOpStatus {
        if (!isValidHeaderUri(url)) return McpAuthOpStatus.invalid_input;
        resource_metadata_len = copyIntoBuf(resource_metadata_buf[0..], url);
        return McpAuthOpStatus.ok;
    }

    /// Register a named OAuth 2.1 scope and return its assigned 64-bit mask (`1 << bit_index`).
    /// Idempotent: re-registering an existing scope name returns its existing bit mask.
    /// Returns `null` if the scope name is invalid or all 64 scope bits are exhausted.
    pub fn registerScope(*_, name: []const u8): u64? {
        if (!isValidScopeToken(name)) return null;

        let free_slot: usize = MCP_AUTH_MAX_SCOPES;
        let i: usize = 0;
        while (i < MCP_AUTH_MAX_SCOPES) : (i += 1) {
            if (scopes[i].active) {
                if (bytesEq(scopes[i].nameSlice(), name)) {
                    return scopes[i].bitMask();
                }
            } else if (free_slot == MCP_AUTH_MAX_SCOPES) {
                free_slot = i;
            }
        }

        if (free_slot == MCP_AUTH_MAX_SCOPES) return null;

        scopes[free_slot] = McpScopeDef{};
        scopes[free_slot].active = true;
        scopes[free_slot].bit_index = @as(u6, @intCast(free_slot));
        scopes[free_slot].name_len = copyIntoBuf(scopes[free_slot].name_buf[0..], name);
        scope_count += 1;
        return scopes[free_slot].bitMask();
    }

    /// Look up the single-bit mask of a registered scope name (returns `0` if unregistered).
    pub fn resolveScopeBit(name: []const u8): u64 {
        if (name.len == 0) return 0;
        let i: usize = 0;
        while (i < MCP_AUTH_MAX_SCOPES) : (i += 1) {
            if (scopes[i].active and bytesEq(scopes[i].nameSlice(), name)) {
                return scopes[i].bitMask();
            }
        }
        return 0;
    }

    /// Parse an RFC 6749 space-delimited `scope` string into a 64-bit bitmask of registered scopes.
    /// Unknown or unregistered scope tokens are safely ignored.
    pub fn parseScopesToMask(scope_list: []const u8): u64 {
        let mask: u64 = 0;
        let i: usize = 0;
        while (i < scope_list.len) {
            // Skip ASCII spaces (0x20).
            while (i < scope_list.len and scope_list[i] == ' ') : (i += 1) {}
            if (i >= scope_list.len) break;
            const start = i;
            while (i < scope_list.len and scope_list[i] != ' ') : (i += 1) {}
            const tok = scope_list[start..i];
            mask = mask | resolveScopeBit(tok);
        }
        return mask;
    }

    /// Format a 64-bit scope bitmask back into an RFC 6749 space-delimited scope string.
    /// Returns the number of bytes written, or `null` if `out_buf` is too small.
    pub fn formatScopeMask(mask: u64, out_buf: *[]u8): usize? {
        let pos: usize = 0;
        let wrote_any = false;
        let i: usize = 0;
        while (i < MCP_AUTH_MAX_SCOPES) : (i += 1) {
            if (!scopes[i].active) continue;
            const bit = scopes[i].bitMask();
            if ((mask & bit) != 0) {
                if (wrote_any) {
                    if (!appendBytes(*out_buf, *pos, " ")) return null;
                }
                if (!appendBytes(*out_buf, *pos, scopes[i].nameSlice())) return null;
                wrote_any = true;
            }
        }
        return pos;
    }

    /// Find the policy slot index for `method_or_tool`, or `null` if not registered.
    pub fn findPolicy(method_or_tool: []const u8): usize? {
        if (method_or_tool.len == 0) return null;
        let i: usize = 0;
        while (i < MCP_AUTH_MAX_POLICIES) : (i += 1) {
            if (policies[i].active and bytesEq(policies[i].methodSlice(), method_or_tool)) {
                return i;
            }
        }
        return null;
    }

    /// Return a read-only copy of the policy at `slot_index`, or `null` if inactive/out of bounds.
    pub fn getPolicy(slot_index: usize): McpScopePolicy? {
        if (slot_index >= MCP_AUTH_MAX_POLICIES) return null;
        if (!policies[slot_index].active) return null;
        return policies[slot_index];
    }

    /// Register a new scope policy for an MCP method or tool.
    /// If `expected_audience` is empty, the engine's `default_audience` is used during authorization.
    pub fn addPolicy(
        *_,
        method_or_tool: []const u8,
        required_scopes: u64,
        require_all: bool,
        expected_audience: []const u8,
    ): McpAuthOpStatus {
        if (method_or_tool.len == 0 or method_or_tool.len > MCP_AUTH_METHOD_MAX_LEN) {
            return McpAuthOpStatus.invalid_input;
        }
        if (!isSafeHeaderParamValue(method_or_tool)) {
            return McpAuthOpStatus.invalid_input;
        }
        if (expected_audience.len > 0 and !isValidHeaderUri(expected_audience)) {
            return McpAuthOpStatus.invalid_input;
        }

        let free_slot: usize = MCP_AUTH_MAX_POLICIES;
        let i: usize = 0;
        while (i < MCP_AUTH_MAX_POLICIES) : (i += 1) {
            if (policies[i].active) {
                if (bytesEq(policies[i].methodSlice(), method_or_tool)) {
                    return McpAuthOpStatus.already_exists;
                }
            } else if (free_slot == MCP_AUTH_MAX_POLICIES) {
                free_slot = i;
            }
        }

        if (free_slot == MCP_AUTH_MAX_POLICIES) return McpAuthOpStatus.capacity_full;

        policies[free_slot] = McpScopePolicy{};
        policies[free_slot].active = true;
        policies[free_slot].method_len = copyIntoBuf(policies[free_slot].method_buf[0..], method_or_tool);
        policies[free_slot].required_scopes = required_scopes;
        policies[free_slot].require_all = require_all;
        if (expected_audience.len > 0) {
            policies[free_slot].audience_len = copyIntoBuf(policies[free_slot].audience_buf[0..], expected_audience);
        }
        policy_count += 1;
        return McpAuthOpStatus.ok;
    }

    /// Insert or update the scope policy for `method_or_tool`.
    pub fn upsertPolicy(
        *_,
        method_or_tool: []const u8,
        required_scopes: u64,
        require_all: bool,
        expected_audience: []const u8,
    ): McpAuthOpStatus {
        if (method_or_tool.len == 0 or method_or_tool.len > MCP_AUTH_METHOD_MAX_LEN) {
            return McpAuthOpStatus.invalid_input;
        }
        if (!isSafeHeaderParamValue(method_or_tool)) {
            return McpAuthOpStatus.invalid_input;
        }
        if (expected_audience.len > 0 and !isValidHeaderUri(expected_audience)) {
            return McpAuthOpStatus.invalid_input;
        }

        if (slot := findPolicy(method_or_tool)) {
            policies[slot].required_scopes = required_scopes;
            policies[slot].require_all = require_all;
            if (expected_audience.len > 0) {
                policies[slot].audience_len = copyIntoBuf(policies[slot].audience_buf[0..], expected_audience);
            } else {
                policies[slot].audience_len = 0;
            }
            return McpAuthOpStatus.ok;
        }

        return addPolicy(method_or_tool, required_scopes, require_all, expected_audience);
    }

    /// Remove an existing policy by MCP method/tool name.
    pub fn removePolicy(*_, method_or_tool: []const u8): McpAuthOpStatus {
        if (slot := findPolicy(method_or_tool)) {
            policies[slot] = McpScopePolicy{};
            if (policy_count > 0) policy_count -= 1;
            return McpAuthOpStatus.ok;
        }
        return McpAuthOpStatus.not_found;
    }

    /// Clear all registered method/tool policies (keeps registered scopes and stats).
    pub fn clearPolicies(*_) {
        let i: usize = 0;
        while (i < MCP_AUTH_MAX_POLICIES) : (i += 1) {
            policies[i] = McpScopePolicy{};
        }
        policy_count = 0;
    }

    /// Authorize an MCP method/tool call against the registered OAuth 2.1 policy table:
    ///   1. Policy lookup (`unknown_policy` -> 403 fail-closed)
    ///   2. Token presence (`missing_token` -> 401)
    ///   3. Strict audience binding `aud == expected_audience` (`invalid_audience` -> 401)
    ///   4. Overflow-safe `nbf` / `exp` validity window with `clock_skew_s` (`token_not_yet_valid` / `token_expired` -> 401)
    ///   5. Scope bitmask coverage (`insufficient_scope` -> 403 with exact `missing_scopes`)
    pub fn authorizeCall(
        *_,
        method_or_tool: []const u8,
        token_claims: McpTokenClaims,
        now_epoch_s: i64,
    ): McpAuthDecision {
        total_checks += 1;

        const slot_opt = findPolicy(method_or_tool);
        if (slot_opt == null) {
            forbidden_403_count += 1;
            unknown_policy_count += 1;
            return McpAuthDecision{
                status: McpAuthStatus.unknown_policy,
                allowed: false,
                http_status: 403,
                required_scopes: 0,
                granted_scopes: 0,
                missing_scopes: 0,
                matched_slot: 0,
                has_policy: false,
            };
        }

        const slot = slot_opt ?? 0;
        const req_scopes = policies[slot].required_scopes;
        const require_all = policies[slot].require_all;
        const expected_aud = if (policies[slot].audience_len > 0)
            policies[slot].expectedAudienceSlice()
        else
            defaultAudience();

        if (!token_claims.present) {
            unauthorized_401_count += 1;
            missing_token_count += 1;
            return McpAuthDecision{
                status: McpAuthStatus.missing_token,
                allowed: false,
                http_status: 401,
                required_scopes: req_scopes,
                granted_scopes: 0,
                missing_scopes: req_scopes,
                matched_slot: slot,
                has_policy: true,
            };
        }

        // Strict audience binding prevents Confused-Deputy token passthrough across MCP servers.
        if (expected_aud.len == 0 or token_claims.audience.len == 0 or !bytesEq(token_claims.audience, expected_aud)) {
            unauthorized_401_count += 1;
            invalid_audience_count += 1;
            return McpAuthDecision{
                status: McpAuthStatus.invalid_audience,
                allowed: false,
                http_status: 401,
                required_scopes: req_scopes,
                granted_scopes: 0,
                missing_scopes: req_scopes,
                matched_slot: slot,
                has_policy: true,
            };
        }

        // Not-before (`nbf`) check with clock skew tolerance.
        if (isTokenNotYetValidAt(now_epoch_s, token_claims.nbf_epoch_s, clock_skew_s)) {
            unauthorized_401_count += 1;
            token_not_yet_valid_count += 1;
            return McpAuthDecision{
                status: McpAuthStatus.token_not_yet_valid,
                allowed: false,
                http_status: 401,
                required_scopes: req_scopes,
                granted_scopes: 0,
                missing_scopes: req_scopes,
                matched_slot: slot,
                has_policy: true,
            };
        }

        // Expiration (`exp`) check with clock skew tolerance.
        if (isTokenExpiredAt(now_epoch_s, token_claims.exp_epoch_s, clock_skew_s)) {
            unauthorized_401_count += 1;
            token_expired_count += 1;
            return McpAuthDecision{
                status: McpAuthStatus.token_expired,
                allowed: false,
                http_status: 401,
                required_scopes: req_scopes,
                granted_scopes: 0,
                missing_scopes: req_scopes,
                matched_slot: slot,
                has_policy: true,
            };
        }

        // Combine pre-parsed mask with space-delimited RFC 6749 `scope` string.
        const granted = token_claims.granted_mask | parseScopesToMask(token_claims.scope_str);

        if (req_scopes == 0) {
            allowed_count += 1;
            return McpAuthDecision{
                status: McpAuthStatus.allowed,
                allowed: true,
                http_status: 200,
                required_scopes: 0,
                granted_scopes: granted,
                missing_scopes: 0,
                matched_slot: slot,
                has_policy: true,
            };
        }

        if (require_all) {
            const missing = req_scopes & (~granted);
            if (missing == 0) {
                allowed_count += 1;
                return McpAuthDecision{
                    status: McpAuthStatus.allowed,
                    allowed: true,
                    http_status: 200,
                    required_scopes: req_scopes,
                    granted_scopes: granted,
                    missing_scopes: 0,
                    matched_slot: slot,
                    has_policy: true,
                };
            }
            forbidden_403_count += 1;
            insufficient_scope_count += 1;
            return McpAuthDecision{
                status: McpAuthStatus.insufficient_scope,
                allowed: false,
                http_status: 403,
                required_scopes: req_scopes,
                granted_scopes: granted,
                missing_scopes: missing,
                matched_slot: slot,
                has_policy: true,
            };
        }

        // ANY-scope mode: at least one required scope bit must be present.
        if ((granted & req_scopes) != 0) {
            allowed_count += 1;
            return McpAuthDecision{
                status: McpAuthStatus.allowed,
                allowed: true,
                http_status: 200,
                required_scopes: req_scopes,
                granted_scopes: granted,
                missing_scopes: 0,
                matched_slot: slot,
                has_policy: true,
            };
        }

        forbidden_403_count += 1;
        insufficient_scope_count += 1;
        return McpAuthDecision{
            status: McpAuthStatus.insufficient_scope,
            allowed: false,
            http_status: 403,
            required_scopes: req_scopes,
            granted_scopes: granted,
            missing_scopes: req_scopes,
            matched_slot: slot,
            has_policy: true,
        };
    }

    /// Format an RFC 9728 / RFC 6750 `WWW-Authenticate` header directly from an `McpAuthDecision`.
    /// Returns `null` when `decision.allowed` is true or if formatting/buffer bounds fail.
    pub fn formatChallengeForDecision(decision: McpAuthDecision, out_buf: *[]u8): usize? {
        if (decision.allowed) return null;

        let scope_buf: [512]u8 = [0; 512];
        let scope_s: []u8 = scope_buf[0..];
        let scope_len: usize = 0;

        const target_mask = if (decision.missing_scopes != 0)
            decision.missing_scopes
        else
            decision.required_scopes;

        if (target_mask != 0) {
            if (n := formatScopeMask(target_mask, *scope_s)) {
                scope_len = n;
            } else {
                return null;
            }
        }

        const meta_url = resourceMetadataUrl();
        const scope_slice = scope_buf[0..scope_len];

        if (decision.status == McpAuthStatus.missing_token) {
            return formatWwwAuthenticateHeader(meta_url, "", "", scope_slice, *out_buf);
        }
        if (decision.status == McpAuthStatus.invalid_audience) {
            return formatWwwAuthenticateHeader(
                meta_url,
                "invalid_token",
                "token audience mismatch",
                "",
                *out_buf,
            );
        }
        if (decision.status == McpAuthStatus.token_expired) {
            return formatWwwAuthenticateHeader(
                meta_url,
                "invalid_token",
                "access token expired",
                "",
                *out_buf,
            );
        }
        if (decision.status == McpAuthStatus.token_not_yet_valid) {
            return formatWwwAuthenticateHeader(
                meta_url,
                "invalid_token",
                "access token not yet valid",
                "",
                *out_buf,
            );
        }
        if (decision.status == McpAuthStatus.insufficient_scope) {
            return formatWwwAuthenticateHeader(
                meta_url,
                "insufficient_scope",
                "insufficient scope for MCP operation",
                scope_slice,
                *out_buf,
            );
        }
        return formatWwwAuthenticateHeader(
            meta_url,
            "insufficient_scope",
            "unknown MCP method policy",
            "",
            *out_buf,
        );
    }

    /// Return a zero-heap global telemetry snapshot.
    pub fn stats(): McpAuthScopeStats {
        return McpAuthScopeStats{
            registered_scopes: scope_count,
            active_policies: policy_count,
            policy_capacity: MCP_AUTH_MAX_POLICIES,
            total_checks: total_checks,
            allowed_count: allowed_count,
            unauthorized_401_count: unauthorized_401_count,
            forbidden_403_count: forbidden_403_count,
            missing_token_count: missing_token_count,
            invalid_audience_count: invalid_audience_count,
            token_expired_count: token_expired_count,
            token_not_yet_valid_count: token_not_yet_valid_count,
            insufficient_scope_count: insufficient_scope_count,
            unknown_policy_count: unknown_policy_count,
        };
    }

    /// Reset all telemetry counters to zero while preserving scopes and policies.
    pub fn resetStats(*_) {
        total_checks = 0;
        allowed_count = 0;
        unauthorized_401_count = 0;
        forbidden_403_count = 0;
        missing_token_count = 0;
        invalid_audience_count = 0;
        token_expired_count = 0;
        token_not_yet_valid_count = 0;
        insufficient_scope_count = 0;
        unknown_policy_count = 0;
    }
}
