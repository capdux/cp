// Source: Absorbed from /data/dev/bun/src/sql/postgres/protocol/Authentication.cp, /data/dev/bun/src/sql/postgres/protocol/NewWriter.cp

export {
    AUTH_MESSAGE_TAG,
    PgAuthError,
    AuthType,
    authTypeFromInt,
    SASLContinueInfo,
    DecodedAuthMessage,
    decodeAuthMessage,
    PgWireWriter,
} from "./auth.cp";
