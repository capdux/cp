[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# space-separated-tokens

Zero-allocation HTML/SVG space-separated and comma-separated token list parser and serializer in pure CP (covers both `space-separated-tokens` and `comma-separated-tokens`).

## API

- `parseSpaceTokens(input: string, out: []string): usize`
- `hasSpaceToken(input: string, target: string): bool`
- `stringifySpaceTokens(tokens: []const string, out: []u8): usize`
- `parseCommaTokens(input: string, out: []string): usize`
- `stringifyCommaTokens(tokens: []const string, out: []u8): usize`

<!-- cap-source-attribution -->
## Source & License Attribution

- **Upstream Ecosystem**: npm (`space-separated-tokens@2.0.2`, `comma-separated-tokens@2.0.3`)
- **Upstream Repository**: https://github.com/wooorm/space-separated-tokens
- **License**: MIT (clean-room CP implementation)
