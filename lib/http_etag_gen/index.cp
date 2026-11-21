// Source: Absorbed from /data/dev/bun/src/http/http.cp (HTTP Strong & Weak ETag Generator)

export {
    fnv1a64,
    generateWeakStatEtag,
    generateStrongHashEtag,
    generateWeakHashEtag,
    isValidEtagSyntax,
} from "./etag_gen.cp";
