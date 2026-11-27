# @cap/type-is

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:type-is` ([https://github.com/jshttp/type-is](https://github.com/jshttp/type-is))

Clean-room MIT, zero-heap HTTP `Content-Type` matcher for CP — modeled on npm's `type-is` v1.6.

## Dependencies
- `@cap/mime-types` (`1.0.0`) — shorthand extension lookup and MIME header parsing
- `@cap/unicase` (`1.0.0`) — case-insensitive MIME type/subtype matching

## API
- `typeIs(content_type, candidates_csv): string?`
- `typeMatches(content_type, pattern): bool`
- `normalizeTypePattern(pattern): string`
- `hasBody(transfer_encoding, content_length): bool`
