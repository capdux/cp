# cap:http_security_headers

> 源码吸收自 `/data/dev/bun/src/http/http.cp`，提供纯 CP、零外部依赖、零堆分配的 Web 安全响应头（HSTS、`Referrer-Policy`、`X-Frame-Options`、`COOP`/`COEP`/`CORP`）解析、校验与格式化库。

## 导入方式

```cp
import {
    HstsPolicy,
    ReferrerPolicy,
    parseReferrerPolicy,
    FrameOptions,
    parseFrameOptions,
    CoopPolicy,
    parseCoopPolicy,
    CoepPolicy,
    parseCoepPolicy,
    CorpPolicy,
    parseCorpPolicy,
    isCrossOriginIsolated,
} from "cap:http_security_headers";
```

## 核心类型与 API

- `HstsPolicy`：`max_age: u64`、`include_subdomains: bool`、`preload: bool`；提供 `parse()`、`isPreloadEligible()`、`format(out)`。
- `ReferrerPolicy` & `parseReferrerPolicy(header)`：完整覆盖 8 种标准策略，支持逗号分隔回退链（取最后一个合法值）。
- `FrameOptions` & `parseFrameOptions(header)`：`DENY` / `SAMEORIGIN`。
- `CoopPolicy` / `CoepPolicy` / `CorpPolicy` 与对应 `parse*`（COOP/COEP 自动剥离 `;report-to=...` 参数）及 `isCrossOriginIsolated(coop, coep)`。

## 使用示例

```cp
import { HstsPolicy, parseCoopPolicy, parseCoepPolicy, isCrossOriginIsolated } from "cap:http_security_headers";

fn example() {
    if (hsts := HstsPolicy.parse("max-age=63072000; includeSubDomains; preload")) {
        * = hsts.isPreloadEligible(); // true
    }
    const coop = parseCoopPolicy("same-origin") ?? unreachable;
    const coep = parseCoepPolicy("require-corp") ?? unreachable;
    * = isCrossOriginIsolated(coop, coep); // true
}
```
