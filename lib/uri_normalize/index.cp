// Source: Absorbed from /data/dev/bun/src/url.zig | Synced: 2026-11-21T14:47:39Z (RFC 3986 URI Normalization)

export {
    isUnreserved,
    defaultPortForScheme,
    normalizePercentEncoding,
    removeDotSegments,
    normalizeOrigin,
} from "./normalize.cp";
