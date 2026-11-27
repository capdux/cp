# @cap/character-entities

Zero-allocation HTML5 named and numeric character reference decoder & encoder in pure CP (covers `character-entities`, `character-entities-legacy`, `character-entities-html4`, `character-reference-invalid`, `decode-named-character-reference`, `parse-entities`, `stringify-entities`, and `micromark-util-decode-numeric-character-reference`).

## API

- `decodeNamedEntity(name: string): u32?`
- `decodeNumericEntity(body: string): u32?`
- `mapInvalidCharRef(cp: u32): u32` — WHATWG HTML5 Windows-1252 C1 remapping (`&#x80;` → `€`, `&#x97;` → `—`, etc.).
- `decodeEntities(input: string, out: []u8): usize`
- `encodeEntities(input: string, out: []u8): usize`

<!-- cap-source-attribution -->
## Source & License Attribution

- **Upstream Ecosystem**: npm (`character-entities@2.0.2`, `decode-named-character-reference@1.1.0`, `parse-entities@4.0.2`, `stringify-entities@4.0.4`)
- **Upstream Repository**: https://github.com/wooorm/character-entities
- **License**: MIT (clean-room CP implementation)
