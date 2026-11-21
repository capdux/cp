// Source: Absorbed from /data/dev/bun/src/http/http.cp (HTTP Security Headers HSTS / Referrer / COOP / COEP)

export {
    HstsPolicy,
    ReferrerPolicy,
    parseReferrerPolicy,
    FrameOptions,
    parseFrameOptions,
    CoopPolicy,
    parseCoopPolicy,
    CoepPolicy,
    parseCoepPolicy,
    CorpPolicy,
    parseCorpPolicy,
    isCrossOriginIsolated,
} from "./security_headers.cp";
