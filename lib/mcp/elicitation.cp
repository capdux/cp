// Origin: Pure-CP Stdlib (Zero-Alloc MCP 2025-06-18 Human-in-the-Loop Elicitation & Flat Schema Gateway) | Created: 2026-11-23
// Pure CP zero-heap-allocation MCP 2025-06-18 `elicitation/create` gateway:
//   - Flat primitive JSON Schema (string/number/integer/boolean; no nested objects per MCP spec)
//   - Required flags, string length bounds, email/uri/date format validators, numeric bounds & enum choices
//   - Bounded ElicitationHub state machine (pending -> accept/decline/cancel/timed_out) with strict
//     schema validation on accept, replay protection & timeout reaping
//   - Zero-alloc JSON-RPC 2.0 `elicitation/create` serializer & response summary formatter

const std = @import("std");

// ============================================================================
// 1. Constants & Primitive Parsers / Format Validators
// ============================================================================

/// Maximum concurrent Elicitation sessions tracked inline (zero heap).
pub const MCP_ELICIT_MAX_SESSIONS: usize = 32;

/// Maximum flat schema fields per Elicitation request.
pub const MCP_ELICIT_MAX_FIELDS: usize = 8;

/// Maximum enum choices per string field.
pub const MCP_ELICIT_MAX_ENUM_VALS: usize = 8;

/// Maximum field name byte length.
pub const MCP_ELICIT_NAME_MAX_LEN: usize = 48;

/// Maximum human-facing prompt message byte length.
pub const MCP_ELICIT_MSG_MAX_LEN: usize = 160;

const MCP_ELICIT_ENUM_MAX_LEN: usize = 32;
const MCP_ELICIT_VAL_MAX_LEN: usize = 128;

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn isAlpha(c: u8): bool {
    return (c >= 0x41 && c <= 0x5A) || (c >= 0x61 && c <= 0x7A);
}

fn isDigit(c: u8): bool {
    return c >= 0x30 && c <= 0x39;
}

/// Validates basic RFC-style email syntax (`local@domain.tld`, no whitespace/controls).
pub fn elicitValidateEmail(s: []const u8): bool {
    if (s.len < 5 || s.len > MCP_ELICIT_VAL_MAX_LEN) return false;
    let at_idx: usize = s.len;
    let at_count: usize = 0;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        const c: u8 = s[i];
        if (c <= 0x20 || c >= 0x7F) return false;
        if (c == 0x40) { // '@'
            at_idx = i;
            at_count += 1;
        }
    }
    if (at_count != 1 || at_idx == 0 || at_idx >= s.len - 3) return false;
    const domain = s[at_idx + 1 ..];
    if (domain[0] == 0x2E || domain[domain.len - 1] == 0x2E) return false;
    let has_dot: bool = false;
    let prev_dot: bool = false;
    let d: usize = 0;
    while (d < domain.len) : (d += 1) {
        if (domain[d] == 0x2E) {
            if (prev_dot) return false;
            has_dot = true;
            prev_dot = true;
        } else {
            prev_dot = false;
        }
    }
    return has_dot;
}

/// Validates basic RFC 3986 URI syntax (`scheme:rest`, valid scheme, no whitespace/controls).
pub fn elicitValidateUri(s: []const u8): bool {
    if (s.len < 3 || s.len > MCP_ELICIT_VAL_MAX_LEN) return false;
    if (!isAlpha(s[0])) return false;

    let colon_idx: usize = s.len;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        const c: u8 = s[i];
        if (c <= 0x20 || c >= 0x7F) return false;
        if (colon_idx == s.len) {
            if (c == 0x3A) { // ':'
                colon_idx = i;
            } else if (!isAlpha(c) && !isDigit(c) && c != 0x2B && c != 0x2D && c != 0x2E) {
                return false;
            }
        }
    }
    return colon_idx >= 1 && colon_idx + 1 < s.len;
}

