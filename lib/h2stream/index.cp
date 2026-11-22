// Source: Absorbed from /data/dev/bun/src/http/h2_client/Stream.cp | Synced: 2026-11-22T00:14:33Z
// Backward-compatible re-export: canonical implementation consolidated into lib/http/h2/stream.cp

export {
    H2_MAX_WINDOW_SIZE,
    H2_DEFAULT_INITIAL_WINDOW,
    H2_RST_STREAM_FRAME_LEN,
    H2_WINDOW_UPDATE_FRAME_LEN,
    StreamState,
    H2ErrorCode,
    errorCodeToU32,
    encodeRstStreamFrame,
    encodeWindowUpdateFrame,
    H2Stream,
} from "../http/h2/stream.cp";
