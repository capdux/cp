[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# type-is

适用于 CP 语言的零堆分配 HTTP `Content-Type` 匹配库，对齐 npm [`type-is`](https://github.com/jshttp/type-is)（MIT 协议，CP v1.0.0，底层复用 `mime-types` 与 `unicase`）。

支持扩展名简写（`"json"`、`"html"`、`"urlencoded"`、`"multipart"`）、MIME 通配符（`"text/*"`、`"*/json"`、`"*/*"`）、结构化后缀（`"+json"`、`"+xml"`）以及 HTTP 请求体存在性判断（`hasBody` / `requestHasBody` / `typeis`）。

## 安装与导入

```bash
cap add type-is
```

```cp
import {
    RequestHeaders,
    is,
    typeIs,
    typeis,
    hasBody,
    normalize,
    isJson,
} from "type-is";
```

## 核心 API 参考

### 数据结构

| 类型 | 字段 | 说明 |
| :--- | :--- | :--- |
| `RequestHeaders` | `content_type: string`, `transfer_encoding: string`, `content_length: string` | 供 `requestHasBody`、`typeis`、`typeisCsv` 使用的轻量 HTTP 请求头元组。 |

### 匹配与规范化函数

| 函数 | 签名 | 说明 |
| :--- | :--- | :--- |
| `is` | `(content_type: string, types: &[]string): string?` | 按顺序匹配候选模式切片，返回首个命中的模式（`types` 为空时返回规范化 essence），无匹配返回 `null`。 |
| `typeIs` | `(content_type: string, candidates_csv: string): string?` | 同 `is`，接收逗号分隔的候选模式字符串（如 `"json, +json, text/*"`）。 |
| `typeis` | `(req: RequestHeaders, types: &[]string): string?` | 先通过 `requestHasBody(req)` 校验请求是否带 Body；无 Body 直接返回 `null`，否则匹配 `req.content_type`。 |
| `typeisCsv` | `(req: RequestHeaders, candidates_csv: string): string?` | 带请求体守卫的 CSV 候选模式匹配。 |
| `hasBody` | `(transfer_encoding: string, content_length: string): bool` | 当 `transfer_encoding` 或 `content_length` 非空时返回 `true`。 |
| `requestHasBody` | `(req: RequestHeaders): bool` | 对 `RequestHeaders` 执行 `hasBody` 判定。 |
| `typeMatches` | `(content_type: string, pattern: string): bool` | 判断单个 `Content-Type` 是否匹配指定模式（简写/通配符/`+suffix`）。 |
| `matchType` | `(expected: string, actual: string): bool` | 对规范化后的期望模式与实际 MIME essence 执行匹配。 |
| `normalize` | `(pattern: string): string?` | 严格对齐 npm `type-is.normalize`；空串或未知扩展名返回 `null`。 |
| `normalizeTypePattern` | `(pattern: string): string` | 宽松规范化：空串返回 `""`，未知模式返回去空白原串。 |
| `extractEssence` | `(content_type: string): string?` | 提取 `type/subtype` 主体，自动剥离 `; charset=utf-8` 等参数。 |
| `isValidContentType` | `(content_type: string): bool` | 判断 `Content-Type` 是否可解析出合法 MIME essence。 |
| `isJson` / `isText` / `isForm` / `isMultipart` | `(content_type: string): bool` | 常用 HTTP 媒体类型快捷判断函数。 |

## 代码示例

### 1. 使用候选切片匹配 `Content-Type`（`is`）

```cp
import { is } from "type-is";

fn routeBody(ct: string): string {
    let candidates = ["urlencoded", "json", "+json", "text/*"];
    let matched = is(ct, &candidates);
    if (matched != null) {
        return matched.?;
    }
    return "unsupported";
}
// routeBody("application/vnd.api+json; charset=utf-8") -> "+json"
```

### 2. 请求体守卫 + 类型匹配（`typeis`）

```cp
import { RequestHeaders, typeis } from "type-is";

fn handleRequest(req: RequestHeaders): bool {
    let allowed = ["json", "application/*+json"];
    let matched = typeis(req, &allowed);
    // 若 Content-Length 与 Transfer-Encoding 均为空，直接返回 null
    return matched != null;
}
```

### 3. CSV 模式与快捷判断

```cp
import { typeIs, isJson, isMultipart, normalize } from "type-is";

fn quickChecks() {
    let m = typeIs("text/html; charset=utf-8", "json, html, text/*"); // "html"
    let ok = isJson("application/problem+json"); // true
    let mp = isMultipart("multipart/form-data; boundary=abc"); // true
    let norm = normalize("urlencoded"); // "application/x-www-form-urlencoded"
}
```

## 与源包差异与零拷贝安全亮点

- **零堆分配切片匹配**：所有去空白、参数剥离、大小写无关比较均直接在原始 `string` 切片上完成，复用 `cap:ascii`、`mime-types` 与 `unicase`。
- **统一可选返回类型**：使用 CP 的 `string?`（未匹配或无 Body 均返回 `null`），消除 JS 版 `string | false | null` 三态联合返回歧义。
- **轻量 `RequestHeaders` 结构体**：用 3 字段值结构体替代 Node.js `IncomingMessage` 对象，适合任意 HTTP 框架集成。
