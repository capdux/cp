// Source: Absorbed from /data/dev/bun/src/sql/ | Synced: 2026-11-21T16:42:31Z
// ============================================================================
// Absorbed from Bun: bun/src/sql/
// Upstream Sync: Refer to bun/src/sql/ for future upstream updates.
// ============================================================================
// lib/sql/index.cp
// Unified Public Exports for cap:sql

export * from "./protocol.cp";
export * from "./postgres.cp";
export * from "./mysql.cp";
export * from "./sqlite.cp";

export * from "./shared.cp";
export * from "./pg_types.cp";
export * from "./pg_protocol.cp";
export * from "./mysql_types.cp";
export * from "./mysql_protocol.cp";

// Converged SQL AST, URI, Escape, Params, and Splitter sub-modules
export {
    SqlError,
    SqlTokenKind,
    SqlToken,
    eqIgnoreCase,
    isSqlKeyword,
    tokenizeSql,
} from "./lexer.cp";

export {
    SqlExprKind,
    SqlExpr,
    SelectItem,
    JoinClause,
    OrderByItem,
    SelectStmt,
    ValueRow,
    InsertStmt,
    AssignItem,
    UpdateStmt,
    DeleteStmt,
    ColumnDef,
    CreateTableStmt,
    SqlStmtKind,
    SqlStmt,
} from "./ast.cp";

export {
    parseSql,
    parseSqlWithMaxDepth,
} from "./parser.cp";

export {
    normalizeSqlFingerprint,
    extractTableNames,
    isReadOnlyQuery,
} from "./analyzer.cp";

export {
    SqlUriDialect,
    SslMode,
    UriPgCommandKind,
    UriPgCommandTag,
    SqlConnUri,
} from "./uri.cp";

export * from "./escape.cp";
export * from "./params.cp";
export * from "./splitter.cp";

export {
    MYSQL_LENENC_NULL_MARKER,
    MYSQL_LENENC_U16_MARKER,
    MYSQL_LENENC_U24_MARKER,
    MYSQL_LENENC_U64_MARKER,
    MYSQL_LENENC_ERR_MARKER,
    MySqlLenEncError,
    EncodedLengthInt,
    DecodeLengthIntResult,
    DecodeLengthStringResult,
    encodedLengthIntSize,
    encodeLengthInt,
    encodeLengthIntInto,
    decodeLengthInt,
    decodeLengthIntStrict,
    encodeLengthString,
    decodeLengthString,
} from "./mysql_lenenc.cp";

export {
    MYSQL_ERR_PACKET_HEADER,
    MYSQL_SQL_STATE_MARKER,
    MySqlErrorPacketError,
    MySqlErrorPacket,
} from "./mysql_error_packet.cp";

export {
    PG_NULL_LENGTH_U32,
    PG_NULL_LENGTH_I32,
    PgWireWriterError,
    PgLengthPatch,
    PgWireWriter,
} from "./pg_wire_writer.cp";

export {
    MySqlWireReaderError,
    MySqlWireReader,
} from "./mysql_wire_reader.cp";

export {
    MYSQL_PACKET_HEADER_SIZE,
    MYSQL_MAX_PACKET_PAYLOAD,
    MySqlWireWriterError,
    MySqlPacketPatch,
    MySqlWireWriter,
} from "./mysql_wire_writer.cp";

export {
    PgWireReaderError,
    PgFrameView,
    PgWireReader,
} from "./pg_wire_reader.cp";

export {
    MYSQL_COM_QUERY_CMD,
    MYSQL_MAX_QUERY_PARAMS,
    MySqlQueryCodecError,
    MySqlQueryParamKind,
    MySqlQueryParam,
    MySqlDecodedQuery,
    encodeMySqlSimpleQuery,
    encodeMySqlQueryWithAttrs,
    decodeMySqlSimpleQuery,
    decodeMySqlQueryWithAttrs,
} from "./mysql_query.cp";

export {
    PG_PROTOCOL_V3_0,
    PG_SSL_REQUEST_CODE,
    PG_CANCEL_REQUEST_CODE,
    PG_MAX_STARTUP_PARAMS,
    PgStartupCodecError,
    PgStartupParam,
    PgStartupConfig,
    PgStartupParamIter,
    PgDecodedStartup,
    encodePgStartup,
    decodePgStartup,
    encodePgSslRequestFrame,
    isPgSslRequestFrame,
    encodePgCancelFrame,
} from "./pg_startup.cp";

export {
    PG_ROW_DESCRIPTION_TAG,
    PG_MAX_ROW_FIELDS,
    PgRowDescError,
    PgFieldFormatCode,
    PgFieldDescription,
    PgRowDescriptionIter,
    encodePgRowDescription,
    decodePgRowDescription,
} from "./pg_row_desc.cp";

export {
    MYSQL_OK_HEADER,
    MYSQL_EOF_HEADER,
    MYSQL_FLAG_IN_TRANS,
    MYSQL_FLAG_AUTOCOMMIT,
    MYSQL_FLAG_MORE_RESULTS_EXISTS,
    MYSQL_FLAG_NO_GOOD_INDEX_USED,
    MYSQL_FLAG_NO_INDEX_USED,
    MYSQL_FLAG_CURSOR_EXISTS,
    MYSQL_FLAG_LAST_ROW_SENT,
    MYSQL_FLAG_DB_DROPPED,
    MYSQL_FLAG_NO_BACKSLASH_ESCAPES,
    MYSQL_FLAG_METADATA_CHANGED,
    MYSQL_FLAG_QUERY_WAS_SLOW,
    MYSQL_FLAG_PS_OUT_PARAMS,
    MYSQL_FLAG_IN_TRANS_READONLY,
    MYSQL_FLAG_SESSION_STATE_CHANGED,
    MySqlOkEofError,
    MySqlServerStatusFlags,
    MySqlOkPacketView,
    MySqlEofPacketView,
    encodeMySqlOkPacket,
    decodeMySqlOkPacket,
    encodeMySqlEofPacket,
    decodeMySqlEofPacket,
} from "./mysql_ok_eof.cp";
