# `cap:route_params` — HTTP 路由参数映射表与 URLPath 零分配解析器

> 参考来源：/data/dev/bun/src/router/ParamMap.cp, /data/dev/bun/src/http_types/URLPath.cp | 吸收日期：2026-11-22

源自 Bun `router/ParamMap.cp` 与 `http_types/URLPath.cp`，提供具备 `~init()` 确定性 RAII 释放的动态路由参数表 `ParamMap` 与零堆分配 `URLPath` 路径解析器（扩展名、首段路由、查询串、`.map` SourceMap 识别与资源前缀剥离）。

## 导出符号

- `Param`: `{ name: []const u8, value: []const u8 }`
- `ParamMap`（含 `~init()` RAII）: `init` / `append` / `set` / `find` / `has` / `get` / `count` / `isEmpty` / `clear` / `shrinkRetainingCapacity`
- `URLPath` + `parseURLPath(pathname)`: `extname` / `path` / `pathname` / `first_segment` / `query_string` / `is_source_map` / `isRoot` / `pathWithoutAssetPrefix`

## 上游行为契约与路径解析边界

1. **小表线性扫描优于哈希表 (`ParamMap`)**：
   - HTTP 路由参数通常仅 1~8 个，`ParamMap` 采用连续数组存储配合线性扫描，缓存局部性远优于动态哈希表；`set(name, value)` 存在同名键时原地覆盖、否则追加；`shrinkRetainingCapacity(0)` 适用于按请求复用容器零重分配。
2. **零拷贝 `URLPath` 切片语义**：
   - `parseURLPath(pathname)` 单遍切分 `path`、`extname`、`first_segment` 与 `query_string`，并识别 `.map` SourceMap 请求（`is_source_map`）；所有字段均为源 `pathname` 只读子切片，`pathWithoutAssetPrefix` 安全剥离静态资源前缀（如 `/_next/static/`）而不产生额外拷贝。
3. **RAII 确定性释放**：
   - `ParamMap` 离开作用域自动触发 `~init()` 释放底层参数数组（不释放外部借用的键/值切片内存）。
