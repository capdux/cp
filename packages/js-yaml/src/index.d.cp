// Type definitions for CP

import {  } from "std";
export fn cp_yaml_parse_events(input: [*]&u8!, size: usize): *auto!?;
export fn cp_yaml_parser_free(parser: *auto!?);
export fn cp_yaml_next_event(parser: *auto!?): i32;
export fn cp_yaml_event_type(parser: *auto!?): i32;
export fn cp_yaml_event_scalar(parser: *auto!?, out_len: *usize!?): [*:0]&u8!?;
export fn cp_yaml_error_problem(parser: *auto!?): [*:0]&u8!?;
export fn cp_yaml_version_string(): [*:0]&u8!?;
export error YamlError { InitFailed, ParseFailed }

export class YamlEvent {
    kind: i32,
    scalar: string?,
    fn isScalar(): bool;
    fn isMappingStart(): bool;
    fn isMappingEnd(): bool;
    fn isSequenceStart(): bool;
    fn isSequenceEnd(): bool;
    fn isStreamEnd(): bool;
}

export class YamlParser {
    raw: *auto!?,
    fn initFromString(input: &[]u8): !YamlParser;
    fn versionString(): string;
    fn nextEvent(): !YamlEvent?;
    fn lastError(): string?;
    fn __cp_deinit(_: &mut YamlParser);
}

