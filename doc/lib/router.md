# CP 标准库：高性能 Radix-Tree 与文件系统路由引擎手册 (`cap:router`)

> 参考来源：`/data/dev/bun/docs/runtime/file-system-router.mdx`, `/data/dev/bun/src/router/router.cp`, `/data/dev/bun/src/router/ParamMap.cp` | 吸收日期：2026-11-22

## 一、 架构总览与设计准则

`cap:router` 是 CP 语言官方内置的泛型 Radix-Tree（前缀字典树）与 Next.js/Bun 风格 `FileSystemRouter` 路由标准库，支持按 HTTP 方法隔离路由树、5 级严格优先级匹配（静态段 > 混合前缀/后缀段 > `:param`/`[param]` 参数段 > `*`/`**`/`[...all]` 通配段 > `[[...opt]]` 可选捕获段）、回溯多参数提取、冲突/重复检测、尾随斜杠模式、栈内联零堆分配 `ParamMap` 以及 `RouteFormatter`。核心架构吸收自 Bun 路由子系统（`/data/dev/bun/src/router/`）。

```text
┌─────────────────────────────────────────────────────────────────────────────┐
│                       Router(T) 按方法分片森林                               │
│   [0:GET] [1:POST] [2:PUT] [3:DELETE] [4:PATCH] [5:HEAD] [6:OPTIONS]        │
│   [7:ALL  (兜底方法树：当特定方法未命中时自动回退匹配)]                        │
├─────────────────────────────────────────────────────────────────────────────┤
│                    RadixNode(T) 五级匹配优先级（支持回溯）                    │
│   ① 静态段 children  >  ② 混合段 hybrid_children (`doc-:id.json`)           │
│   ③ 动态参数 param_child (`:id` / `[id]`)                                   │
│   ④ 必选通配 wildcard_child (`*` / `**` / `[...path]`)                      │
│   ⑤ 可选通配 optional_catch_all_child (`[[...path]]`, 匹配 0 或多段)         │
├────────────────────────────────────┬────────────────────────────────────────┤
│ FileSystemRouter (Next.js/Bun 风格) │ RouteFormatter (反向路径生成)           │
│ [slug].cp / [...all].cp / (group)  │ extractParamNames / formatPath         │
└────────────────────────────────────┴────────────────────────────────────────┘
```

### 1. 模块文件拓扑与谱系来源

`lib/router/` 下全部源文件第 1 行均严格标记上游 Bun 源码谱系：

| 文件路径 | 上游谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/router/paramMap.cp` | `// Source: /data/dev/bun/src/router/ParamMap.cp` | `Param` 与 `ParamMap`（固定 16 槽位栈内联存储，提供只读 `get`/`has`/`at`/`count`/`isEmpty` 与写入 `append`） |
| `lib/router/route.cp` | `// Source: /data/dev/bun/src/router/Route.cp` | 泛型匹配结果 `RouteMatch(comptime T: type)`，封装 `handler: T` 与 `params: ParamMap` |
| `lib/router/radix.cp` | `// Source: /data/dev/bun/src/router/Router.cp` | 泛型 `RadixNode(T)`、`HybridSegment`、`RouterOptions`、`TrailingSlashMode` 与 `Router(T)` |
| `lib/router/fsRouter.cp` | `// Source: /data/dev/bun/src/router/router.cp` | `FileSystemRouter`：支持 `[slug].cp`、`[...catchall].cp`、`[[...optional]].cp`、`(group)` 分组忽略、`scanDir` 与优先级自动排序 |
| `lib/router/pattern.cp` | `// Source: /data/dev/bun/src/router/Router.cp` | `RouteFormatter`：占位符提取 `extractParamNames` 与反向路径渲染 `formatPath` |
| `lib/router/index.cp` | `// Source: /data/dev/bun/src/router/router.cp` | `cap:router` 统一对外导出入口 |

---

## 二、 核心 API 与匹配语义

### 1. 路径语法与 5 级严格优先级规则

每层段匹配遵循严格五级优先级，且支持在深层静态分支失配时自动回溯到同级混合段、参数段或通配段：

