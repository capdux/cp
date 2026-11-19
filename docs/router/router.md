# CP 标准库：高性能 Radix-Tree 路由引擎与反向路径格式化手册 (`cap:router`)

## 一、 架构总览与设计准则

`cap:router` 是 CP 语言官方内置的泛型 Radix-Tree（前缀字典树）HTTP 路由匹配与反向路径格式化标准库，支持按 HTTP 方法隔离路由树、静态段优先匹配、`:param` / `[param]` 动态参数提取、`*path` / `[...path]` 通配符捕获、栈内联零堆分配参数字典 `ParamMap`，以及模板反向路径渲染器 `RouteFormatter`。核心架构吸收自 Bun 路由子系统（`/data/dev/bun/src/router/`）。

```text
┌─────────────────────────────────────────────────────────────────────────┐
│                       Router(T) 按方法分片森林                           │
│   [0:GET] [1:POST] [2:PUT] [3:DELETE] [4:PATCH] [5:HEAD] [6:OPTIONS]    │
│   [7:ALL  (兜底方法树：当特定方法未命中时自动回退匹配)]                    │
├─────────────────────────────────────────────────────────────────────────┤
│                    RadixNode(T) 三级匹配优先级                           │
│   ① 静态子节点 children[16]  >  ② 动态参数 param_child  >  ③ 通配 wildcard│
├────────────────────────────────────┬────────────────────────────────────┤
│ RouteMatch(T) + ParamMap (16 栈槽)  │ RouteFormatter (反向路径生成)       │
│ handler: T, params.get("id")       │ extractParamNames / formatPath     │
└────────────────────────────────────┴────────────────────────────────────┘
```

### 1. 模块文件拓扑与谱系来源

`lib/router/` 下全部 5 个源文件第 1 行均严格标记上游 Bun 源码谱系：

| 文件路径 | 上游谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/router/param_map.cp` | `// Source: /data/dev/bun/src/router/ParamMap.cp` | `Param` 与 `ParamMap`（固定 16 槽位栈内联存储，提供只读 `get`/`has`/`at`/`count`/`isEmpty` 与写入 `append`） |
| `lib/router/route.cp` | `// Source: /data/dev/bun/src/router/Route.cp` | 泛型匹配结果 `RouteMatch(comptime T: type)`，封装 `handler: T` 与 `params: ParamMap` |
| `lib/router/radix.cp` | `// Source: /data/dev/bun/src/router/Router.cp` | 泛型 `RadixNode(T)` 与 `Router(T)`，支持 `get`/`post`/`put`/`delete`/`patch`/`all` 注册与 `lookup` 查找 |
| `lib/router/pattern.cp` | `// Source: /data/dev/bun/src/router/Router.cp` | `RouteFormatter`：占位符提取 `extractParamNames` 与反向路径渲染 `formatPath` |
| `lib/router/index.cp` | `// Source: /data/dev/bun/src/router/router.cp` | `cap:router` 统一对外导出入口 |

---

## 二、 核心 API 与匹配语义

### 1. 路径语法与优先级规则

`Router(T)` 自动规范化首尾斜杠（如 `/users/` 与 `/users` 等价），每层段匹配遵循严格的三级优先级：

1. **静态段（最高优先级）**：精确字节比较，例如 `/users/profile` 优先于 `/users/:id`。
2. **动态参数段**：同时兼容 Express 风格 `:id` 与文件路由风格 `[id]`，捕获单段内容存入 `ParamMap`。
3. **通配符捕获段（兜底优先级）**：同时兼容 `*filepath` / `*` 与 `[...docpath]`，将剩余全部子路径（含 `/`）一次性捕获并结束匹配。

### 2. HTTP 方法分片与 `ALL` 回退

- `Router(T)` 内建 8 棵独立的 Radix 树，按 `methodIndex` 映射：`GET (0)`、`POST (1)`、`PUT (2)`、`DELETE (3)`、`PATCH (4)`、`HEAD (5)`、`OPTIONS (6)`、`ALL (7)`。
- 调用 `lookup(method, path)` 时，先在对应方法树查找；若未命中且 `method != "ALL"`，自动回退查询 `trees[7]`（由 `router.all(path, handler)` 注册的跨方法路由）。

### 3. 反向路由格式化 (`RouteFormatter`)

- **`extractParamNames(pattern, out_names): usize`**：扫描路径模板中的所有 `:param` 占位符，返回参数个数并将参数名切片写入 `out_names`（零分配）。
- **`formatPath(pattern, params, out): usize!`**：将模板中的 `:param` 和 `*` 按 `ParamMap` 替换为真实路径并写入 `out`。缺少必需参数时返回 `error.MissingRouteParam`，目标缓冲区不足时返回 `error.BufferTooSmall`。

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import { Router, ParamMap, RouteFormatter } from "cap:router";

pub fn demoRouter(): !void {
    let router = Router(string).init(std.heap.page_allocator);

    // 1. 注册静态、动态参数与通配符路由
    router.get("/", "index_handler");
    router.get("/users/:id/posts/:postId", "user_post_handler");
    router.get("/org/[orgId]/members/[memberId]", "org_member_handler");
    router.get("/assets/*filepath", "static_asset_handler");
    router.all("/health", "health_handler");

    // 2. 路由查找与零分配参数读取
    if (m := router.lookup("GET", "/users/42/posts/99")) {
        // m.handler == "user_post_handler"
        const uid = m.params.get("id").?;       // "42"
        const pid = m.params.get("postId").?;   // "99"
        * = uid;
        * = pid;
    }

    if (asset := router.lookup("GET", "/assets/img/logo.svg")) {
        const fp = asset.params.get("filepath").?; // "img/logo.svg"
        * = fp;
    }

    // 3. 反向路径生成与占位符提取
    let names: [4][]const u8 = undefined;
    const n_cnt = RouteFormatter.extractParamNames("/users/:id/posts/:postId", names[0..4]);
    * = n_cnt; // 2

    let pm = ParamMap.init();
    pm.append("id", "42");
    pm.append("postId", "99");

    let out: [128]u8 = [0 as u8; 128];
    const url_len = try RouteFormatter.formatPath("/users/:id/posts/:postId", &pm, out[0..128]);
    // out[0..url_len] == "/users/42/posts/99"
    * = url_len;
}
```

---

## 四、 与 Bun / Node.js API 对照表

| 功能特性 | Bun (`Bun.FileSystemRouter` / `Bun.serve({ routes })`) | Node.js (`find-my-way` / `express.Router`) | CP 标准库 (`cap:router`) |
| :--- | :--- | :--- | :--- |
| **Radix 路由表** | `Bun.serve` 内置 Radix 路由树 | `find-my-way` / `rou3` | `Router(T).init(allocator)` |
| **动态参数语法** | `:id` 与 `[id]`（Next.js 风格） | `:id` | 同时支持 `:id` 与 `[id]` |
| **通配符捕获** | `*` 与 `[...slug]` | `*` | 同时支持 `*name`、`*` 与 `[...name]` |
| **参数容器** | `req.params` (JS Object) | `req.params` (JS Object) | `ParamMap`（16 槽栈内联零堆分配） |
| **全方法兜底** | `routes["/health"]` | `app.all("/health", fn)` | `router.all("/health", handler)` |
| **反向路径渲染** | — | `path-to-regexp` `compile()` | `RouteFormatter.formatPath` / `extractParamNames` |
