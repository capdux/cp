// Source: Absorbed from /data/dev/bun/src/url.zig (RFC 3986 URI Normalization)

export {
    isUnreserved,
    defaultPortForScheme,
    normalizePercentEncoding,
    removeDotSegments,
    normalizeOrigin,
} from "./normalize.cp";
