// @cap/yaml — YAML 1.1 parser (libyaml 0.2.5) native C binding for CP
// License: MIT (Upstream: yaml/libyaml)

import std from "std";

pub @c {
    fn cp_yaml_parse_events(input: [*]const u8!, size: usize): *!?;
    fn cp_yaml_parser_free(parser: *!?);
    fn cp_yaml_next_event(parser: *!?): i32;
    fn cp_yaml_event_type(parser: *!?): i32;
    fn cp_yaml_event_scalar(parser: *!?, out_len: *usize?!): [*:0]const u8?;
    fn cp_yaml_error_problem(parser: *!?): [*:0]const u8?;
    fn cp_yaml_version_string(): [*:0]const u8?;
}

pub const EVENT_NONE: i32 = 0;
pub const EVENT_STREAM_START: i32 = 1;
pub const EVENT_STREAM_END: i32 = 2;
pub const EVENT_DOCUMENT_START: i32 = 3;
pub const EVENT_DOCUMENT_END: i32 = 4;
pub const EVENT_ALIAS: i32 = 5;
pub const EVENT_SCALAR: i32 = 6;
pub const EVENT_SEQUENCE_START: i32 = 7;
pub const EVENT_SEQUENCE_END: i32 = 8;
pub const EVENT_MAPPING_START: i32 = 9;
pub const EVENT_MAPPING_END: i32 = 10;

pub err YamlError {
    InitFailed,
    ParseFailed,
}

/// Non-owning zero-copy view of the current YAML parser event.
/// Valid until the next `nextEvent()` call or parser destruction.
pub struct YamlEvent {
    pub kind: i32,
    pub scalar: string?,

    pub fn isScalar(): bool {
        return _.kind == EVENT_SCALAR;
    }

    pub fn isMappingStart(): bool {
        return _.kind == EVENT_MAPPING_START;
    }

    pub fn isMappingEnd(): bool {
        return _.kind == EVENT_MAPPING_END;
    }

    pub fn isSequenceStart(): bool {
        return _.kind == EVENT_SEQUENCE_START;
    }

    pub fn isSequenceEnd(): bool {
        return _.kind == EVENT_SEQUENCE_END;
    }

    pub fn isStreamEnd(): bool {
        return _.kind == EVENT_STREAM_END;
    }
}

/// Owned RAII YAML event-stream parser backed by libyaml 0.2.5.
/// Automatically releases native libyaml resources on scope exit.
pub struct YamlParser {
    raw: *!? = null,

    pub static fn initFromString(input: []const u8): YamlParser! {
        let p: *!? = null;
        unsafe {
            p = cp_yaml_parse_events(input.ptr, input.len);
        }
        if (p == null) return YamlError.InitFailed;
        return YamlParser{ .raw = p };
    }

    pub static fn versionString(): string {
        unsafe {
            if (ver := cp_yaml_version_string()) {
                return std.mem.span(ver);
            }
        }
        return "0.2.5";
    }

    pub fn nextEvent(): YamlEvent?! {
        if (_.raw == null) return YamlError.ParseFailed;
        let rc: i32 = 0;
        unsafe {
            rc = cp_yaml_next_event(_.raw);
        }
        if (rc < 0) return YamlError.ParseFailed;
        if (rc == 0) return null;

        let kind: i32 = EVENT_NONE;
        unsafe {
            kind = cp_yaml_event_type(_.raw);
            if (kind == EVENT_SCALAR) {
                let len: usize = 0;
                if (ptr := cp_yaml_event_scalar(_.raw, &len)) {
                    return YamlEvent{ .kind = kind, .scalar = ptr[0..len] };
                }
            }
        }
        return YamlEvent{ .kind = kind, .scalar = null };
    }

    pub fn lastError(): string? {
        if (_.raw == null) return null;
        unsafe {
            if (msg := cp_yaml_error_problem(_.raw)) {
                return std.mem.span(msg);
            }
        }
        return null;
    }

    ~init() {
        if (p := _.raw) {
            unsafe { cp_yaml_parser_free(p); }
            _.raw = null;
        }
    }
}
