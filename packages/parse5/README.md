[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# parse5

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:parse5` ([https://github.com/inikulin/parse5](https://github.com/inikulin/parse5))

Clean-room MIT, zero-heap WHATWG HTML5 streaming tokenizer, attribute parser, and plain-text extractor for CP — modeled on npm's `parse5` v7.

## Dependencies
- `unicase` (`1.0.0`) — case-insensitive HTML5 void element matching.

## API
- `HtmlTokenizer` (`new`, `next`) — yields `HTML_TOK_DOCTYPE`, `START_TAG`, `END_TAG`, `SELF_CLOSING`, `COMMENT`, `TEXT`, `EOF`
- `parseAttributes(raw_attrs, out_attrs): usize`
- `extractTextContent(html, out): usize`
- `isVoidElement(tag): bool`
