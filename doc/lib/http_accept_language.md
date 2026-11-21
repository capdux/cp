# cap:http_accept_language

> 源码吸收自 `/data/dev/bun/src/http/`（RFC 9110 §12.5.4 + RFC 4647 HTTP `Accept-Language` 语言协商规范），提供纯 CP、零外部依赖、零堆分配的 BCP 47 语言范围解析、前缀匹配与权重协商库。

## 导入方式

```cp
import {
    LanguageRange,
    AcceptLanguageIter,
    negotiateLanguage,
} from "cap:http_accept_language";
```

## 核心类型与 API

- `LanguageRange`（`tag: []const u8`, `q_permille: u16`）：
  - `q_permille` 以 `0..=1000` 整数千分比表示权重（默认 `1000`），避免浮点精度误差。
  - `isWildcard(): bool`：判断是否为 `"*"` 通配符。
  - `matchesTag(candidate: []const u8): bool`：RFC 4647 大小写不敏感基本前缀匹配（`"en"` 匹配 `"en"` / `"en-US"`，不匹配 `"eng"`；`"*"` 匹配任意非空标签）。
- `AcceptLanguageIter`：
  - `init(header: []const u8): AcceptLanguageIter`
  - `next(*_): LanguageRange?`：零分配解析逗号分隔条目及 `;q=0.8` 权重，严格校验 BCP 47 子标签（`1*8ALPHA *("-" 1*8alphanum)` / `"*"`）与 `0.000..=1.000` 权重范围，自动跳过非法项。
- `negotiateLanguage(header: []const u8, supported: []const []const u8): ([]const u8)?`：按 RFC 9110 规则为 `supported` 计算最具体匹配标签与最高权重，排除 `q=0` 显式拒绝项，返回最佳匹配语言；无匹配返回 `null`。

## 使用示例

```cp
import { negotiateLanguage } from "cap:http_accept_language";

fn example() {
    const supported: [3][]const u8 = ["en-US", "zh-CN", "ja-JP"];
    * = negotiateLanguage("fr-FR;q=0.9, zh;q=0.8, en;q=0.7", &supported); // "zh-CN"
}
```
