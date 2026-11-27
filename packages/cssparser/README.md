[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/cssparser

Zero-heap CSS Syntax Level 3 lexer and tokenizer (`CssTokKind`, `CssToken`, `CssLexer`) in pure CP.

## API

- `CssTokKind` enum (`Ident`, `AtKeyword`, `Hash`, `String`, `Number`, `Percentage`, `Dimension`, `Function`, `Colon`, `Semicolon`, `Comma`, `LBrace`, `RBrace`, `LParen`, `RParen`, `LBracket`, `RBracket`, `Delim`, `Eof`)
- `CssToken { kind, text, unit, start, end }`
- `CssLexer.new(src: string): CssLexer` / `nextToken(): CssToken`

<!-- cap-source-attribution -->
## Source & License Attribution

- **Upstream Ecosystem**: crates.io (`cssparser@0.34.0`)
- **Upstream Repository**: https://github.com/servo/rust-cssparser
- **License**: MIT (clean-room CP implementation)
