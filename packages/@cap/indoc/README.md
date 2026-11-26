# @cap/indoc

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:indoc` ([https://github.com/dtolnay/indoc](https://github.com/dtolnay/indoc))

Zero-allocation multiline string `dedent`, `indent`, `stripMargin`, `trimBlankLines`, and newline normalization engine for CP — clean-room MIT parity with Rust `indoc`, npm `dedent`, and Kotlin `trimMargin`.

零堆分配的 CP 多行文本去缩进与模板对齐引擎，专为 SQL / HTML / YAML / LLM Prompt 嵌入式多行字符串打造，支持公共最小缩进剥离、相对嵌套缩进保留、Kotlin/Scala 风格 `stripMargin('|')` 与 CRLF 换行规范化（纯 CP 洁净室 MIT 实现）。

## Install / 安装

```bash
cap add @cap/indoc
```

## API Reference / 接口一览

| Symbol | Signature | Description |
| :--- | :--- | :--- |
| `minCommonIndent` | `(input: string) -> usize` | 计算全部非空白行的最小公共前导空格/制表符数 |
| `dedent` | `(input: string, out: []u8) -> string` | 剥离首尾单行空行与公共最小缩进，保留相对嵌套并清空纯空白行 |
| `indent` | `(input: string, prefix: string, skip_empty: bool, out: []u8) -> string` | 为每行添加统一前缀（可选跳过空行） |
| `stripMargin` | `(input: string, margin_char: u8, out: []u8) -> string` | 剥离每行前导空白及紧随的边界符（如 `'|'`） |
| `normalizeNewlines` | `(input: string, out: []u8) -> string` | 统一 `\r\n` 与 `\r` 为 `\n` |
| `trimBlankLines` | `(input: string) -> string` | 零拷贝切片剥离首尾全部空行 |

## Usage / 使用示例

```cp
import { dedent, indent, stripMargin } from "@cap/indoc";

let out: [512]u8 = undefined;
const sql = dedent("\n    SELECT *\n      FROM users\n    WHERE active = 1\n", out[0..512]);
// => "SELECT *\n  FROM users\nWHERE active = 1"
```

## License

MIT
