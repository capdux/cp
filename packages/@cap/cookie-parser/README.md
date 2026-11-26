# `@cap/cookie-parser`

Zero-heap HTTP `Cookie` header parser with HMAC-SHA256 signed cookie (`s:`) verification, multi-secret key rotation, and JSON cookie (`j:`) prefix extraction — clean-room CP equivalent of npm `cookie-parser`.

Reuses `@cap/cookie-jar` and `@cap/cookie-signature` as independent packages.

## Features
- `parseCookieHeader(header, secret)` & `parseCookieHeaderWithSecrets(header, secrets, count)`
- `signCookieValue(val, secret, out)` & `signedCookie(val, secret)` / `signedCookieWithSecrets(...)`
- `isSignedCookie(val)`, `isJsonCookie(val)`, `stripJsonCookiePrefix(val)`
- `lookupRawCookie(header, name)`
- Fixed 32-slot zero-heap `ParsedCookies` separation between unsigned `cookies` and verified `signed_cookies`.