/// Validates strict ISO 8601 calendar date `YYYY-MM-DD` (including month/day range check).
pub fn elicitValidateDate(s: []const u8): bool {
    if (s.len != 10) return false;
    if (s[4] != 0x2D || s[7] != 0x2D) return false;
    let i: usize = 0;
    while (i < 10) : (i += 1) {
        if (i == 4 || i == 7) continue;
        if (!isDigit(s[i])) return false;
    }
    const year: u32 = ((s[0] - 0x30) as u32) * 1000 +
        ((s[1] - 0x30) as u32) * 100 +
        ((s[2] - 0x30) as u32) * 10 +
        ((s[3] - 0x30) as u32);
    const month: u32 = ((s[5] - 0x30) as u32) * 10 + ((s[6] - 0x30) as u32);
    const day: u32 = ((s[8] - 0x30) as u32) * 10 + ((s[9] - 0x30) as u32);
    if (month < 1 || month > 12 || day < 1) return false;

    let max_day: u32 = 31;
    if (month == 4 || month == 6 || month == 9 || month == 11) {
        max_day = 30;
    } else if (month == 2) {
        const is_leap: bool = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
        max_day = if (is_leap) 29 else 28;
    }
    return day <= max_day;
}

fn parseSignedInt64(s: []const u8): i64? {
    if (s.len == 0) return null;
    let neg: bool = false;
    let pos: usize = 0;
    if (s[0] == 0x2D) { // '-'
        neg = true;
        pos = 1;
    } else if (s[0] == 0x2B) { // '+'
        pos = 1;
    }
    if (pos >= s.len) return null;

    let acc: u64 = 0;
    while (pos < s.len) : (pos += 1) {
        if (!isDigit(s[pos])) return null;
        const d: u64 = (s[pos] - 0x30) as u64;
        if (acc > (9223372036854775807 - d) / 10) return null;
        acc = acc * 10 + d;
    }
    const signed: i64 = acc as i64;
    return if (neg) (0 - signed) else signed;
}

fn parseDecimalF64(s: []const u8): f64? {
    if (s.len == 0) return null;
    let neg: bool = false;
    let pos: usize = 0;
    if (s[0] == 0x2D) {
        neg = true;
        pos = 1;
    } else if (s[0] == 0x2B) {
        pos = 1;
    }
    if (pos >= s.len) return null;

    let int_part: f64 = 0.0;
    let frac_part: f64 = 0.0;
    let frac_div: f64 = 1.0;
    let seen_dot: bool = false;
    let seen_digit: bool = false;

    while (pos < s.len) : (pos += 1) {
        const c: u8 = s[pos];
        if (c == 0x2E) {
            if (seen_dot) return null;
            seen_dot = true;
        } else if (isDigit(c)) {
            seen_digit = true;
            const d: f64 = (c - 0x30) as f64;
            if (!seen_dot) {
                int_part = int_part * 10.0 + d;
            } else {
                frac_div *= 10.0;
                frac_part += d / frac_div;
            }
        } else {
            return null;
        }
    }
    if (!seen_digit) return null;
    const total: f64 = int_part + frac_part;
    if (!std.math.isFinite(total)) return null;
    return if (neg) (0.0 - total) else total;
}

// ============================================================================
// 2. Public Schema, Session & Telemetry Models
// ============================================================================

/// Primitive JSON Schema types permitted by MCP 2025-06-18 flat Elicitation schema.
pub enum ElicitFieldType {
    string_val,
    number_val,
    integer_val,
    boolean_val,
}

/// Optional string format constraint for `string_val` fields.
pub enum ElicitStringFormat {
    none,
    email,
    uri,
    date,
}

/// User decision action in an MCP Elicitation response.
pub enum ElicitAction {
    accept,
    decline,
    cancel,
}

/// Lifecycle state of an Elicitation session.
pub enum ElicitSessionState {
    pending,
    accepted,
    declined,
    cancelled,
    timed_out,
}

/// Validation & hub operation status codes.
pub enum ElicitStatus {
    ok,
    not_found,
    already_resolved,
    session_expired,
    capacity_full,
    duplicate_request_id,
    invalid_schema,
    missing_required_field,
    unknown_field,
    duplicate_field,
    type_mismatch,
    length_out_of_bounds,
    format_invalid,
    number_out_of_bounds,
    enum_mismatch,
}

/// Caller-supplied flat primitive field descriptor for `createRequest` and `encodeElicitationRequest`.
pub struct ElicitFieldSpec {
    name: []const u8 = "";
    field_type: ElicitFieldType = ElicitFieldType.string_val;
    required: bool = false;
    min_len: usize = 0;
    max_len: usize = 0;
    string_format: ElicitStringFormat = ElicitStringFormat.none;
    has_min_num: bool = false;
    min_num: f64 = 0.0;
    has_max_num: bool = false;
    max_num: f64 = 0.0;
    enum_values: []const []const u8 = [];

