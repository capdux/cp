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
export * from "./pgTypes.cp";
export * from "./pgProtocol.cp";
export * from "./mysqlTypes.cp";
export * from "./mysqlProtocol.cp";

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
} from "./mysqlLenenc.cp";

export {
    MYSQL_ERR_PACKET_HEADER,
    MYSQL_SQL_STATE_MARKER,
    MySqlErrorPacketError,
    MySqlErrorPacket,
} from "./mysqlErr.cp";

export {
    PG_NULL_LENGTH_U32,
    PG_NULL_LENGTH_I32,
    PgWireWriterError,
    PgLengthPatch,
    PgWireWriter,
    simdPgCopyBytes,
} from "./pgWriter.cp";

export {
    MySqlWireReaderError,
    MySqlWireReader,
} from "./mysqlReader.cp";

export {
    MYSQL_PACKET_HEADER_SIZE,
    MYSQL_MAX_PACKET_PAYLOAD,
    MySqlWireWriterError,
    MySqlPacketPatch,
    MySqlWireWriter,
} from "./mysqlWriter.cp";

export {
    PG_MAX_FRAME_LEN,
    PgWireReaderError,
    PgFrameView,
    PgWireReader,
    simdFindNul,
} from "./pgReader.cp";

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
} from "./mysqlQuery.cp";

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
} from "./pgStartup.cp";

export {
    PG_ROW_DESCRIPTION_TAG,
    PG_MAX_ROW_FIELDS,
    PgRowDescError,
    PgFieldFormatCode,
    PgFieldDescription,
    PgRowDescriptionIter,
    encodePgRowDescription,
    decodePgRowDescription,
} from "./pgRowDesc.cp";

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
} from "./mysqlOkEof.cp";

export {
    PG_PARSE_TAG,
    PG_DESCRIBE_TAG,
    PG_EXECUTE_TAG,
    PG_CLOSE_TAG,
    PG_SYNC_TAG,
    PG_FLUSH_TAG,
    PG_TARGET_PORTAL,
    PG_TARGET_STATEMENT,
    PG_MAX_PARSE_PARAMS,
    PgExtendedError,
    PgTargetKind,
    PgTargetRef,
    PgParamOidIter,
    PgDecodedParse,
    PgExecuteMsg,
    encodePgParse,
    decodePgParse,
    encodePgDescribe,
    decodePgDescribe,
    encodePgExecute,
    decodePgExecute,
    encodePgClose,
    decodePgClose,
    encodePgSync,
    decodePgSync,
    encodePgFlush,
    decodePgFlush,
} from "./pgExtended.cp";

export {
    PG_ERROR_RESPONSE_TAG,
    PG_NOTICE_RESPONSE_TAG,
    PG_MAX_DIAGNOSTIC_FIELDS,
    PgErrorNoticeError,
    PgDiagnosticFieldKind,
    PgDiagnosticField,
    PgDiagnosticIter,
    PgErrorNoticeView,
    encodePgErrorResponse,
    encodePgNoticeResponse,
    decodePgErrorNoticeIter,
    decodePgErrorOrNotice,
    decodePgErrorResponse,
    decodePgNoticeResponse,
} from "./pgErrNotice.cp";

export {
    PG_READY_FOR_QUERY_TAG,
    PG_BACKEND_KEY_DATA_TAG,
    PG_COMMAND_COMPLETE_TAG,
    PG_PARAMETER_STATUS_TAG,
    PG_BACKEND_KEY_DATA_FRAME_LEN,
    PgControlError,
    PgTxStatus,
    PgBackendKeyData,
    PgParameterStatusView,
    PgCommandCompleteView,
    encodePgReadyForQuery,
    decodePgReadyForQuery,
    encodePgBackendKeyData,
    decodePgBackendKeyData,
    encodePgCommandComplete,
    decodePgCommandComplete,
    encodePgParameterStatus,
    decodePgParameterStatus,
} from "./pgControl.cp";

