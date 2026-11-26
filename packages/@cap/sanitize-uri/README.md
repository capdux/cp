# @cap/sanitize-uri

Zero-allocation safe URI protocol sanitizer and RFC 3986 percent-encoder in pure CP (clean-room implementation of `micromark-util-sanitize-uri`).

## API

- `isSafeUri(uri: string): bool` — allows relative URLs and safe protocols (`http`, `https`, `mailto`, `tel`, `irc`, `ircs`, `xmpp`, `ftp`), while blocking `javascript:`, `vbscript:`, `data:`, `file:`, and control-character obfuscated schemes.
- `sanitizeUri(uri: string, out: []u8): usize` — returns `0` for disallowed schemes, otherwise writes percent-encoded URI to `out` (preserving valid `%XX` escapes).

<!-- cap-source-attribution -->
## Source & License Attribution

- **Upstream Ecosystem**: npm (`micromark-util-sanitize-uri@2.0.1`)
- **Upstream Repository**: https://github.com/micromark/micromark
- **License**: MIT (clean-room CP implementation)
