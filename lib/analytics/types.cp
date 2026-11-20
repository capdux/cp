// Source: cp/lib/analytics/types.cp (Pure CP Runtime Telemetry Event, Property & Redaction Types)
// Zero external dependencies; deterministic ownership and automatic sensitive field redaction.

const std = @import("std");

pub const REDACTED_VALUE: []const u8 = "[REDACTED]";
pub const MAX_EVENT_NAME_LEN: usize = 128;
pub const MAX_PROP_KEY_LEN: usize = 64;
pub const MAX_PROPS_PER_EVENT: usize = 64;

pub err AnalyticsError {
    EmptyEventName,
    EventNameTooLong,
    EmptyPropKey,
    PropKeyTooLong,
    TooManyProps,
    InvalidConfig,
    BufferFull,
    OutOfMemory,
}

pub enum DropPolicy {
    DropOldest,
    DropNewest,
    RejectWhenFull,
}

pub enum PropKind {
    String,
    Int,
    Float,
    Bool,
}

pub struct PropValue {
    kind: PropKind = .String,
    str_val: []const u8 = "",
    int_val: i64 = 0,
    float_val: f64 = 0.0,
    bool_val: bool = false,

    pub fn fromString(s: []const u8): PropValue {
        return PropValue{
            kind: .String,
            str_val: s,
        };
    }

    pub fn fromInt(v: i64): PropValue {
        return PropValue{
            kind: .Int,
            int_val: v,
        };
    }

    pub fn fromFloat(v: f64): PropValue {
        return PropValue{
            kind: .Float,
            float_val: v,
        };
    }

    pub fn fromBool(v: bool): PropValue {
        return PropValue{
            kind: .Bool,
            bool_val: v,
        };
    }
}

pub struct EventProp {
    key: []const u8 = "",
    value: PropValue = PropValue{},

    pub fn str(key: []const u8, val: []const u8): EventProp {
        return EventProp{ key: key, value: PropValue.fromString(val) };
    }

    pub fn int(key: []const u8, val: i64): EventProp {
        return EventProp{ key: key, value: PropValue.fromInt(val) };
    }

    pub fn float(key: []const u8, val: f64): EventProp {
        return EventProp{ key: key, value: PropValue.fromFloat(val) };
    }

    pub fn boolean(key: []const u8, val: bool): EventProp {
        return EventProp{ key: key, value: PropValue.fromBool(val) };
    }
}

pub struct AnalyticsEvent {
    name: []const u8 = "",
    distinct_id: []const u8 = "",
    session_id: []const u8 = "",
    timestamp_ms: i64 = 0,
    props: []EventProp = [],
}

fn toLowerAscii(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
}

fn containsIgnoreCase(haystack: []const u8, needle: []const u8): bool {
    if (needle.len == 0) return true;
    if (haystack.len < needle.len) return false;
    let i: usize = 0;
    const limit = haystack.len - needle.len;
    while (i <= limit) : (i += 1) {
        let j: usize = 0;
        let matched = true;
        while (j < needle.len) : (j += 1) {
            if (toLowerAscii(haystack[i + j]) != toLowerAscii(needle[j])) {
                matched = false;
                break;
            }
        }
        if (matched) return true;
    }
    return false;
}

/// Returns true if the property key refers to sensitive credentials or secrets.
pub fn isSensitiveKey(key: []const u8): bool {
    if (containsIgnoreCase(key, "password")) return true;
    if (containsIgnoreCase(key, "passwd")) return true;
    if (containsIgnoreCase(key, "secret")) return true;
    if (containsIgnoreCase(key, "token")) return true;
    if (containsIgnoreCase(key, "api_key")) return true;
    if (containsIgnoreCase(key, "apikey")) return true;
    if (containsIgnoreCase(key, "private_key")) return true;
    if (containsIgnoreCase(key, "access_key")) return true;
    if (containsIgnoreCase(key, "credential")) return true;
    if (containsIgnoreCase(key, "authorization")) return true;
    return false;
}

pub fn validateEventName(name: []const u8): void! {
    if (name.len == 0) {
        return AnalyticsError.EmptyEventName;
    }
    if (name.len > MAX_EVENT_NAME_LEN) {
        return AnalyticsError.EventNameTooLong;
    }
}

