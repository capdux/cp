[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# js-yaml

YAML 1.1 parser and emitter (libyaml 0.2.5) native C binding for CP

| 元信息 | 值 |
| --- | --- |
| 包名 | `js-yaml` |
| 版本 | `0.2.5` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

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

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `error` | `YamlError` | `InitFailed, ParseFailed` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `YamlEvent` | `class` | 字段: `kind: i32, scalar: string?` · 方法: `isScalar, isMappingStart, isMappingEnd, isSequenceStart, isSequenceEnd, isStreamEnd` |
| `YamlParser` | `class` | 字段: `raw: *auto!?` · 方法: `initFromString, versionString, nextEvent, lastError, __cp_deinit` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `cp_yaml_parse_events` | `(input: [*]&u8!, size: usize): *auto!?` | 调用 `cp_yaml_parse_events` 并返回 `*auto!?`。 |
| `cp_yaml_parser_free` | `(parser: *auto!?)` | 就地执行 `cp_yaml_parser_free` 操作。 |
| `cp_yaml_next_event` | `(parser: *auto!?): i32` | 调用 `cp_yaml_next_event` 并返回 `i32`。 |
| `cp_yaml_event_type` | `(parser: *auto!?): i32` | 调用 `cp_yaml_event_type` 并返回 `i32`。 |
| `cp_yaml_event_scalar` | `(parser: *auto!?, out_len: *usize!?): [*:0]&u8!?` | 调用 `cp_yaml_event_scalar` 并返回 `[*:0]&u8!?`。 |
| `cp_yaml_error_problem` | `(parser: *auto!?): [*:0]&u8!?` | 调用 `cp_yaml_error_problem` 并返回 `[*:0]&u8!?`。 |
| `cp_yaml_version_string` | `(): [*:0]&u8!?` | 调用 `cp_yaml_version_string` 并返回 `[*:0]&u8!?`。 |

## 最小调用示例

```cp
import { cp_yaml_version_string } from "js-yaml";

pub fn main() {
    let result = cp_yaml_version_string();
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **零开销原生绑定**：通过 CP FFI 直接调用底层原生 C 库，无 GC 停顿与动态运行时桥接开销。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **确定性资源释放**：底层句柄支持 RAII `__cp_deinit` 或 `defer` 显式释放，防止句柄与内存泄漏。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
