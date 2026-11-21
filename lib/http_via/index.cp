// Source: Absorbed from /data/dev/bun/src/http/http.cp (RFC 9110 Via Proxy Chain & Max-Forwards)

export {
    ViaEntry,
    ViaIter,
    viaHopCount,
    viaContainsProxy,
    appendVia,
    parseMaxForwards,
    decrementMaxForwards,
} from "./via.cp";
