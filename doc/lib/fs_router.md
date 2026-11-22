# `cap:fs_router` — 零堆分配文件系统路由解析与匹配引擎

> 参考来源：/data/dev/bun/src/router/{Route,route_types}.cp, /data/dev/bun/docs/runtime/file-system-router.mdx | 吸收日期：2026-11-22

`cap:fs_router` 提供纯 CP、零外部依赖、零堆分配的 Next.js / Bun 风格文件系统路由解析、优先级排序与零拷贝参数提取引擎，支持静态路由、动态参数 `[id]`、必选通配 `[...slug]` 与可选通配 `[[...cats]]`。

## 核心类型

### `RouteKind` & `SegmentKind`

```cp
pub enum RouteKind: u8 {
    static_route = 0,
    dynamic = 1,
    catch_all = 2,
    optional_catch_all = 3,

    fn asStr(): string
}

pub enum SegmentKind: u8 {
    literal = 0,
    param = 1,
    catch_all = 2,
    optional_catch_all = 3,
}
```

### `RouteParam` & `RouteMatch`

```cp
pub struct RouteParam {
    name: []const u8 = "",
    value: []const u8 = "",
}

pub struct RouteMatch {
    route_idx: usize = 0,
    params: [8]RouteParam = [RouteParam{}; 8],
    param_len: usize = 0,

    fn getParam(param_name: []const u8): ([]const u8)?
}
```

### `RoutePattern`

```cp
pub struct RoutePattern {
    raw: []const u8 = "",
    kind: RouteKind = .static_route,
    param_count: usize = 0,
    segments: [16]RouteSegment = [RouteSegment{}; 16],
    seg_len: usize = 0,

    static fn parse(pattern: []const u8): RoutePattern?
    fn match(pathname: []const u8): RouteMatch?
    fn comparePriority(other: RoutePattern): i8
}
```

- `RoutePattern.parse(pattern)`：自动归一化尾斜杠；校验 `catch-all` 必须位于末段、参数名非空、无未闭合括号与空段 `//`。
- `match(pathname)`：自动剥离查询串 `?` / 锚点 `#` 与尾斜杠，零拷贝提取所有命名参数。
- `comparePriority(other)`：遵循 `static_route > dynamic > catch_all > optional_catch_all`，同级别按前缀段具体度与字母序裁决（返回 `-1` 表示当前路由优先级更高）。

### `FsRouter`

```cp
pub struct FsRouter {
    routes: [64]RoutePattern = [RoutePattern{}; 64],
    len: usize = 0,

    static fn init(): FsRouter
    fn addRoute(*_, pattern: []const u8): usize?
    fn matchPath(pathname: []const u8): RouteMatch?
}
```

- 固定容量 64 条路由，零堆分配；无论注册顺序如何，`matchPath` 始终返回最高优先级命中项及其稳定 `route_idx`。

## 使用示例

```cp
import { FsRouter } from "cap:fs_router";

let router = FsRouter.init();
const dyn_idx = router.addRoute("/users/[id]").?;
const static_idx = router.addRoute("/users/profile").?;

const m1 = router.matchPath("/users/profile").?; // m1.route_idx == static_idx（静态优先）
const m2 = router.matchPath("/users/42").?;      // m2.route_idx == dyn_idx, m2.getParam("id").? == "42"
```

## 上游行为契约与路由边界注意事项

1. **Next.js 风格路由优先级仲裁（与注册顺序无关）**：
   - 继承自 Bun `FileSystemRouter` 的确定性优先级规则：`static_route` > `dynamic` (`[id]`) > `catch_all` (`[...slug]`) > `optional_catch_all` (`[[...cats]]`)；
   - `FsRouter.matchPath` 全量扫描已注册模式并按 `RoutePattern.comparePriority` 裁决，即使先注册 `/users/[id]` 再注册 `/users/profile`，访问 `/users/profile` 仍恒定命中静态路由。
2. **URL 路径归一化与零拷贝边界**：
   - `RoutePattern.match` 自动剥离查询串 `?`、锚点 `#` 及多余尾斜杠；返回的 `RouteParam.name` / `value` 均为输入切片零拷贝视图，调用方需保证源 `pathname` 切片生命周期覆盖 `RouteMatch` 使用期。
3. **固定容量与畸形模式防御**：
   - `RoutePattern.parse` 严格拒绝空参数名（`[]`）、未闭合括号、空段 `//` 与非末段通配符；
   - 单条路由最多 16 段、单次匹配最多提取 8 个命名参数，`FsRouter` 固定容量 64 条路由，超限时 `addRoute` 安全返回 `null`（零堆分配、无 Panic）。
