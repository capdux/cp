# @cap/compressible

Zero-allocation HTTP `Content-Type` compressibility checker in pure CP.

## API

- `stripContentTypeParams(ct: string): string` — strips `; charset=...` parameters and trims ASCII whitespace
- `isCompressible(content_type: string): bool` — case-insensitive compressibility check for MIME types

<!-- cap-source-attribution -->
## Source & License Attribution

- **Upstream Ecosystem**: npm (`compressible@2.0.18`)
- **Upstream Repository**: https://github.com/jshttp/compressible
- **License**: MIT (clean-room CP implementation)
