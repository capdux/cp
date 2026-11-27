[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/matchit

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:matchit` ([https://github.com/ibraheemdev/matchit](https://github.com/ibraheemdev/matchit))

Zero-allocation high-priority URL router for CP — clean-room MIT parity with Rust `matchit` (595M downloads on crates.io, powering `axum`).

纯 CP 零堆分配超高速 URL 路由引擎，严格遵循 **静态字面段 > 命名参数段 (`:id` / `{id}`) > 通配捕获段 (`*rest` / `{*rest}`)** 优先级，零拷贝返回路径参数切片。

## Install / 安装

```bash
cap add @cap/matchit
```

## API

| Symbol | Signature | Description |
| --- | --- | --- |
| `RouteParam` | `struct { key: string, value: string }` | Zero-copy extracted parameter |
| `RouteMatch` | `struct { matched: bool, handler_id: u32, params: [8]RouteParam, param_count: usize }` | Match result with `param(key) -> string?` |
| `Router` | `init()` / `insert(*_, pattern, handler_id) -> bool` / `at(path) -> RouteMatch` / `routeCount() -> usize` | Fixed-capacity (64 routes) zero-allocation URL router |

## Usage / 使用示例

```cp
import { Router } from "@cap/matchit";

let r = Router.init();
* = r.insert("/users/me", 1 as u32);
* = r.insert("/users/:id", 2 as u32);
* = r.insert("/files/*filepath", 3 as u32);

const m = r.at("/users/42");
// m.matched == true, m.handler_id == 2, m.param("id") == "42"
```
