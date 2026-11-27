[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# bcp-47

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:bcp-47` ([https://github.com/words/bcp-47](https://github.com/words/bcp-47))（原库名 `bcp-47` → CP 包名 `bcp-47`）

Zero-allocation RFC 5646 / BCP-47 language tag parser, canonicalizer, and fallback locale matcher for CP — clean-room MIT parity with npm `bcp-47` / `bcp-47-match` and Rust `unic-langid`.

零堆分配的 CP RFC 5646 / BCP-47 语言标签解析、规范化大小写与回退匹配引擎，同时兼容 `-` 与 `_` 分隔符，完整识别 `language`、`script`、`region`（ISO-3166 & UN M.49）、`variant`、`extension`（如 `u-ca-gregory`）与 `private_use`（`x-...`）子标签（纯 CP 洁净室 MIT 实现）。

## Install / 安装

```bash
cap add bcp-47
```

## API Reference / 接口一览

| Symbol | Signature | Description |
| :--- | :--- | :--- |
| `Bcp47Tag` | `pub struct { valid, language, script, region, variant, extension, private_use }` | 零拷贝语言标签解析结果结构体 |
| `parseBcp47` | `(raw: string) -> Bcp47Tag` | 解析 RFC 5646 / BCP-47 语言标签（兼容 `-` 与 `_`） |
| `canonicalizeBcp47` | `(tag: Bcp47Tag, out: []u8) -> string` | 按 RFC 5646 规范大小写与 `-` 连接符格式化（如 `"ZH_hans_cn"` → `"zh-Hans-CN"`） |
| `matchLocale` | `(requested: Bcp47Tag, supported: []const string) -> string?` | 按「精确匹配 > 语言+文字 > 语言+地区 > 仅语言」回退挑选最佳 locale |

## Usage / 使用示例

```cp
import { parseBcp47, canonicalizeBcp47, matchLocale } from "bcp-47";

let buf: [64]u8 = undefined;
const tag = parseBcp47("ZH_hans_cn");
const canon = canonicalizeBcp47(tag, buf[0..64]); // "zh-Hans-CN"

const supported: [3]string = ["en-US", "zh-Hans", "zh-Hant-TW"];
const best = matchLocale(tag, supported[0..3]);   // "zh-Hans"
```

## License

MIT
