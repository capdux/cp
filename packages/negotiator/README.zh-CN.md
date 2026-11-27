[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# negotiator

零堆分配的纯 CP RFC 7231 / RFC 9110 HTTP 内容协商库，支持 `Accept`、`Accept-Encoding`、`Accept-Language` 与 `Accept-Charset` 的 q-value（`0.000..1.000`）与特异性排序。净室移植自 npm `negotiator`。

## 源包溯源与零成本迁移

| 项目 | 说明 |
| :--- | :--- |
| **源平台** | npm |
| **上游原包** | `negotiator` (`jshttp/negotiator`) |
| **CP 包名** | `negotiator` (`v1.0.0`) |
| **许可证** | MIT |

CP 生态保持原包名 `negotiator` 不变，无需 `@cap/*` 前缀或改名：

```typescript
// TypeScript / Node.js (npm: negotiator)
import Negotiator from "negotiator";
const neg = new Negotiator(req);
const enc = neg.encoding(["br", "gzip", "deflate"]);
```

```typescript
// CP（保持原包名 "negotiator"）
import { negotiateEncoding } from "negotiator";
let avail: [3]string = ["br", "gzip", "deflate"];
let enc = negotiateEncoding("gzip;q=0.8, br;q=1.0", avail[0..3]); // "br"
```

## 安装

```bash
cap add negotiator
```

## 快速上手与实战示例

### 1. 高层令牌协商（编码、语言、字符集）

```typescript
import {
    negotiateEncoding,
    negotiateLanguage,
    negotiateCharset,
} from "negotiator";

fn negotiateHeaders(): void {
    // Accept-Encoding
    let encs: [3]string = ["br", "gzip", "identity"];
    if (let best_enc = negotiateEncoding("gzip;q=0.8, br;q=1.0", encs[0..3])) {
        // best_enc == "br"
    }

    // Accept-Language（支持 BCP-47 前缀匹配，如 "en-US" <-> "en"）
    let langs: [3]string = ["en", "zh-CN", "fr"];
    if (let best_lang = negotiateLanguage("zh-CN, en;q=0.9", langs[0..3])) {
        // best_lang == "zh-CN"
    }

    // Accept-Charset
    let charsets: [2]string = ["utf-8", "iso-8859-1"];
    if (let best_cs = negotiateCharset("iso-8859-1;q=0.5, utf-8;q=1.0", charsets[0..2])) {
        // best_cs == "utf-8"
    }
}
```

### 2. 返回候选索引的核心协商函数（含 Media Type）

```typescript
import { bestMediaType, bestEncoding, bestLanguage } from "negotiator";

fn selectMediaType(): void {
    let types: [3]string = ["text/html", "application/json", "text/plain"];
    let idx = bestMediaType("text/*;q=0.5, application/json;q=0.9", types[0..3]);
    if (idx >= 0) {
        let chosen = types[idx as usize]; // "application/json"
    }

    let encs: [2]string = ["gzip", "br"];
    let e_idx = bestEncoding("br, gzip;q=0.8", encs[0..2]); // 1

    let langs: [2]string = ["en-US", "fr"];
    let l_idx = bestLanguage("en;q=0.8, fr;q=0.5", langs[0..2]); // 0
}
```

### 3. 底层加权列表解析（`parseWeightedList`、`parseAcceptList`、`parseQValue`）

```typescript
import {
    QualitySpec,
    NegSpec,
    parseQValue,
    parseWeightedList,
    parseAcceptList,
} from "negotiator";

fn inspectHeader(): void {
    let q = parseQValue("0.75"); // 750（0..1000 千分制整数）

    let mut qspecs: [16]QualitySpec = undefined;
    let n1 = parseWeightedList("br;q=1.0, gzip;q=0.8, *;q=0.1", qspecs[0..16]);

    let mut specs: [32]NegSpec = undefined;
    let n2 = parseAcceptList("text/html, application/json;q=0.9", specs[0..32]);
}
```

## API 速览

### 高层协商函数
- `negotiateEncoding(accept_encoding, available): string?`
- `negotiateLanguage(accept_language, available): string?`
- `negotiateCharset(accept_charset, available): string?`
- `negNegotiateToken(header, available, is_lang): string?`

### 核心索引协商函数（`core` 导出）
- `bestMediaType(header, available): i64`（无匹配返回 `-1`）
- `bestEncoding(header, available): i64`、`bestLanguage(header, available): i64`
- `parseAcceptList(header, out: []NegSpec): usize`

### 解析类型、常量与辅助函数
- 常量：`NEG_MAX_SPECS`（`32`）、`SPEC_WILDCARD`（`1`）、`SPEC_PARTIAL`（`2`）、`SPEC_EXACT`（`4`）
- `QualitySpec { value: string, q_milli: u32, order: usize }`
- `NegSpec { start: usize, len: usize, q_milli: u16, specificity: u8, order: usize }`
- `parseQValue(raw): u32`、`parseWeightedList(header, out: []QualitySpec): usize`、`negTokenSpecificity`、`negTrim`、`negIsWs`
