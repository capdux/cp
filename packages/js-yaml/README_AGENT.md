# js-yaml — AI Agent Quick Ref

YAML 1.1 parser and emitter (libyaml 0.2.5) native C binding for CP (`v0.2.5`, `MIT`)

## Import

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

## Core Types & Constants

```cp
error YamlError { InitFailed, ParseFailed }
class YamlEvent { kind: i32, scalar: string? } // methods: isScalar(...) +5
class YamlParser { raw: *auto!? } // methods: initFromString(...) +4
```

## API Signatures

```cp
fn cp_yaml_parse_events(input: [*]&u8!, size: usize): *auto!?;
fn cp_yaml_parser_free(parser: *auto!?);
fn cp_yaml_next_event(parser: *auto!?): i32;
fn cp_yaml_event_type(parser: *auto!?): i32;
fn cp_yaml_event_scalar(parser: *auto!?, out_len: *usize!?): [*:0]&u8!?;
fn cp_yaml_error_problem(parser: *auto!?): [*:0]&u8!?;
fn cp_yaml_version_string(): [*:0]&u8!?;
```

## Minimal Usage

```cp
import { cp_yaml_version_string } from "js-yaml";

pub fn main() {
    let result = cp_yaml_version_string();
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"js-yaml"`; install via `cap add js-yaml`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Error unions (`!T`)**: Handle fallible calls with `try`, `catch`, or `?` error propagation (CP has no `try/catch` blocks).
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
- **Native FFI safety**: Ensure C pointers are non-null and released deterministically via `defer` or `__cp_deinit`.
