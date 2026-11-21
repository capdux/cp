// Source: Absorbed from /data/dev/bun/src/runtime/server/HTTPStatusText.cp

export {
    StatusClass,
    statusLine,
    statusText,
    statusTextOrEmpty,
    classify,
    isInformational,
    isSuccess,
    isRedirect,
    isClientError,
    isServerError,
    isError,
    canHaveBody,
    isCacheableByDefault,
    isRetryable,
} from "./status.cp";
