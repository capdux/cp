# `@cap/pep440-rs`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:pep440_rs` ([https://github.com/astral-sh/uv](https://github.com/astral-sh/uv))（原库名 `pep440_rs` → CP 包名 `@cap/pep440-rs`）

Zero-heap PEP 440 Python version parser, total-order comparator, and version specifier matcher in pure CP, modeled after Astral's `pep440_rs` crate. Supports epochs (`1!2.0`), multi-segment releases, pre-releases (`a`/`b`/`rc`), post-releases (`.postN`/`-N`), dev-releases (`.devN`), local versions (`+local`), exact PEP 440 ordering (`dev < a < b < rc < final < post`), and compound specifiers (`==`, `!=`, `>=`, `<=`, `>`, `<`, `~=`).

零堆分配纯 CP PEP 440 版本解析、精确全序比较与范围约束匹配引擎，完整对标 Astral `pep440_rs`：支持 epoch、多段 release、预发布（`a`/`b`/`rc`）、后发布（`post`）、开发版（`dev`）、局部标识与精确全序（`dev < a < b < rc < final < post`），完整支持 `==`/`!=`/`>=`/`<=`/`>`/`<`/`~=` 与逗号复合约束。

## Installation

```bash
cap add @cap/pep440-rs
```

## API

| Symbol | Description |
| --- | --- |
| `PreKind` | Enum: `None`, `Alpha`, `Beta`, `Rc` |
| `Pep440Version` | Parsed PEP 440 version struct (`valid`, `epoch`, `release`, `pre_*`, `post_*`, `dev_*`) |
| `parsePep440(s: string): Pep440Version` | Parse PEP 440 version string |
| `comparePep440(a, b): i32` | Total-order comparison returning `-1`, `0`, or `1` |
| `matchesSpecifier(ver, spec): bool` | Match version against comma-separated PEP 440 specifiers |

## License

MIT OR Apache-2.0