    /// Constructs a string field specification.
    static fn stringField(
        name: []const u8,
        required: bool,
        min_len: usize,
        max_len: usize,
        fmt: ElicitStringFormat,
    ): ElicitFieldSpec {
        return ElicitFieldSpec{
            name: name,
            field_type: ElicitFieldType.string_val,
            required: required,
            min_len: min_len,
            max_len: max_len,
            string_format: fmt,
        };
    }

    /// Constructs a string enum field specification.
    static fn enumField(
        name: []const u8,
        required: bool,
        choices: []const []const u8,
    ): ElicitFieldSpec {
        return ElicitFieldSpec{
            name: name,
            field_type: ElicitFieldType.string_val,
            required: required,
            enum_values: choices,
        };
    }

    /// Constructs an integer field specification with optional inclusive `[min_v, max_v]` bounds.
    static fn integerField(
        name: []const u8,
        required: bool,
        has_min: bool,
        min_v: i64,
        has_max: bool,
        max_v: i64,
    ): ElicitFieldSpec {
        return ElicitFieldSpec{
            name: name,
            field_type: ElicitFieldType.integer_val,
            required: required,
            has_min_num: has_min,
            min_num: min_v as f64,
            has_max_num: has_max,
            max_num: max_v as f64,
        };
    }

    /// Constructs a number (f64) field specification with optional inclusive `[min_v, max_v]` bounds.
    static fn numberField(
        name: []const u8,
        required: bool,
        has_min: bool,
        min_v: f64,
        has_max: bool,
        max_v: f64,
    ): ElicitFieldSpec {
        return ElicitFieldSpec{
            name: name,
            field_type: ElicitFieldType.number_val,
            required: required,
            has_min_num: has_min,
            min_num: min_v,
            has_max_num: has_max,
            max_num: max_v,
        };
    }

    /// Constructs a boolean field specification.
    static fn booleanField(name: []const u8, required: bool): ElicitFieldSpec {
        return ElicitFieldSpec{
            name: name,
            field_type: ElicitFieldType.boolean_val,
            required: required,
        };
    }
}

/// User-submitted field name and serialized primitive value for `respondAccept`.
pub struct ElicitFieldValue {
    name: []const u8 = "";
    raw_value: []const u8 = "";
}

/// Outcome returned by `respondAccept`, `respondDecline`, and `respondCancel`.
pub struct ElicitResponseOutcome {
    status: ElicitStatus = ElicitStatus.ok;
    state: ElicitSessionState = ElicitSessionState.pending;
    failed_field_index: usize = 0;
    has_failed_field: bool = false;
}

/// Telemetry snapshot returned by `ElicitationHub.stats()`.
pub struct ElicitHubStats {
    active_pending: usize = 0;
    total_created: u64 = 0;
    accepted_count: u64 = 0;
    declined_count: u64 = 0;
    cancelled_count: u64 = 0;
    timed_out_count: u64 = 0;
    validation_failures: u64 = 0;
    replay_rejections: u64 = 0;
}

// ============================================================================
// 3. Internal Inline Storage & Schema Validator
// ============================================================================

struct StoredEnumChoice {
    bytes: [32]u8 = [0; 32];
    len: usize = 0;
}

struct StoredElicitField {
    name: [48]u8 = [0; 48];
    name_len: usize = 0;
    field_type: ElicitFieldType = ElicitFieldType.string_val;
    required: bool = false;
    min_len: usize = 0;
    max_len: usize = 0;
    string_format: ElicitStringFormat = ElicitStringFormat.none;
    has_min_num: bool = false;
    min_num: f64 = 0.0;
    has_max_num: bool = false;
    max_num: f64 = 0.0;
    enum_vals: [8]StoredEnumChoice = [StoredEnumChoice{}; 8];
    enum_count: usize = 0;
}

struct ElicitSessionSlot {
    request_id: u64 = 0;
    message: [160]u8 = [0; 160];
    msg_len: usize = 0;
    fields: [8]StoredElicitField = [StoredElicitField{}; 8];
    field_count: usize = 0;
    state: ElicitSessionState = ElicitSessionState.pending;
    created_ms: u64 = 0;
    expires_ms: u64 = 0;
    occupied: bool = false;
}