export {
    MYSQL_STMT_PREPARE_OK_STATUS,
    MYSQL_STMT_PREPARE_OK_MIN_LEN,
    MYSQL_STMT_PREPARE_OK_STD_LEN,
    MySqlStmtControlError,
    MySqlWireCmd,
    MySqlWirePacketHeader,
    MySqlStmtPrepareOkView,
    MySqlResultSetHeaderView,
    MySqlSimpleCommandView,
    encodeMySqlStmtPrepareOk,
    decodeMySqlStmtPrepareOk,
    encodeMySqlResultSetHeader,
    decodeMySqlResultSetHeader,
    encodeMySqlSimpleCommand,
    decodeMySqlSimpleCommand,
    encodeMySqlStmtIdCommand,
    decodeMySqlStmtIdCommand,
} from "./mysqlStmt.cp";

export {
    MYSQL_AUTH_SWITCH_HEADER,
    MYSQL_LOCAL_INFILE_HEADER,
    MYSQL_SSL_REQUEST_PAYLOAD_LEN,
    MYSQL_CAP_CLIENT_SSL,
    MySqlAuthFrameError,
    MySqlAuthSwitchReqView,
    MySqlAuthSwitchRespView,
    MySqlSslRequestView,
    MySqlLocalInfileReqView,
    encodeMySqlAuthSwitchRequest,
    decodeMySqlAuthSwitchRequest,
    encodeMySqlAuthSwitchResponse,
    decodeMySqlAuthSwitchResponse,
    encodeMySqlSslRequestFrame,
    decodeMySqlSslRequestFrame,
    encodeMySqlLocalInfileRequest,
    decodeMySqlLocalInfileRequest,
} from "./mysqlAuth.cp";

export {
    PG_PASSWORD_TAG,
    PG_COPY_DATA_TAG,
    PG_COPY_DONE_TAG,
    PG_COPY_FAIL_TAG,
    PG_TERMINATE_TAG,
    PgAuthCopyError,
    PgSaslInitialResponseView,
    encodePgPasswordMessage,
    decodePgPasswordMessage,
    encodePgSaslInitialResponse,
    decodePgSaslInitialResponse,
    encodePgSaslResponse,
    decodePgSaslResponse,
    encodePgCopyData,
    decodePgCopyData,
    encodePgCopyDone,
    decodePgCopyDone,
    encodePgCopyFail,
    decodePgCopyFail,
    encodePgTerminate,
    decodePgTerminate,
} from "./pgAuthCopy.cp";

export {
    MYSQL_COM_STMT_EXECUTE_CMD,
    MYSQL_STMT_MAX_PARAMS,
    MYSQL_CURSOR_TYPE_NO_CURSOR,
    MYSQL_CURSOR_TYPE_READ_ONLY,
    MYSQL_CURSOR_TYPE_FOR_UPDATE,
    MYSQL_CURSOR_TYPE_SCROLLABLE,
    MYSQL_PARAM_UNSIGNED_FLAG,
    MySqlStmtExecuteError,
    MySqlExecFieldType,
    MySqlExecParam,
    MySqlStmtExecuteRequest,
    MySqlStmtExecuteDecoded,
    mysqlNullBitmapLen,
    mysqlIsNullBitmapSet,
    encodeMySqlStmtExecutePayload,
    encodeMySqlStmtExecute,
    decodeMySqlStmtExecutePayload,
    decodeMySqlStmtExecuteFrame,
} from "./mysqlExec.cp";

export {
    PG_AUTHENTICATION_TAG,
    PG_AUTH_CODE_OK,
    PG_AUTH_CODE_KERBEROS_V5,
    PG_AUTH_CODE_CLEARTEXT,
    PG_AUTH_CODE_MD5,
    PG_AUTH_CODE_GSS,
    PG_AUTH_CODE_GSS_CONTINUE,
    PG_AUTH_CODE_SSPI,
    PG_AUTH_CODE_SASL,
    PG_AUTH_CODE_SASL_CONTINUE,
    PG_AUTH_CODE_SASL_FINAL,
    PgAuthChallengeError,
    PgAuthChallengeKind,
    PgSaslContinueChallenge,
    PgSaslFinalView,
    PgSaslMechanismIter,
    PgAuthChallengeView,
    parsePgSaslContinue,
    parsePgSaslFinal,
    encodePgAuthOk,
    encodePgAuthCleartext,
    encodePgAuthMd5,
    encodePgAuthSasl,
    encodePgAuthSaslContinue,
    encodePgAuthSaslFinal,
    decodePgAuthChallenge,
} from "./pgAuthChal.cp";

