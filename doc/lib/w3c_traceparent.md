# cap:w3c_traceparent

> 参考来源：/data/dev/bun/src/http/http.cp | 吸收日期：2026-11-22

基于 W3C Trace Context Level 1 规范，提供纯 CP、零外部依赖、零堆分配的 `traceparent` 与 `tracestate` 分布式追踪头解析与格式化库。

## 导入方式

```cp
import { TraceParent, TraceStateEntry, TraceStateIter, findTraceState } from "cap:w3c_traceparent";
```

## 核心类型与 API

### `TraceParent`

字段：`version: u8`、`trace_id: [16]u8`、`parent_id: [8]u8`、`trace_flags: u8`。

- `TraceParent.parse(header: []const u8): TraceParent?`：严格遵循 W3C Trace Context（拒绝大写十六进制、`version == 0xff`、全零 `trace_id`、全零 `parent_id`；`version > 0` 支持 `-extra` 向前兼容）。
- `isSampled(): bool`：检查采样标志位（`bit 0`）。
- `withSampled(sampled: bool): TraceParent`：返回更新采样位后的新实例。
- `withParentId(new_parent: [8]u8): TraceParent`：返回更新子 Span `parent_id` 后的新实例。
- `format(out: []u8): ([]const u8)?`：规范化输出 55 字节小写 `traceparent` 头部。

### `TraceStateEntry` / `TraceStateIter` / `findTraceState`

- `TraceStateIter.init(header)` / `next(*_): TraceStateEntry?`：按逗号流式迭代合法 `key=value` 成员（支持 `tenant@vendor` 多租户键）。
- `findTraceState(header: []const u8, key: []const u8): ([]const u8)?`：按 vendor key 精确查找值。

## 使用示例

```cp
import { TraceParent, findTraceState } from "cap:w3c_traceparent";

fn example() {
    if (tp := TraceParent.parse("00-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-01")) {
        const child = tp.withParentId([0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08]);
        let buf: [64]u8 = undefined;
        * = child.format(&buf);
    }
    * = findTraceState("rojo=00f067aa0ba902b7,congo=t61rcWkgMzE", "congo");
}
```

## 上游行为契约与分布式追踪边界

1. **W3C Trace Context 严格合法性拒绝**：
   - `TraceParent.parse` 严格拒绝大写十六进制字符、保留版本号 `version == 0xff`、全零 `trace_id`（16 字节全 0）以及全零 `parent_id`（8 字节全 0）；收到非法头时按 W3C 规范应开启全新 Trace，避免污染下游观测链路。
2. **未来版本向前兼容**：
   - `version == 0x00` 严格要求 55 字节精确长度；当遇到 `version > 0`（如未来 Level 2 扩展 `-extra` 尾部字段）时允许忽略已知 55 字节后的附加后缀，实现平滑跨版本互通。
3. **Span 透传与 `tracestate` 多租户键**：
   - 跨服务转发时使用 `withParentId` 替换为当前节点新生成的 8 字节 Span ID，同时通过 `withSampled` 保留或调整采样决策位；`TraceStateIter` / `findTraceState` 原生支持 `tenant@vendor` 多租户键格式。
