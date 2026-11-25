// Compatibility re-export shim: canonical implementation lives in `cap:router/radixRouter` (`lib/router/radixRouter.cp`).
export {
    RADIX_STATIC_BUCKETS,
    RADIX_MAX_NODES,
    RADIX_MAX_CHILDREN,
    RADIX_MAX_PARAMS,
    RADIX_MAX_SEG_LEN,
    RADIX_MAX_PATH_LEN,
    RADIX_METHOD_SLOTS,
    RadixParam,
    RadixMatchResult,
    RadixRouter,
} from "../router/radixRouter.cp";