export {
    PG_DATA_ROW_TAG,
    PG_DATA_ROW_NULL_LEN,
    PG_DATA_ROW_MAX_COLS,
    PG_DATA_ROW_MAX_FRAME_LEN,
    PgDataRowError,
    PgDataRowColumn,
    PgDataRowIter,
    PgDataRowView,
    estimatePgDataRowFrameLen,
    encodePgDataRowPayload,
    encodePgDataRowFrame,
    decodePgDataRowIter,
    decodePgDataRowFrame,
} from "./pgDataRow.cp";

export {
    MYSQL_CHARSET_DEFAULT_ID,
    MYSQL_CHARSET_UTF8MB4_0900_AI_CI_ID,
    MYSQL_CHARSET_UTF8MB4_BIN_ID,
    MYSQL_CHARSET_UTF8MB4_UNICODE_CI_ID,
    MYSQL_CHARSET_UTF8MB3_GENERAL_CI_ID,
    MYSQL_CHARSET_BINARY_ID,
    MYSQL_CHARSET_LATIN1_SWEDISH_CI_ID,
    MYSQL_CHARSET_ASCII_GENERAL_CI_ID,
    MYSQL_CHARSET_GBK_CHINESE_CI_ID,
    MYSQL_CHARSET_GB18030_CHINESE_CI_ID,
    MySqlCharsetFamily,
    MySqlCollationInfo,
    lookupMySqlCollation,
    lookupMySqlCollationByName,
    isMySqlBinaryColumn,
    mysqlMaxCharLen,
} from "./mysqlCharset.cp";

export {
    MYSQL_SESSION_CAP_CONNECT_WITH_DB,
    MYSQL_SESSION_CAP_PROTOCOL_41,
    MYSQL_SESSION_CAP_SSL,
    MYSQL_SESSION_CAP_TRANSACTIONS,
    MYSQL_SESSION_CAP_SECURE_CONNECTION,
    MYSQL_SESSION_CAP_MULTI_STATEMENTS,
    MYSQL_SESSION_CAP_MULTI_RESULTS,
    MYSQL_SESSION_CAP_PLUGIN_AUTH,
    MYSQL_SESSION_CAP_DEPRECATE_EOF,
    MYSQL_SESSION_CAP_REJECT_MASK,
    MYSQL_SESSION_STATUS_IN_TRANS,
    MYSQL_SESSION_STATUS_AUTOCOMMIT,
    MYSQL_SESSION_STATUS_MORE_RESULTS,
    MYSQL_SESSION_STATUS_NO_BACKSLASH_ESCAPES,
    MYSQL_SESSION_STATUS_QUERY_SLOW,
    MYSQL_SESSION_STATUS_IN_TRANS_READONLY,
    MYSQL_SESSION_STATUS_SESSION_STATE_CHANGED,
    MySqlSessionError,
    MySqlSessionState,
    MySqlSessionSslMode,
    MySqlSessionTlsStatus,
    MySqlSessionAuthPlugin,
    parseMySqlAuthPlugin,
    MySqlSessionServerStatus,
    defaultMySqlClientCapabilities,
    decideMySqlTlsUpgrade,
    negotiateMySqlCapabilities,
    MySqlSessionMachine,
} from "./mysqlSession.cp";

export {
    PgSessionError,
    PgSessionState,
    PgSessionSslMode,
    PgSessionTlsStatus,
    PgSessionTxStatus,
    parsePgSessionTxStatus,
    PgSessionCmdKind,
    PgSessionCmdResult,
    parsePgSessionCommandTag,
    decidePgTlsResponse,
    PgSessionMachine,
} from "./pgSession.cp";

export {
    SQL_COLUMN_ID_MAX_COLS,
    SQL_COLUMN_ID_MAX_INDEX,
    SqlColumnIdError,
    SqlColumnIdKind,
    SqlColumnId,
    SqlColumnIdMap,
} from "./columnId.cp";

export {
    SQL_DATA_CELL_MAX_ROW_COLS,
    SqlDataCellError,
    SqlDataCellTag,
    SqlDataCellFlags,
    SqlDataCell,
    SqlDataRow,
} from "./dataCell.cp";
