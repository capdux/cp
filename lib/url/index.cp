// Source: Absorbed from /data/dev/bun/src/url/url.cp | Synced: 2026-11-22T12:49:17Z
// ============================================================================
// lib/url/index.cp — WHATWG URL, URLSearchParams & PercentEncoding Module (cap:url)
// ============================================================================

export * from "./percent";
export * from "./searchParams";
export * from "./url";
export * from "./url_";

export {
    isUnreserved,
    defaultPortForScheme,
    normalizePercentEncoding,
    removeDotSegments,
    normalizeOrigin,
} from "./normalize.cp";

export {
    DataUrl,
    percentDecode,
    encodeBase64DataUrl,
    encodePercentDataUrl,
    encodeShortestDataUrl,
    base64Encode,
    base64Decode,
} from "./dataUrl.cp";