1. **静态段（最高优先级）**：精确字节比较，例如 `/files/latest.json` 优先于 `/files/doc-:id.json`。
2. **前缀/后缀混合段**：支持 `prefix-:param`、`:name.json`、`doc-:id.json`、`file-[id].cp` 等混合段，按 `prefix.len + suffix.len` 降序排列。
3. **动态参数段**：同时兼容 Express 风格 `:id` 与文件路由风格 `[id]`，捕获单段内容存入 `ParamMap`。
4. **通配符捕获段**：同时兼容 `*filepath` / `*` / `**` / `[...docpath]`，至少匹配 1 段并捕获全部剩余子路径。
5. **可选捕获段（兜底优先级）**：支持 `[[...rest]]`，可匹配 0 段（不写入参数）或任意多段剩余子路径。

### 2. 冲突检测与尾随斜杠规范化 (`RouterOptions` & `tryInsert`)

- `TrailingSlashMode`：`Ignore`（默认，`/users/` 等价于 `/users`）、`Strict`（区分 `/users/` 与 `/users`）、`Canonical`。
- `router.tryInsert(method, path, handler)` 在 `detect_conflicts = true` 时静态检测：
  - `error.DuplicateRoute`（同方法同路径重复注册）；
  - `error.RouteConflict`（同层级参数名不一致冲突，如 `/users/:id` vs `/users/:name`）；
  - `error.CatchAllMustBeAtEnd`（通配符未置于路径末尾）。

### 3. 文件系统路由映射器 (`FileSystemRouter`)

- **规则映射**：
  - `index.cp` -> `/`；`users/index.cp` -> `/users`；
  - `(group)/about.cp` -> `/about`（圆括号路由组从 URL 中自动剥离，保留原 `file_path`）；
  - `blog/[slug].cp` -> 动态参数 `slug`；
  - `files/[...catchall].cp` -> 必选全捕获 `catchall`（至少 1 段）；
  - `docs/[[...optional]].cp` -> 可选全捕获 `optional`（支持 `/docs` 与 `/docs/a/b`）；
- **自动优先级排序**：`Static < Dynamic < CatchAll < OptionalCatchAll`，并按段优先级及静态段数量排序；跨路由组路径冲突自动返回 `error.DuplicateRoute`。
- **目录扫描**：支持内存注册 `addFile("...")` 以及递归磁盘扫描 `scanDir("/path/to/pages")`。

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import {
    Router,
    RouterOptions,
    TrailingSlashMode,
    FileSystemRouter,
    ParamMap,
    RouteFormatter
} from "cap:router";

