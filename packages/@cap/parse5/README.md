# @cap/parse5

Clean-room MIT, zero-heap WHATWG HTML5 streaming tokenizer, attribute parser, and plain-text extractor for CP — modeled on npm's `parse5` v7.

## Dependencies
- `@cap/unicase` (`1.0.0`) — case-insensitive HTML5 void element matching.

## API
- `HtmlTokenizer` (`new`, `next`) — yields `HTML_TOK_DOCTYPE`, `START_TAG`, `END_TAG`, `SELF_CLOSING`, `COMMENT`, `TEXT`, `EOF`
- `parseAttributes(raw_attrs, out_attrs): usize`
- `extractTextContent(html, out): usize`
- `isVoidElement(tag): bool`
