# `cap:jwt_claims` — RFC 7519 JWT Structure & Claims Inspector

Zero-allocation JWT compact JWS splitter, Base64URL decoder, header/payload claim extractor, and time-window/identity claim validator (with `alg: "none"` detection and clock-skew leeway).

## Exports

- `splitJwt(token): JwtParts?` — Splits `header.payload.signature` and returns `signing_input`.
- `decodeBase64Url(input, out): ([]const u8)?` — Unpadded/padded Base64URL decoder.
- `extractStringClaim(json, key): ([]const u8)?` / `extractIntClaim(json, key): i64?` — Top-level JSON object claim extractors (safely skips nested objects/arrays).
- `JwtHeader` — `parseJson`, `isNoneAlg()` (`alg`, `typ`, `kid`).
- `ClaimStatus` & `JwtClaims` — `parseJson`, `validateTime(now, leeway)`, `validate(now, leeway, iss, sub, aud)`.
