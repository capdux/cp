# `cap:route_params` — HTTP 路由参数映射表与 URLPath 零分配解析器

源自 Bun `router/ParamMap.cp` 与 `http_types/URLPath.cp`，提供具备 `~init()` 确定性 RAII 释放的动态路由参数表 `ParamMap` 与零堆分配 `URLPath` 路径解析器（扩展名、首段路由、查询串、`.map` SourceMap 识别与资源前缀剥离）。

## 导出符号

- `Param`: `{ name: []const u8, value: []const u8 }`
- `ParamMap`（含 `~init()` RAII）: `init` / `append` / `set` / `find` / `has` / `get` / `count` / `isEmpty` / `clear` / `shrinkRetainingCapacity`
- `URLPath` + `parseURLPath(pathname)`: `extname` / `path` / `pathname` / `first_segment` / `query_string` / `is_source_map` / `isRoot` / `pathWithoutAssetPrefix`
