# `cap:http_router` — Radix Trie HTTP 路由器与中间件分发标准库

## 1. 概述

`cap:http_router`（同时支持 `import cap from "cap"` 下的 `cap.http_router`）提供纯 CP 实现的高性能 Radix Trie HTTP 路由器，专为零堆路径参数提取、严格优先级路由匹配、路由分组及自动 `405 Method Not Allowed` (`Allow` 头) 响应而设计。

- **零堆匹配**：`match()` 使用栈上固定段切片（最多 64 段）与固定参数缓冲（`MAX_PARAMS = 16`），匹配与参数提取全程零堆内存分配。
- **严格优先级**：同一层级严格遵循 **Static > Param (`:id`) > Catch-All (`*filepath`)**。
- **完备的错误校验**：插入时自动拦截同路径同方法重复注册、同级不同参数名冲突（`RouteConflict`）以及非末尾通配符（`InvalidCatchAll`）。
- **路由分组与规范化**：支持 `RouteGroup` 嵌套前缀拼接，自动折叠多余斜杠并忽略查询字符串 `?query`。

---

## 2. 核心类型

| 类型 | 说明 |
| :--- | :--- |
| `MAX_PARAMS` | 单条路由支持提取的最大参数个数（`16`） |
| `RouterError` | 错误集合：`OutOfMemory`、`InvalidPattern`、`InvalidCatchAll`、`RouteConflict`、`TooManyParams` |
| `HttpMethod` | HTTP 方法枚举：`GET`、`POST`、`PUT`、`DELETE`、`PATCH`、`HEAD`、`OPTIONS` |
| `PathParam` | 路径参数键值对 `{ name: []const u8, value: []const u8 }` |
| `RouteMatch` | 匹配成功结果，含 `handler_id`、参数数组与 `getParam(name)` / `paramSlice()` |
| `AllowInfo` | 405 允许方法列表，提供 `allowHeader(): []const u8`（如 `"GET, POST"`） |
| `RouteResult` | 路由匹配联合结果：`matched`、`method_not_allowed`、`not_found` |
| `RouteGroup` | 带前缀的路由分组句柄，支持嵌套 `.group()` 与快捷方法注册 |
| `HttpRouter` | 路由器主结构体，负责路由树构建、分组管理与请求分发 |

---

## 3. API 速查

### `HttpRouter`
- `HttpRouter.init(alloc: std.mem.Allocator): RouterError!HttpRouter`
- `router.deinit()`
- `router.addRoute(method: HttpMethod, pattern: []const u8, handler_id: usize): RouterError!void`
- `router.get / post / put / delete / patch / head / options(pattern, handler_id): RouterError!void`
- `router.group(prefix: []const u8): RouterError!RouteGroup`
- `router.match(method: HttpMethod, path: []const u8): RouteResult`

### `RouteGroup`
- `group.addRoute(method, pattern, handler_id): RouterError!void`
- `group.get / post / put / delete / patch / head / options(pattern, handler_id): RouterError!void`
- `group.group(sub_prefix: []const u8): RouterError!RouteGroup`

### `RouteMatch` & `RouteResult`
- `res.matchInfo(): RouteMatch?`
- `res.allowInfo(): AllowInfo?`
- `res.isNotFound(): bool`
- `m.getParam(name: []const u8): ([]const u8)?`
- `m.paramSlice(): []const PathParam`
- `a.allowHeader(): []const u8`

---

## 4. 示例

```cp
const std = @import("std");
import { HttpRouter, HttpMethod } from "cap:http_router";

pub fn main(): !void {
    let router = try HttpRouter.init(std.heap.page_allocator);
    defer router.deinit();

    try router.get("/users/me", 1);
    try router.get("/users/:id/posts/:pid", 2);
    try router.get("/static/*filepath", 3);

    const api = try router.group("/api/v1");
    try api.post("/orders", 4);

    const res = router.match(.GET, "/users/42/posts/99");
    if (m := res.matchInfo()) {
        const id = m.getParam("id") orelse "";
        const pid = m.getParam("pid") orelse "";
        _ = id;
        _ = pid;
    }
}
```
