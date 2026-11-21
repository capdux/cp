// Source: Absorbed from /data/dev/bun/src/http/ (RFC 9110 HTTP Trailer Header Parser & Validator)

export {
    isForbiddenTrailerField,
    isValidTrailerFieldName,
    TrailerFieldIter,
    validateTrailerHeader,
    hasTrailerField,
} from "./trailer.cp";
