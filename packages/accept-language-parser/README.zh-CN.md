[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# accept-language-parser

面向 CP 语言的零堆分配 RFC 9110 / BCP-47 `Accept-Language` 权重解析与 HTTP 语言协商引擎。

对齐 npm 源包 [`accept-language-parser`](https://github.com/opentable/accept-language-parser) 与 crates.io `accept-language`（MIT）。

## 安装与导入

```bash
cap add accept-language-parser
```

```cp
import {
    LangRange,
    parseQuality,
    parseAcceptLanguage,
    matchesTag,
    pickLanguage,
} from "accept-language-parser";
```

## 核心 API

### 数据结构

| 类名 | 字段 | 说明 |
| --- | --- | --- |
| `LangRange` | `tag: string`, `primary: string`, `region: string`, `quality: u16` | 解析后的 BCP-47 语言范围，字段均为零拷贝子切片，`quality` 为 `0..=1000` 定点整数（如 `q=0.85` -> `850`）。 |

### 函数列表

| 函数 | 签名 | 说明 |
| --- | --- | --- |
| `parseQuality` | `(s: string): i32` | 按 RFC 9110 解析 `qvalue`（`0`..`1`，最多 3 位小数）为 `0..=1000`，非法格式返回 `-1`。 |
| `parseAcceptLanguage` | `(header: string, out: []LangRange): usize` | 解析 `Accept-Language` 请求头并按 `quality` 降序稳定排序写入 `out`，返回写入条数。 |
| `matchesTag` | `(range_tag: string, supported_tag: string): bool` | RFC 4647 忽略大小写前缀匹配：`"*"` 匹配任意标签，`"en"` 匹配 `"en"` 与 `"en-US"`。 |
| `pickLanguage` | `(header: string, supported: &[]string): string` | 协商最匹配的受支持语言（支持 `q=0` 显式排除），无匹配时返回空字符串 `""`。 |

## 使用示例

### 1. HTTP 请求语言协商

```cp
import { print } from "cap:io";
import { pickLanguage } from "accept-language-parser";

pub fn main() {
    let header = "zh-CN,zh;q=0.9,en-US;q=0.8,en;q=0.7";
    let supported: [3]string = ["en-US", "zh-Hans", "zh-CN"];
    let best = pickLanguage(header, supported[0..3]);
    print("selected: {}\n", best); // "zh-CN"
}
```

### 2. 解析并遍历按权重排序的语言列表

```cp
import { print } from "cap:io";
import { LangRange, parseAcceptLanguage } from "accept-language-parser";

pub fn main() {
    let ranges: [8]LangRange = undefined;
    let n = parseAcceptLanguage("fr-CA;q=0.7, en-GB;q=0.9, de", ranges[0..8]);
    let i: usize = 0;
    while (i < n) {
        print("{}: primary={} region={} q={}\n",
            ranges[i].tag, ranges[i].primary, ranges[i].region, ranges[i].quality);
        i += 1;
    }
}
```

### 3. 处理 `q=0` 显式排除

```cp
import { print } from "cap:io";
import { pickLanguage } from "accept-language-parser";

pub fn main() {
    let supported: [2]string = ["en-US", "fr-FR"];
    // en-US 被 q=0 明确排除，通配符命中 fr-FR
    let chosen = pickLanguage("en-US;q=0, *;q=0.5", supported[0..2]);
    print("chosen: {}\n", chosen); // "fr-FR"
}
```

## 与源包差异与零拷贝安全亮点

- **零堆分配与零拷贝切片**：`LangRange` 中的 `tag`、`primary`、`region` 直接借用自原始 `header`，写入调用方栈数组 `out: []LangRange`。
- **定点整数权重**：采用 `u16`（`0..=1000`）替代浮点数，避免浮点精度误差，并严格校验 RFC 9110 `qvalue` 语法（最多 3 位小数）。
- **完整 `q=0` 排除语义**：`pickLanguage` 正确处理 `q=0` 拒绝项，即使同时出现通配符 `*` 也不会误选被排除语言。
