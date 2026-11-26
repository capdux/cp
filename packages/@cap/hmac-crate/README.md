# `@cap/hmac-crate`

Zero-heap RFC 2104 streaming HMAC-SHA256 MAC with constant-time `verifySlice`, arbitrary-length key hashing (>64B), and hex output — clean-room CP equivalent of crates.io `hmac`.

Reuses `@cap/cookie-signature` (`sha256` & `ctEqual`) as an independent package.
