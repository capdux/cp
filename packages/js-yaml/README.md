[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# js-yaml

> Native C binding (`libyaml 0.2.5`) YAML 1.1 streaming parser & emitter for CP.

- **License**: MIT
- **Type declarations**: [`src/index.d.cp`](./src/index.d.cp)
- **Install / Import**:
  ```cp
  import { cp_yaml_parse_events, cp_yaml_next_event, cp_yaml_parser_free } from "js-yaml";
  ```

## Overview

`js-yaml` wraps the battle-tested C `libyaml` 0.2.5 streaming event engine with zero-overhead CP FFI bindings, providing deterministic memory management via `defer cp_yaml_parser_free(parser)`.

## Quick Example

```cp
import {
    cp_yaml_parse_events,
    cp_yaml_next_event,
    cp_yaml_event_type,
    cp_yaml_parser_free,
} from "js-yaml";

fn countEvents(yaml: string): usize {
    let parser = cp_yaml_parse_events(yaml.ptr, yaml.len) ?? return 0;
    defer cp_yaml_parser_free(parser);
    let mut n: usize = 0;
    while (cp_yaml_next_event(parser) == 1) {
        n += 1;
        if (cp_yaml_event_type(parser) == 2) { break; }
    }
    return n;
}
```

For full API signatures, see [`src/index.d.cp`](./src/index.d.cp) or [`README_AGENT.md`](./README_AGENT.md).
