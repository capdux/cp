# cap:http_permissions_policy

> 源码吸收自 `/data/dev/bun/src/http/http.cp`（W3C Permissions-Policy 规范），提供纯 CP、零外部依赖、零堆分配的 `Permissions-Policy` 响应头解析器与特性权限查询器。

## 导入方式

```cp
import { PermissionDirective, PermissionsPolicyIter, findPermission } from "cap:http_permissions_policy";
```

## 核心类型与 API

### `PermissionDirective`

字段：`feature: []const u8`、`allowlist_raw: []const u8`、`is_wildcard: bool`。

- `isDisabled(): bool`：是否为空允许列表 `()` 且非 `*`。
- `allowsAll(): bool`：是否为 `*` 或括号内含 `*`。
- `allowsSelf(): bool`：是否允许同源（`self` 或 `*`）。
- `allowsOrigin(origin: []const u8): bool`：是否允许指定来源（`*` 或双引号包裹的精确匹配 origin）。

### `PermissionsPolicyIter` & `findPermission`

- `PermissionsPolicyIter.init(header)` / `next(*_): PermissionDirective?`：按逗号流式迭代 `feature=(...)`、`feature=*`、`feature=self`，安全处理引号内逗号与括号内空格。
- `findPermission(header, feature): PermissionDirective?`：大小写不敏感查找特性权限指令。

## 使用示例

```cp
import { findPermission } from "cap:http_permissions_policy";

fn example() {
    const hdr = "geolocation=(self \"https://maps.example.com\"), camera=(), microphone=*";
    if (geo := findPermission(hdr, "geolocation")) {
        * = geo.allowsSelf();                          // true
        * = geo.allowsOrigin("https://maps.example.com"); // true
    }
}
```
