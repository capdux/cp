[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent 速查手册](README_AGENT.md)

# type-is

面向 CP 的零堆内存 HTTP `Content-Type` 匹配库，完整支持简写扩展名（`json`、`html`、`urlencoded`、`multipart`）、通配符（`text/*`、`*/*`）、结构化后缀（`+json`、`application/*+json`）以及请求体检测（`hasBody` / `requestHasBody`）。

## 源包溯源与零成本迁移

| 属性 | 说明 |
| --- | --- |
| **上游源平台** | [npmjs.com/package/type-is](https://www.npmjs.com/package/type-is) |
| **上游原包名** | `type-is`（CP 版本 `1.0.0`） |
| **上游仓库** | https://github.com/jshttp/type-is |
| **许可证** | MIT |

从 TypeScript/npm 迁移到 CP 保持原包名不变：

```ts
// TypeScript (npm)
import typeis from "type-is";
const matched = typeis(req, ["json", "application/*+json", "urlencoded"]);
```

```cp
// CP —— 原包名直接导入，零堆 HTTP Content-Type 协商匹配
import {
    RequestHeaders, typeis, typeisCsv, is, typeIs,
    hasBody, requestHasBody, isJson,
} from "type-is";
```

## 安装

```bash
cap add type-is
```

## 实战示例

### 1. 请求级内容类型匹配与请求体检测（`typeis`、`typeisCsv`、`requestHasBody`）

```cp
import {
    RequestHeaders, typeis, typeisCsv,
    hasBody, requestHasBody,
} from "type-is";

fn handleRequest() {
    let req = RequestHeaders {
        content_type: "application/vnd.api+json; charset=utf-8",
        transfer_encoding: "",
        content_length: "128",
    };

    if (requestHasBody(req)) {
        let candidates: [3]string = ["urlencoded", "+json", "text/*"];
        if (typeis(req, &candidates)) |matched| {
            // matched == "+json"
        }

        // 逗号分隔候选列表快捷版
        if (typeisCsv(req, "html, application/*+json, text/*")) |m2| {
            // m2 == "application/*+json"
        }
    }

    // 独立请求体检测（存在 transfer_encoding 或 content_length > 0 时返回 true）
    let body_present = hasBody("chunked", ""); // true
}
```

### 2. 直接 `Content-Type` 匹配、规范化与快捷谓词

```cp
import {
    is, typeIs, typeMatches, matchType,
    normalize, normalizeTypePattern, extractEssence, isValidContentType,
    isJson, isText, isForm, isMultipart,
} from "type-is";

fn demoMatchers() {
    let ct = "application/json; charset=utf-8";

    // 提取核心 MIME 类型并校验格式
    if (extractEssence(ct)) |essence| {
        // essence == "application/json"
    }
    let valid = isValidContentType(ct); // true

    // 依托 mime-types 展开简写（"json" -> "application/json"）
    if (normalize("json")) |norm| {
        // norm == "application/json"
    }
    let pat = normalizeTypePattern("urlencoded"); // "application/x-www-form-urlencoded"

    // 切片候选或 CSV 候选匹配
    let list: [2]string = ["html", "json"];
    if (is(ct, &list)) |hit| {
        // hit == "json"
    }
    let csv_hit = typeIs(ct, "text/*, json"); // "json"

    // 单模式匹配与常用快捷判断
    let m1 = typeMatches(ct, "application/*");                       // true
    let m2 = matchType("application/*+json", "application/ld+json"); // true
    let j = isJson(ct);                                              // true
    let t = isText("text/plain; charset=utf-8");                     // true
    let f = isForm("application/x-www-form-urlencoded");             // true
    let mp = isMultipart("multipart/form-data; boundary=x");         // true
}
```

## 性能与安全亮点

- **零堆分配**：解析、裁剪、提取 MIME essence 及忽略大小写比对全部基于借用字符串切片完成，无堆内存开销。
- **RFC 兼容大小写无关匹配**：复用 `unicase` 与 `mime-types`，准确识别 `Application/JSON; Charset=UTF-8` 等各类变体。
- **畸形报文安全防护**：空或非法 `Content-Type` 与无请求体场景均安全返回 `null`/`false`，不会崩溃或越界。

## API 参考

完整类型定义见 [`src/index.d.cp`](src/index.d.cp)，AI 智能体极简速查见 [`README_AGENT.md`](README_AGENT.md)。