pub fn demoRouter(): !void {
    let router = Router(string).initWithOptions(std.heap.page_allocator, RouterOptions{
        trailing_slash: TrailingSlashMode.Ignore,
        detect_conflicts: true,
    });

    // 1. 注册静态、混合前缀/后缀、动态参数、通配符与可选捕获路由
    try router.tryInsert("GET", "/", "index_handler");
    try router.tryInsert("GET", "/files/latest.json", "latest_handler");
    try router.tryInsert("GET", "/files/doc-:id.json", "doc_handler");
    try router.tryInsert("GET", "/users/:id/posts/:postId", "user_post_handler");
    try router.tryInsert("GET", "/assets/**filepath", "static_asset_handler");
    try router.tryInsert("GET", "/docs/[[...slug]]", "docs_handler");
    router.all("/health", "health_handler");

    // 2. 混合段与可选捕获查询
    if (m := router.lookup("GET", "/files/doc-42.json")) {
        const id = m.params.get("id").?; // "42"
        * = id;
    }
    if (d0 := router.lookup("GET", "/docs")) {
        // 0 段匹配成功，d0.handler == "docs_handler"
        * = d0;
    }

    // 3. FileSystemRouter 文件系统路由映射
    let fsr = FileSystemRouter.init(std.heap.page_allocator);
    try fsr.addFile("docs/[[...optional]].cp");
    try fsr.addFile("(marketing)/about.cp");
    try fsr.addFile("blog/[slug].cp");
    fsr.sortRoutes();

    if (fm := fsr.matchRoute("/about")) {
        // fm.file_path == "(marketing)/about.cp"
        * = fm;
    }
}
```

---

## 四、 与 Bun / Node.js API 对照表

| 功能特性 | Bun (`Bun.FileSystemRouter` / `Bun.serve({ routes })`) | Node.js (`find-my-way` / `express.Router`) | CP 标准库 (`cap:router`) |
| :--- | :--- | :--- | :--- |
| **Radix 路由表** | `Bun.serve` 内置 Radix 路由树 | `find-my-way` / `rou3` | `Router(T).initWithOptions(allocator, opts)` |
| **混合前缀/后缀段** | — | `find-my-way` 参数后缀 | `HybridSegment`（`doc-:id.json` / `file-[id].cp`） |
| **动态与捕获语法** | `:id`、`[id]`、`[...slug]`、`[[...opt]]` | `:id`、`*` | 全面支持 `:id`、`[id]`、`*`、`**`、`[...all]`、`[[...opt]]` |
| **文件系统路由** | `new Bun.FileSystemRouter({ dir })` | Next.js App/Pages Router | `FileSystemRouter`（含 `(group)` 分组与 `scanDir`） |
| **冲突与重复检测** | `RouteLoader` 重复路径报错 | 启动期抛异常 | `tryInsert` 返回 `DuplicateRoute` / `RouteConflict` |
| **参数容器** | `req.params` (JS Object) | `req.params` (JS Object) | `ParamMap`（16 槽栈内联零堆分配） |
| **反向路径渲染** | — | `path-to-regexp` `compile()` | `RouteFormatter.formatPath` / `extractParamNames` |

---

## 五、 上游 Bun 行为契约与路由边界注意事项

1. **文件系统路由约定（参考 `/data/dev/bun/docs/runtime/file-system-router.mdx`）**：
   - `FileSystemRouter` 遵循 Next.js / Bun Pages Router 风格：`index.*` 映射至父目录根路径、`[slug]` 映射动态单段、`[...all]` 映射必选多段捕获、`[[...opt]]` 映射可选 0..N 段捕获、`(group)` 路由分组目录不出现在最终 URL 路径中。
2. **五级优先级与回溯确定性**：
   - 路由查找严格按 `静态段 > 混合前缀/后缀段 (HybridSegment) > 动态参数 (:id/[id]) > 必选通配 (* / [...all]) > 可选捕获 ([[...opt]])` 顺序匹配；当静态分支部分命中但后续路径不匹配时，引擎自动回溯尝试参数或通配分支，避免漏匹配。
3. **启动期冲突校验与 `ParamMap` 槽位边界**：
   - 使用 `tryInsert` 在注册期捕获重复路由（`DuplicateRoute`）与同层参数名冲突（`RouteConflict`）；`ParamMap` 固定 16 槽栈内联实现零堆分配，单条路由参数超过 16 个时应通过通配捕获后按需切分。
4. **尾随斜杠模式一致性（`TrailingSlashMode`）**：
   - 生产服务建议在 `RouterOptions` 中显式固定尾随斜杠策略（严格区分或自动归一化），防止 `/users` 与 `/users/` 在鉴权中间件与业务路由间产生匹配分歧。
5. **Next.js 方括号路由模式严格校验与零分配匹配（`lib/router/routeTypes.cp`，吸收自 `/data/dev/bun/src/router/route_types.cp` & `/data/dev/bun/docs/runtime/file-system-router.mdx`）**：
   - 导出：`ROUTE_PATTERN_MAX_PARAMS` (`16`)、`RoutePatternError`、`RoutePatternTag`（`Static`/`Dynamic`/`CatchAll`/`OptionalCatchAll`）、`RouteTinyPtr`（32-bit 紧凑 `(u16 offset, u16 len)` 指针）、`RoutePatternToken`、`RoutePatternValidation`、`RouteCapturedParam`、`RouteCapturedParams`、`RoutePatternEngine`（`validate`/`match`/`diagnosticMessage`）；
   - **严格语法边界**：`[...slug]` 与 `[[...slug]]` 必须位于路径末尾（否则报错 `CatchAllMustBeAtTheEnd`）；仅接受纯 ASCII 路由名（否则报错 `NonAsciiRoute`）；空参数名 `[]`/`[...]` 报错 `MissingParamName`；未闭合方括号报错 `PatternMissingClosingBracket`。

```cp
import { RoutePatternEngine, RouteCapturedParams } from "cap:router";

