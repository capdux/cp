# js-yaml@0.2.5 (Agent Quickref)

> YAML 1.1 parser and emitter (libyaml 0.2.5) native C binding for CP | License: MIT
> Import: `import { ... } from "js-yaml";` | Full types: `src/index.d.cp`

## Functions
- `export fn cp_yaml_parse_events(input: [*]&u8!, size: usize): *auto!?;`
- `export fn cp_yaml_parser_free(parser: *auto!?);`
- `export fn cp_yaml_next_event(parser: *auto!?): i32;`
- `export fn cp_yaml_event_type(parser: *auto!?): i32;`
- `export fn cp_yaml_event_scalar(parser: *auto!?, out_len: *usize!?): [*:0]&u8!?;`
- `export fn cp_yaml_error_problem(parser: *auto!?): [*:0]&u8!?;`
- `export fn cp_yaml_version_string(): [*:0]&u8!?;`
