# @cap/unicode-seg

Zero-allocation UAX #29 Unicode grapheme cluster and word boundary segmentation engine for CP — clean-room MIT parity with Rust `unicode-segmentation`.

零堆分配的 CP UAX #29 Unicode 字素簇与词边界切分引擎，精准处理 CRLF（`\r\n`）、组合重音字符（`e + U+0301`）、变体选择符、Emoji 肤色修饰符、ZWJ 零宽连字序列（`👨‍👩‍👧`）与成对区域指示符国旗（`🇨🇳`/`🇺🇸`），支持零拷贝安全字素截断与中英混合词边界迭代（纯 CP 洁净室 MIT 实现）。

## Install / 安装

```bash
cap add @cap/unicode-seg
```

## API Reference / 接口一览

| Symbol | Signature | Description |
| :--- | :--- | :--- |
| `DecodedRune` | `pub struct { cp: u32, byte_len: usize }` | UTF-8 解码结果 |
| `TextSpan` | `pub struct { start: usize, end: usize, slice: string }` | 零拷贝边界切片 |
| `decodeUtf8At` | `(s: string, offset: usize) -> DecodedRune` | 严格 UTF-8 解码（畸形字节安全返回 `U+FFFD`） |
| `nextGrapheme` | `(s: string, offset: usize) -> TextSpan?` | 迭代下一个 UAX #29 扩展字素簇 |
| `graphemeCount` | `(s: string) -> usize` | 统计字符串字素簇总数 |
| `truncateGraphemes` | `(s: string, max_graphemes: usize) -> string` | 按字素簇安全截断（绝不撕裂 Emoji 或组合字符） |
| `nextWord` / `wordCount` | `(s: string, ...) -> TextSpan? / usize` | 中英混合词边界迭代与词数统计 |

## Usage / 使用示例

```cp
import { graphemeCount, truncateGraphemes, wordCount } from "@cap/unicode-seg";

const n = graphemeCount("Hi👨‍👩‍👧🇨🇳");          // 4 ("H", "i", "👨‍👩‍👧", "🇨🇳")
const safe = truncateGraphemes("Hi👨‍👩‍👧🇨🇳", 3);   // "Hi👨‍👩‍👧"
const wc = wordCount("Hello世界 CP");              // 4 ("Hello", "世", "界", "CP")
```

## License

MIT