let params = RouteCapturedParams.init();
const ok = RoutePatternEngine.match("/blog/[slug]/[[...rest]]", "/blog/hello-cp/a/b", true, *params)?;
// ok == true, params.get("slug") == "hello-cp", params.get("rest") == "a/b"
* = ok;
```

6. **DevServer 路由打包生命周期状态机与客户端脚本代数管理（`lib/router/routeBundle.cp`，吸收自 `/data/dev/bun/src/bake/DevServer/RouteBundle.cp` & `/data/dev/bun/docs/bundler/fullstack.mdx`）**：
   - **导出符号**：`ROUTE_BUNDLE_MAX_COUNT` (`128`)、`RouteBundleError`、`RouteBundleKind`（`Framework`/`Html`）、`RouteBundleState`（6 态：`Unqueued`/`Bundling`/`DeferredToNextBundle`/`PossibleBundlingFailures`/`EvaluationFailure`/`Loaded`，含 `isReady`/`isFailed`/`isPending`）、`RouteBundleEntry`（`sourceMapKey`/`hasActiveViewers`/`isGenerationCurrent`/`memoryCost`/`formatClientBundleUrl`）、`RouteBundleRegistry`（`init`/`count`/`registerFramework`/`registerHtml`/`get`/`findByPattern`/`beginBundling`/`markLoaded`/`markBundlingFailure`/`markEvaluationFailure`/`deferToNextBundle`/`invalidateClientBundle`/`addViewer`/`removeViewer`/`activeRouteCount`/`totalMemoryCost`）；
   - **Bun 全栈开发服务器路由按需打包最佳实践（参考 `fullstack.mdx`）**：
     - 区分 **Framework 服务端路由**（SSR/全栈）与 **HTML 前端入口路由**（SPA，支持 `script_injection_offset` 注入 `<script>`/`<link>`）；
     - 每次下游客户端模块更新时调用 `invalidateClientBundle()` 清除旧脚本与 HTML 响应缓存并轮转 `client_script_generation`（若浏览器仍请求旧代数脚本，`isGenerationCurrent(gen)` 返回 `false` 以指示整页刷新）；
     - 通过 `addViewer()` / `removeViewer()` 维护每个路由上挂载的活跃 HMR 连接数，热重载时可跳过 `active_viewers == 0` 的冷路由，大幅降低大型全栈工程的增量图遍历开销。

```cp
import { RouteBundleRegistry } from "cap:router";

let reg = RouteBundleRegistry.init();
const id = reg.registerHtml("/dashboard", 1)?;
* = reg.addViewer(id)?;
* = reg.beginBundling(id)?;
* = reg.markLoaded(id, 4096, 1024, 1200, 256)?;
const next_gen = reg.invalidateClientBundle(id)?; // 2; client_bundle & cached_response cleared
* = next_gen;
```

---

## 零堆 O(1) 静态哈希 + 压缩 Radix 路由引擎 (`cap:router/radixRouter` — `lib/router/radixRouter.cp`)

融合 Elysia `memoirist` 风格 FNV-1a 静态快表与 Axum `matchit` 压缩 Radix Trie 的零堆路由核心（同时在 `cap:web` 中向后兼容 re-export）：

- **核心类型**：`RadixRouter`、`RadixMatchResult`、`RadixParam`。
- **关键特性**：
  - 静态路径 O(1) FNV-1a 开放寻址查表（`undefined` 零清零热路径）；
  - 动态参数 `:param` / `{param}` 与通配符 `*wildcard` 匹配，严格遵循 `Static > Param > Wildcard` 优先级；
  - 自动 `HEAD -> GET` 回退与 405 `allowed_mask` 位掩码输出。
