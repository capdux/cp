// Source: Absorbed from /data/dev/bun/src/http/http.cp (RFC 6265 Cookie Domain/Path Match & Send Policy)

export {
    defaultCookiePath,
    cookieDomainMatches,
    cookiePathMatches,
    CookiePrefixKind,
    detectCookiePrefix,
    validateCookiePrefix,
    canSendCookie,
} from "./cookie_match.cp";
