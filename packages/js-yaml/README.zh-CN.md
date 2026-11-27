[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# js-yaml

> 版本：`0.2.5` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

YAML 1.1 parser and emitter (libyaml 0.2.5) native C binding for CP

## 安装与导入

```bash
cap add js-yaml
```

```cp
import { cp_yaml_parse_events } from "js-yaml";
```

## 核心导出 API

- `export fn cp_yaml_parse_events(input: [*]&u8!, size: usize): *auto!?;`
- `export fn cp_yaml_parser_free(parser: *auto!?);`
- `export fn cp_yaml_next_event(parser: *auto!?): i32;`
- `export fn cp_yaml_event_type(parser: *auto!?): i32;`
- `export fn cp_yaml_event_scalar(parser: *auto!?, out_len: *usize!?): [*:0]&u8!?;`
- `export fn cp_yaml_error_problem(parser: *auto!?): [*:0]&u8!?;`
- `export fn cp_yaml_version_string(): [*:0]&u8!?;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`js-yaml@0.2.5`)，许可证：MIT
- **上游生态**：`crates.io` (`serde_yaml@0.2.5`)，许可证：MIT
