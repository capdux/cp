// Source: Absorbed from /data/dev/bun/src/http/http.cp (RFC 9110 §10.2.1 HTTP Allow & Method Set Codec)

export {
    HttpMethodBit,
    AllowIter,
    AllowMethodSet,
    parseAllowHeader,
    isMethodAllowed,
    isSafeMethod,
    isIdempotentMethod,
} from "./allow.cp";
