# CP 标准库：OpenAPI 3.1.0 规范构建、路径路由与请求契约校验引擎手册 (`cap:openapi` / `cap.openapi`)

> [!NOTE]
> 本子模块已归并至统一主包 `cap:web`，导入请使用 `import { ... } from "cap:web"`。

## 一、 架构总览与设计原则

`cap:openapi` 是 CP 语言官方内置的纯原生 **OpenAPI 3.1.0** 规范建模、路径模板匹配、参数提取、组件复用、RFC 8259 合规 JSON 导出与请求契约校验引擎，专为 **CP 后端 API 网关** 与 **AI Agent Tool / Function 契约校验**打造。

全模块 **零 C 依赖、零 `std.posix`、零 `@import("bun")`**，可与 `cap:json`、`cap:schema` 无缝协同，支持 `import { ... } from "cap:web"` 与 `import cap from "cap"; cap.openapi.*` 动态懒挂载调用。

### 模块文件拓扑

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/openapi/spec.cp` | `// Source: cp/lib/openapi/spec.cp (OpenAPI 3.1.0 Specification AST, Builder & JSON Serializer)` | 核心错误/枚举、`OpenApiDoc` Arena 模型、路径/状态码/`operationId` 校验、组件复用与 `toJson()` |
| `lib/openapi/router.cp` | `// Source: cp/lib/openapi/router.cp (OpenAPI 3.1.0 Path Template Matcher & Request Contract Validator)` | `OpenApiValidator`：静态段优先路径模板匹配、404/405 判别与请求参数/Body 契约校验 |
| `lib/openapi/index.cp` | `// Source: cp/lib/openapi/index.cp (Unified Exports for cap:openapi)` | `cap:openapi` 与 `cap.openapi` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误与枚举（`spec.cp`）

- **`OpenApiError`**：`InvalidPath`、`DuplicateOperationId`、`InvalidStatusCode`、`RouteNotFound`、`MethodNotAllowed`、`MissingRequiredParam`、`InvalidParamType`、`OutOfMemory`。
- **`HttpMethod`**：`Get`、`Post`、`Put`、`Patch`、`Delete`、`Head`、`Options`（提供 `asStr()` 小写、`toUpperString()` 大写、`fromString(s)` 大小写不敏感解析）。
- **`ParamIn`**：`Path`、`Query`、`Header`、`Cookie`。
- **`SecuritySchemeType`**：`ApiKey`、`HttpBearer`、`HttpBasic`、`OAuth2`。

### 2. 规范模型与构造器（`spec.cp`）

- **`OpenApiInfo`**：`init(title, version)`，链式方法 `withSummary`、`withDescription`、`withContact`、`withLicense(name, identifier, url)`。
- **`OpenApiServer` / `OpenApiTag`**：`init(...)` 描述服务端点与分组标签。
- **`OpenApiParam`**：
  - 便捷构造：`path(name, schema_type)`（强制 `required = true`）、`query(name, required, schema_type)`、`header(name, required, schema_type)`、`cookie(name, required, schema_type)`；
  - 链式扩展：`withDescription`、`withFormat`、`withSchemaJson`、`withDeprecated`。
- **`OpenApiResponse`**：`init(status_code, description)`、`jsonRef(status_code, description, schema_name)`、`jsonSchema(status_code, description, schema_json)`。
- **`OpenApiSecurityScheme`**：`bearer(name, bearer_format)`、`basic(name)`、`apiKey(name, param_name, key_in)`、`oauth2(name, flows_json)`、`withDescription`。
- **`OpenApiOperation`**：`init(operation_id)`，链式配置 `withSummary`、`withDescription`、`withTags`、`withParams`、`withResponses`、`withSecurity`、`withDeprecated`、`withRequestBodyRequired`、`withRequestBodyRef`、`withRequestBodyJson`。
- **`OpenApiDoc`**：
  - `init(alloc, info)` / `deinit()`：内部由独立 `std.heap.ArenaAllocator` 管理所有拷贝字符串与端点元数据，$O(1)$ 释放；
  - `addServer(url, description)`、`addTag(name, description)`、`addSecurityScheme(scheme)`、`registerComponentSchema(name, json_schema_str)`；
  - `addOperation(path, method, op)`：自动校验路径以 `/` 开头、`{param}` 括号闭合且非空不嵌套不重复、HTTP 状态码合法性及 `operationId` 全局唯一性；
  - `toJson(alloc)`：按插入路径聚合多 HTTP 方法并输出标准 OpenAPI `3.1.0` JSON 文档。

### 3. 路径模板匹配与请求契约校验器（`router.cp`）

- **`OpenApiValidator.init(&doc)`**：
  - `matchRoute(alloc, method, raw_path): RouteMatch!`：
    - 剥离 `?query` / `#fragment`，按段匹配模板并遵循 **静态段 > 混合段 > `{param}` 参数段** 优先级；
    - 提取 `path_params`（`KeyValuePair` 切片，支持 `getParam(name)`），准确区分 `OpenApiError.RouteNotFound` 与 `OpenApiError.MethodNotAllowed`。
  - `matchRouteEx(alloc, method, raw_path): RouteMatchResult!` & `allowedMethods(alloc, raw_path): ([]HttpMethod)!`：
    - 在 405 `MethodNotAllowed` 场景下同步返回当前路径模板允许的全部 `allowed_methods` 列表。
  - `validateRequest(alloc, method, raw_path, query_params, headers, has_body): ValidationReport!`：
    - 自动完成路由匹配、必填 `path` / `query` / `header`（大小写不敏感）/ `cookie` 参数缺失检查、`integer` / `number` / `boolean` 基础类型格式校验及 `request_body_required` 检查；
    - 返回结构化 `ValidationReport`（含 `valid`、`status_code`、`operation_id`、`path_params`、`allowed_methods` 与 `errors` 明细，调用 `report.deinit()` 释放）。

---

## 三、 使用示例

```typescript
const std = @import("std");
import cap from "cap";
import {
    OpenApiInfo,
    OpenApiDoc,
    OpenApiParam,
    OpenApiResponse,
    OpenApiOperation,
    OpenApiSecurityScheme,
    OpenApiValidator,
    KeyValuePair,
} from "cap:web";

pub fn buildAndValidate(): ! {
    const alloc = std.heap.page_allocator;

    let doc = OpenApiDoc.init(alloc, OpenApiInfo.init("Agent API", "1.0.0"))?;
    defer doc.deinit();

    doc.addSecurityScheme(OpenApiSecurityScheme.bearer("bearerAuth", "JWT"))?;
    doc.registerComponentSchema("User", "{\"type\":\"object\",\"properties\":{\"id\":{\"type\":\"string\"}}}")?;

    const params: [2]OpenApiParam = [
        OpenApiParam.path("id", "integer"),
        OpenApiParam.query("verbose", false, "boolean"),
    ];
    const resps: [1]OpenApiResponse = [
        OpenApiResponse.jsonRef("200", "User found", "User"),
    ];
    const op = OpenApiOperation.init("getUser")
        .withParams(&params)
        .withResponses(&resps);
    doc.addOperation("/users/{id}", .Get, op)?;

    const json_spec = doc.toJson(alloc)?;
    defer alloc.free(json_spec);

    const validator = cap.openapi.OpenApiValidator.init(&doc);
    const q: [1]KeyValuePair = [KeyValuePair{ key: "verbose", value: "true" }];
    let report = validator.validateRequest(alloc, .Get, "/users/42", &q, [], false)?;
    defer report.deinit();
}
```
