# `cap:http_client_hints` — W3C `Sec-CH-UA` Client Hints Parser

Zero-allocation W3C User-Agent Client Hints parser for `Sec-CH-UA`, `Sec-CH-UA-Mobile`, and `Sec-CH-UA-Platform`, with GREASE brand detection.

## Exports

- `BrandVersion` (`brand`, `version`, `isGrease(): bool`)
- `BrandListIter` (`init(header)`, `next(*_): BrandVersion?`)
- `findBrandVersion(sec_ch_ua, brand_name): ([]const u8)?`
- `parseMobileHint(sec_ch_ua_mobile): bool?`
- `parsePlatformHint(sec_ch_ua_platform): ([]const u8)?`