fn validateStoredFieldValue(field: StoredElicitField, raw: []const u8): ElicitStatus {
    if (field.field_type == ElicitFieldType.boolean_val) {
        if (bytesEq(raw, "true") || bytesEq(raw, "false")) return ElicitStatus.ok;
        return ElicitStatus.type_mismatch;
    }

    if (field.field_type == ElicitFieldType.integer_val) {
        const iv = parseSignedInt64(raw) orelse return ElicitStatus.type_mismatch;
        const fv: f64 = iv as f64;
        if (field.has_min_num && fv < field.min_num) return ElicitStatus.number_out_of_bounds;
        if (field.has_max_num && fv > field.max_num) return ElicitStatus.number_out_of_bounds;
        return ElicitStatus.ok;
    }

    if (field.field_type == ElicitFieldType.number_val) {
        const fv = parseDecimalF64(raw) orelse return ElicitStatus.type_mismatch;
        if (field.has_min_num && fv < field.min_num) return ElicitStatus.number_out_of_bounds;
        if (field.has_max_num && fv > field.max_num) return ElicitStatus.number_out_of_bounds;
        return ElicitStatus.ok;
    }

    // string_val
    if (raw.len > MCP_ELICIT_VAL_MAX_LEN) return ElicitStatus.length_out_of_bounds;
    if (raw.len < field.min_len) return ElicitStatus.length_out_of_bounds;
    if (field.max_len > 0 && raw.len > field.max_len) return ElicitStatus.length_out_of_bounds;

    if (field.enum_count > 0) {
        let matched: bool = false;
        let e: usize = 0;
        while (e < field.enum_count) : (e += 1) {
            if (bytesEq(raw, field.enum_vals[e].bytes[0..field.enum_vals[e].len])) {
                matched = true;
                break;
            }
        }
        if (!matched) return ElicitStatus.enum_mismatch;
    }

    if (field.string_format == ElicitStringFormat.email && !elicitValidateEmail(raw)) {
        return ElicitStatus.format_invalid;
    }
    if (field.string_format == ElicitStringFormat.uri && !elicitValidateUri(raw)) {
        return ElicitStatus.format_invalid;
    }
    if (field.string_format == ElicitStringFormat.date && !elicitValidateDate(raw)) {
        return ElicitStatus.format_invalid;
    }

    return ElicitStatus.ok;
}

// ============================================================================
// 4. Zero-Alloc ElicitationHub Session State Machine
// ============================================================================

/// Zero-heap-allocation MCP 2025-06-18 human-in-the-loop Elicitation session gateway.
pub struct ElicitationHub {
    default_timeout_ms: u64 = 60000,
    slots: [32]ElicitSessionSlot = [ElicitSessionSlot{}; 32],
    active_pending: usize = 0,
    total_created: u64 = 0,
    accepted_count: u64 = 0,
    declined_count: u64 = 0,
    cancelled_count: u64 = 0,
    timed_out_count: u64 = 0,
    validation_failures: u64 = 0,
    replay_rejections: u64 = 0;

    /// Initializes an `ElicitationHub` with `default_timeout_ms` (`0` disables default timeout).
    static fn init(default_timeout_ms: u64): ElicitationHub {
        return ElicitationHub{
            default_timeout_ms: default_timeout_ms,
        };
    }

    fn findSlot(request_id: u64): usize? {
        let i: usize = 0;
        while (i < MCP_ELICIT_MAX_SESSIONS) : (i += 1) {
            if (_.slots[i].occupied && _.slots[i].request_id == request_id) return i;
        }
        return null;
    }

    fn findFreeSlot(): usize? {
        let i: usize = 0;
        while (i < MCP_ELICIT_MAX_SESSIONS) : (i += 1) {
            if (!_.slots[i].occupied) return i;
        }
        return null;
    }

