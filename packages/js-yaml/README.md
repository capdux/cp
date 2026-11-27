[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# js-yaml

YAML 1.1 parser and emitter (libyaml 0.2.5) native C binding for CP

| Metadata | Value |
| --- | --- |
| Package | `js-yaml` |
| Version | `0.2.5` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add js-yaml
```

```cp
import {
    YamlError,
    YamlEvent,
    YamlParser,
    cp_yaml_parse_events,
    cp_yaml_parser_free,
    cp_yaml_next_event,
    cp_yaml_event_type,
    cp_yaml_event_scalar,
} from "js-yaml";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `error` | `YamlError` | `InitFailed, ParseFailed` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `YamlEvent` | `class` | Fields: `kind: i32, scalar: string?` · Methods: `isScalar, isMappingStart, isMappingEnd, isSequenceStart, isSequenceEnd, isStreamEnd` |
| `YamlParser` | `class` | Fields: `raw: *auto!?` · Methods: `initFromString, versionString, nextEvent, lastError, __cp_deinit` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `cp_yaml_parse_events` | `(input: [*]&u8!, size: usize): *auto!?` | Invokes `cp_yaml_parse_events` and returns `*auto!?`. |
| `cp_yaml_parser_free` | `(parser: *auto!?)` | Executes `cp_yaml_parser_free` in-place. |
| `cp_yaml_next_event` | `(parser: *auto!?): i32` | Invokes `cp_yaml_next_event` and returns `i32`. |
| `cp_yaml_event_type` | `(parser: *auto!?): i32` | Invokes `cp_yaml_event_type` and returns `i32`. |
| `cp_yaml_event_scalar` | `(parser: *auto!?, out_len: *usize!?): [*:0]&u8!?` | Invokes `cp_yaml_event_scalar` and returns `[*:0]&u8!?`. |
| `cp_yaml_error_problem` | `(parser: *auto!?): [*:0]&u8!?` | Invokes `cp_yaml_error_problem` and returns `[*:0]&u8!?`. |
| `cp_yaml_version_string` | `(): [*:0]&u8!?` | Invokes `cp_yaml_version_string` and returns `[*:0]&u8!?`. |

## Minimal Example

```cp
import { cp_yaml_version_string } from "js-yaml";

pub fn main() {
    let result = cp_yaml_version_string();
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-overhead native binding**: Direct CP FFI to the underlying native C library without GC pauses or dynamic runtime wrappers.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Deterministic resource cleanup**: Native handles provide RAII `__cp_deinit` or explicit `defer` release functions to prevent leaks.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