pub fn validatePropKey(key: []const u8): void! {
    if (key.len == 0) {
        return AnalyticsError.EmptyPropKey;
    }
    if (key.len > MAX_PROP_KEY_LEN) {
        return AnalyticsError.PropKeyTooLong;
    }
}

fn dupOwnedStr(alloc: std.mem.Allocator, s: []const u8): ([]const u8)! {
    if (s.len == 0) return "";
    return alloc.dupe(u8, s) catch return AnalyticsError.OutOfMemory;
}

fn freeOwnedStr(alloc: std.mem.Allocator, s: []const u8) {
    if (s.len > 0) {
        alloc.free(s);
    }
}

pub fn sanitizeAndCloneProp(alloc: std.mem.Allocator, prop: EventProp): EventProp! {
    try validatePropKey(prop.key);
    const owned_key = try dupOwnedStr(alloc, prop.key);
    errdefer freeOwnedStr(alloc, owned_key);

    if (isSensitiveKey(prop.key)) {
        const redacted = try dupOwnedStr(alloc, REDACTED_VALUE);
        return EventProp{
            key: owned_key,
            value: PropValue.fromString(redacted),
        };
    }

    if (prop.value.kind == .String) {
        const owned_val = try dupOwnedStr(alloc, prop.value.str_val);
        return EventProp{
            key: owned_key,
            value: PropValue.fromString(owned_val),
        };
    }

    return EventProp{
        key: owned_key,
        value: prop.value,
    };
}

pub fn freeProp(alloc: std.mem.Allocator, prop: *EventProp) {
    freeOwnedStr(alloc, prop.key);
    if (prop.value.kind == .String) {
        freeOwnedStr(alloc, prop.value.str_val);
    }
    prop.key = "";
    prop.value = PropValue{};
}

pub fn cloneEventWithSanitization(
    alloc: std.mem.Allocator,
    name: []const u8,
    distinct_id: []const u8,
    session_id: []const u8,
    timestamp_ms: i64,
    props: []const EventProp,
): AnalyticsEvent! {
    try validateEventName(name);
    if (props.len > MAX_PROPS_PER_EVENT) {
        return AnalyticsError.TooManyProps;
    }

    const owned_name = try dupOwnedStr(alloc, name);
    errdefer freeOwnedStr(alloc, owned_name);

    const owned_distinct = try dupOwnedStr(alloc, distinct_id);
    errdefer freeOwnedStr(alloc, owned_distinct);

    const owned_session = try dupOwnedStr(alloc, session_id);
    errdefer freeOwnedStr(alloc, owned_session);

    let owned_props: []EventProp = [];
    let cloned_count: usize = 0;
    errdefer {
        let idx: usize = 0;
        while (idx < cloned_count) : (idx += 1) {
            freeProp(alloc, &owned_props[idx]);
        }
        if (owned_props.len > 0) {
            alloc.free(owned_props);
        }
    }

    if (props.len > 0) {
        owned_props = alloc.alloc(EventProp, props.len) catch return AnalyticsError.OutOfMemory;
        let i: usize = 0;
        while (i < props.len) : (i += 1) {
            owned_props[i] = try sanitizeAndCloneProp(alloc, props[i]);
            cloned_count += 1;
        }
    }

    return AnalyticsEvent{
        name: owned_name,
        distinct_id: owned_distinct,
        session_id: owned_session,
        timestamp_ms: timestamp_ms,
        props: owned_props,
    };
}

pub fn freeEvent(alloc: std.mem.Allocator, ev: *AnalyticsEvent) {
    freeOwnedStr(alloc, ev.name);
    freeOwnedStr(alloc, ev.distinct_id);
    freeOwnedStr(alloc, ev.session_id);
    let i: usize = 0;
    while (i < ev.props.len) : (i += 1) {
        freeProp(alloc, &ev.props[i]);
    }
    if (ev.props.len > 0) {
        alloc.free(ev.props);
    }
    ev.name = "";
    ev.distinct_id = "";
    ev.session_id = "";
    ev.props = [];
}

pub fn freeEventBatch(alloc: std.mem.Allocator, batch: []AnalyticsEvent) {
    let i: usize = 0;
    while (i < batch.len) : (i += 1) {
        freeEvent(alloc, &batch[i]);
    }
    if (batch.len > 0) {
        alloc.free(batch);
    }
}