    /// Registers a new pending Elicitation session with a validated flat primitive schema.
    fn createRequest(
        *_,
        request_id: u64,
        message: []const u8,
        fields: []const ElicitFieldSpec,
        timeout_ms: u64,
        now_ms: u64,
    ): ElicitStatus {
        if (request_id == 0) return ElicitStatus.invalid_schema;
        if (message.len == 0 || message.len > MCP_ELICIT_MSG_MAX_LEN) return ElicitStatus.invalid_schema;
        if (fields.len == 0 || fields.len > MCP_ELICIT_MAX_FIELDS) return ElicitStatus.invalid_schema;
        if (_.findSlot(request_id) != null) return ElicitStatus.duplicate_request_id;

        // Validate every field spec and check duplicate field names.
        let f: usize = 0;
        while (f < fields.len) : (f += 1) {
            const spec = fields[f];
            if (spec.name.len == 0 || spec.name.len > MCP_ELICIT_NAME_MAX_LEN) return ElicitStatus.invalid_schema;
            if (spec.max_len > 0 && spec.min_len > spec.max_len) return ElicitStatus.invalid_schema;
            if (spec.has_min_num && !std.math.isFinite(spec.min_num)) return ElicitStatus.invalid_schema;
            if (spec.has_max_num && !std.math.isFinite(spec.max_num)) return ElicitStatus.invalid_schema;
            if (spec.has_min_num && spec.has_max_num && spec.min_num > spec.max_num) return ElicitStatus.invalid_schema;
            if (spec.enum_values.len > MCP_ELICIT_MAX_ENUM_VALS) return ElicitStatus.invalid_schema;
            let e: usize = 0;
            while (e < spec.enum_values.len) : (e += 1) {
                if (spec.enum_values[e].len == 0 || spec.enum_values[e].len > MCP_ELICIT_ENUM_MAX_LEN) {
                    return ElicitStatus.invalid_schema;
                }
            }
            let prev: usize = 0;
            while (prev < f) : (prev += 1) {
                if (bytesEq(fields[prev].name, spec.name)) return ElicitStatus.invalid_schema;
            }
        }

        const slot_idx = _.findFreeSlot() orelse return ElicitStatus.capacity_full;

        let slot = ElicitSessionSlot{
            request_id: request_id,
            msg_len: message.len,
            field_count: fields.len,
            state: ElicitSessionState.pending,
            created_ms: now_ms,
            occupied: true,
        };
        const eff_timeout: u64 = if (timeout_ms > 0) timeout_ms else _.default_timeout_ms;
        slot.expires_ms = if (eff_timeout > 0) (now_ms + eff_timeout) else 0;

        let m: usize = 0;
        while (m < message.len) : (m += 1) {
            slot.message[m] = message[m];
        }

        f = 0;
        while (f < fields.len) : (f += 1) {
            const spec = fields[f];
            let sf = StoredElicitField{
                name_len: spec.name.len,
                field_type: spec.field_type,
                required: spec.required,
                min_len: spec.min_len,
                max_len: spec.max_len,
                string_format: spec.string_format,
                has_min_num: spec.has_min_num,
                min_num: spec.min_num,
                has_max_num: spec.has_max_num,
                max_num: spec.max_num,
                enum_count: spec.enum_values.len,
            };
            let ni: usize = 0;
            while (ni < spec.name.len) : (ni += 1) {
                sf.name[ni] = spec.name[ni];
            }
            let ei: usize = 0;
            while (ei < spec.enum_values.len) : (ei += 1) {
                const choice = spec.enum_values[ei];
                let ch = StoredEnumChoice{ len: choice.len };
                let ci: usize = 0;
                while (ci < choice.len) : (ci += 1) {
                    ch.bytes[ci] = choice[ci];
                }
                sf.enum_vals[ei] = ch;
            }
            slot.fields[f] = sf;
        }

        _.slots[slot_idx] = slot;
        _.active_pending += 1;
        _.total_created += 1;
        return ElicitStatus.ok;
    }

