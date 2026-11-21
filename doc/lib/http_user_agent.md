# `cap:http_user_agent` — RFC 9110 `User-Agent` Parser & Classifier

Zero-allocation RFC 9110 §10.1.5 `User-Agent` product token iterator (skipping nested comments) and browser/OS/mobile/bot classifier.

## Exports

- `ProductToken` & `UserAgentIter` (`init(ua)`, `next(*_): ProductToken?`)
- `BrowserFamily`, `OsFamily`
- `isBotUserAgent(ua): bool`, `isMobileUserAgent(ua): bool`
- `detectBrowser(ua): BrowserFamily`, `detectOs(ua): OsFamily`
- `findProductVersion(ua, product_name): ([]const u8)?`
