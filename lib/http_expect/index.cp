// Source: Absorbed from /data/dev/bun/src/http/http.cp (RFC 9110 HTTP Expect Header Parser & 100-continue Negotiator)

export {
    ExpectItem,
    ExpectIter,
    ExpectDecision,
    hasContinueExpectation,
    hasUnknownExpectation,
    evaluateExpectHeader,
} from "./expect.cp";