    /// Processes a user `accept` response, strictly validating all submitted values against
    /// the declared flat schema. On schema violation, keeps the session `pending` and reports
    /// exact `status` + `failed_field_index`.
    fn respondAccept(
        *_,
        request_id: u64,
        values: []const ElicitFieldValue,
        now_ms: u64,
    ): ElicitResponseOutcome {
        const idx = _.findSlot(request_id) orelse {
            return ElicitResponseOutcome{ status: ElicitStatus.not_found };
        };

        if (_.slots[idx].state != ElicitSessionState.pending) {
            _.replay_rejections += 1;
            return ElicitResponseOutcome{
                status: ElicitStatus.already_resolved,
                state: _.slots[idx].state,
            };
        }

        if (_.slots[idx].expires_ms != 0 && now_ms >= _.slots[idx].expires_ms) {
            _.slots[idx].state = ElicitSessionState.timed_out;
            if (_.active_pending > 0) _.active_pending -= 1;
            _.timed_out_count += 1;
            return ElicitResponseOutcome{
                status: ElicitStatus.session_expired,
                state: ElicitSessionState.timed_out,
            };
        }

        const n_fields: usize = _.slots[idx].field_count;
        let seen_field: [8]bool = [false; 8];

        // 1. Check unknown/duplicate submitted fields and validate each value against its schema field.
        let v: usize = 0;
        while (v < values.len) : (v += 1) {
            const val = values[v];
            let matched_f: usize = n_fields;
            let f: usize = 0;
            while (f < n_fields) : (f += 1) {
                const fname = _.slots[idx].fields[f].name[0.._.slots[idx].fields[f].name_len];
                if (bytesEq(val.name, fname)) {
                    matched_f = f;
                    break;
                }
            }
            if (matched_f == n_fields) {
                _.validation_failures += 1;
                return ElicitResponseOutcome{
                    status: ElicitStatus.unknown_field,
                    state: ElicitSessionState.pending,
                    failed_field_index: v,
                    has_failed_field: true,
                };
            }
            if (seen_field[matched_f]) {
                _.validation_failures += 1;
                return ElicitResponseOutcome{
                    status: ElicitStatus.duplicate_field,
                    state: ElicitSessionState.pending,
                    failed_field_index: matched_f,
                    has_failed_field: true,
                };
            }
            seen_field[matched_f] = true;

            const vst = validateStoredFieldValue(_.slots[idx].fields[matched_f], val.raw_value);
            if (vst != ElicitStatus.ok) {
                _.validation_failures += 1;
                return ElicitResponseOutcome{
                    status: vst,
                    state: ElicitSessionState.pending,
                    failed_field_index: matched_f,
                    has_failed_field: true,
                };
            }
        }

        // 2. Verify all required fields were provided.
        let f: usize = 0;
        while (f < n_fields) : (f += 1) {
            if (_.slots[idx].fields[f].required && !seen_field[f]) {
                _.validation_failures += 1;
                return ElicitResponseOutcome{
                    status: ElicitStatus.missing_required_field,
                    state: ElicitSessionState.pending,
                    failed_field_index: f,
                    has_failed_field: true,
                };
            }
        }

        _.slots[idx].state = ElicitSessionState.accepted;
        if (_.active_pending > 0) _.active_pending -= 1;
        _.accepted_count += 1;
        return ElicitResponseOutcome{
            status: ElicitStatus.ok,
            state: ElicitSessionState.accepted,
        };
    }

    fn resolveNonAccept(*_, request_id: u64, target_state: ElicitSessionState, now_ms: u64): ElicitResponseOutcome {
        const idx = _.findSlot(request_id) orelse {
            return ElicitResponseOutcome{ status: ElicitStatus.not_found };
        };
        if (_.slots[idx].state != ElicitSessionState.pending) {
            _.replay_rejections += 1;
            return ElicitResponseOutcome{
                status: ElicitStatus.already_resolved,
                state: _.slots[idx].state,
            };
        }
        if (_.slots[idx].expires_ms != 0 && now_ms >= _.slots[idx].expires_ms) {
            _.slots[idx].state = ElicitSessionState.timed_out;
            if (_.active_pending > 0) _.active_pending -= 1;
            _.timed_out_count += 1;
            return ElicitResponseOutcome{
                status: ElicitStatus.session_expired,
                state: ElicitSessionState.timed_out,
            };
        }

        _.slots[idx].state = target_state;
        if (_.active_pending > 0) _.active_pending -= 1;
        if (target_state == ElicitSessionState.declined) {
            _.declined_count += 1;
        } else {
            _.cancelled_count += 1;
        }
        return ElicitResponseOutcome{
            status: ElicitStatus.ok,
            state: target_state,
        };
    }

    /// Records a user `decline` decision for a pending Elicitation session.
    fn respondDecline(*_, request_id: u64, now_ms: u64): ElicitResponseOutcome {
        return _.resolveNonAccept(request_id, ElicitSessionState.declined, now_ms);
    }

