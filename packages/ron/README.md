[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/ron

Zero-heap Rusty Object Notation (RON) lexer and top-level struct field extractor in pure CP.

## API

- `RonTokenKind` — token kind enum (`Ident`, `String`, `Int`, `Float`, `Bool`, `LParen`, `RParen`, `LBracket`, `RBracket`, `LBrace`, `RBrace`, `Colon`, `Comma`, `Eof`, `Invalid`)
- `RonToken` — token slice record (`kind`, `text`, `start`, `end`)
- `RonLexer.new(src)` / `nextToken(*_): RonToken` — skips whitespace, `//` line comments, and `/* ... */` nested block comments; lexes identifiers, booleans, integers, floats, escaped strings, and delimiters
- `ronGetField(ron_src, field_name): string?` — zero-allocation top-level struct field value extractor

<!-- cap-source-attribution -->
## Source & License Attribution

- **Upstream Ecosystem**: crates.io (`ron@0.8.1`)
- **Upstream Repository**: https://github.com/ron-rs/ron
- **License**: MIT (clean-room CP implementation)