    /// Records a user `cancel` decision for a pending Elicitation session.
    fn respondCancel(*_, request_id: u64, now_ms: u64): ElicitResponseOutcome {
        return _.resolveNonAccept(request_id, ElicitSessionState.cancelled, now_ms);
    }

    /// Sweeps all pending sessions whose deadline `<= now_ms` into `timed_out`.
    fn reapExpired(*_, now_ms: u64): usize {
        let reaped: usize = 0;
        let i: usize = 0;
        while (i < MCP_ELICIT_MAX_SESSIONS) : (i += 1) {
            if (!_.slots[i].occupied || _.slots[i].state != ElicitSessionState.pending) continue;
            if (_.slots[i].expires_ms != 0 && now_ms >= _.slots[i].expires_ms) {
                _.slots[i].state = ElicitSessionState.timed_out;
                if (_.active_pending > 0) _.active_pending -= 1;
                _.timed_out_count += 1;
                reaped += 1;
            }
        }
        return reaped;
    }

    /// Reclaims all terminal (`accepted`/`declined`/`cancelled`/`timed_out`) slots for reuse.
    fn purgeTerminal(*_): usize {
        let purged: usize = 0;
        let i: usize = 0;
        while (i < MCP_ELICIT_MAX_SESSIONS) : (i += 1) {
            if (_.slots[i].occupied && _.slots[i].state != ElicitSessionState.pending) {
                _.slots[i] = ElicitSessionSlot{};
                purged += 1;
            }
        }
        return purged;
    }

    /// Queries the current lifecycle state of `request_id`, or `null` if not found.
    fn getSessionState(request_id: u64): ElicitSessionState? {
        const idx = _.findSlot(request_id) orelse return null;
        return _.slots[idx].state;
    }

    /// Returns a diagnostic telemetry snapshot.
    fn stats(): ElicitHubStats {
        return ElicitHubStats{
            active_pending: _.active_pending,
            total_created: _.total_created,
            accepted_count: _.accepted_count,
            declined_count: _.declined_count,
            cancelled_count: _.cancelled_count,
            timed_out_count: _.timed_out_count,
            validation_failures: _.validation_failures,
            replay_rejections: _.replay_rejections,
        };
    }
}

// ============================================================================
// 5. Zero-Alloc JSON-RPC 2.0 Elicitation Request & Response Summary Codecs
// ============================================================================

fn appendRaw(out: *[]u8, pos: *usize, s: []const u8): bool {
    if (pos.* + s.len > out.len) return false;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        out[pos.* + i] = s[i];
    }
    pos.* = pos.* + s.len;
    return true;
}

fn appendJsonEscaped(out: *[]u8, pos: *usize, s: []const u8): bool {
    if (!appendRaw(out, pos, "\"")) return false;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        const c: u8 = s[i];
        if (c == 0x22) {
            if (!appendRaw(out, pos, "\\\"")) return false;
        } else if (c == 0x5C) {
            if (!appendRaw(out, pos, "\\\\")) return false;
        } else if (c == 0x0A) {
            if (!appendRaw(out, pos, "\\n")) return false;
        } else if (c == 0x0D) {
            if (!appendRaw(out, pos, "\\r")) return false;
        } else if (c == 0x09) {
            if (!appendRaw(out, pos, "\\t")) return false;
        } else {
            if (pos.* >= out.len) return false;
            out[pos.*] = c;
            pos.* = pos.* + 1;
        }
    }
    return appendRaw(out, pos, "\"");
}

fn appendU64(out: *[]u8, pos: *usize, val_in: u64): bool {
    if (val_in == 0) return appendRaw(out, pos, "0");
    let tmp: [24]u8 = [0; 24];
    let n: usize = 0;
    let v: u64 = val_in;
    while (v > 0) {
        tmp[n] = (0x30 as u8) + ((v % 10) as u8);
        v = v / 10;
        n += 1;
    }
    if (pos.* + n > out.len) return false;
    let i: usize = 0;
    while (i < n) : (i += 1) {
        out[pos.* + i] = tmp[n - 1 - i];
    }
    pos.* = pos.* + n;
    return true;
}

/// Encodes a complete MCP 2025-06-18 JSON-RPC 2.0 `elicitation/create` request into `out_buf`.
/// Returns total bytes written, or `null` if inputs are invalid or `out_buf` is too small.
pub fn encodeElicitationRequest(
    request_id: u64,
    message: []const u8,
    fields: []const ElicitFieldSpec,
    out_buf: *[]u8,
): usize? {
    if (request_id == 0 || message.len == 0 || fields.len == 0 || fields.len > MCP_ELICIT_MAX_FIELDS) {
        return null;
    }

    let pos: usize = 0;
    if (!appendRaw(out_buf, *pos, "{\"jsonrpc\":\"2.0\",\"id\":")) return null;
    if (!appendU64(out_buf, *pos, request_id)) return null;
    if (!appendRaw(out_buf, *pos, ",\"method\":\"elicitation/create\",\"params\":{\"message\":")) return null;
    if (!appendJsonEscaped(out_buf, *pos, message)) return null;
    if (!appendRaw(out_buf, *pos, ",\"requestedSchema\":{\"type\":\"object\",\"properties\":{")) return null;

    let f: usize = 0;
    while (f < fields.len) : (f += 1) {
        if (f > 0) {
            if (!appendRaw(out_buf, *pos, ",")) return null;
        }
        const spec = fields[f];
        if (!appendJsonEscaped(out_buf, *pos, spec.name)) return null;
        if (!appendRaw(out_buf, *pos, ":{\"type\":\"")) return null;
        if (spec.field_type == ElicitFieldType.string_val) {
            if (!appendRaw(out_buf, *pos, "string\"")) return null;
            if (spec.string_format == ElicitStringFormat.email) {
                if (!appendRaw(out_buf, *pos, ",\"format\":\"email\"")) return null;
            } else if (spec.string_format == ElicitStringFormat.uri) {
                if (!appendRaw(out_buf, *pos, ",\"format\":\"uri\"")) return null;
            } else if (spec.string_format == ElicitStringFormat.date) {
                if (!appendRaw(out_buf, *pos, ",\"format\":\"date\"")) return null;
            }
            if (spec.enum_values.len > 0) {
                if (!appendRaw(out_buf, *pos, ",\"enum\":[")) return null;
                let e: usize = 0;
                while (e < spec.enum_values.len) : (e += 1) {
                    if (e > 0) {
                        if (!appendRaw(out_buf, *pos, ",")) return null;
                    }
                    if (!appendJsonEscaped(out_buf, *pos, spec.enum_values[e])) return null;
                }
                if (!appendRaw(out_buf, *pos, "]")) return null;
            }
        } else if (spec.field_type == ElicitFieldType.integer_val) {
            if (!appendRaw(out_buf, *pos, "integer\"")) return null;
        } else if (spec.field_type == ElicitFieldType.number_val) {
            if (!appendRaw(out_buf, *pos, "number\"")) return null;
        } else {
            if (!appendRaw(out_buf, *pos, "boolean\"")) return null;
        }
        if (!appendRaw(out_buf, *pos, "}")) return null;
    }

    if (!appendRaw(out_buf, *pos, "},\"required\":[")) return null;
    let req_written: usize = 0;
    f = 0;
    while (f < fields.len) : (f += 1) {
        if (!fields[f].required) continue;
        if (req_written > 0) {
            if (!appendRaw(out_buf, *pos, ",")) return null;
        }
        if (!appendJsonEscaped(out_buf, *pos, fields[f].name)) return null;
        req_written += 1;
    }
    if (!appendRaw(out_buf, *pos, "]}}}")) return null;
    return pos;
}

/// Formats a compact JSON summary of an Elicitation user response into `out_buf`.
pub fn formatElicitationResponseSummary(
    action: ElicitAction,
    field_count: usize,
    out_buf: *[]u8,
): usize? {
    let pos: usize = 0;
    if (action == ElicitAction.accept) {
        if (!appendRaw(out_buf, *pos, "{\"action\":\"accept\",\"fieldCount\":")) return null;
        if (!appendU64(out_buf, *pos, field_count as u64)) return null;
        if (!appendRaw(out_buf, *pos, "}")) return null;
        return pos;
    }
    if (action == ElicitAction.decline) {
        if (!appendRaw(out_buf, *pos, "{\"action\":\"decline\"}")) return null;
        return pos;
    }
    if (!appendRaw(out_buf, *pos, "{\"action\":\"cancel\"}")) return null;
    return pos;
}
