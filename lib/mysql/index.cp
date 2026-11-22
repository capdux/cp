// Source: Absorbed from /data/dev/bun/src/sql/mysql/ | Synced: 2026-11-21T17:00:18Z (Unified MySQL protocol & wire standard library: cap:mysql)

import authMod from "./auth.cp";
import capsMod from "./caps.cp";
import controlMod from "./control.cp";
import wireMod from "./wire.cp";

// --- 1. Auth & SSLRequest (./auth.cp) ---
export {
    NATIVE_SCRAMBLE_LEN,
    CACHING_SHA2_SCRAMBLE_LEN,
    MIN_SALT_LEN,
    SSL_REQUEST_PAYLOAD_LEN,
    SSL_REQUEST_FRAME_LEN,
    AuthMethod,
    AuthFastStatus,
    scrambleNativePassword,
    scrambleCachingSha2Password,
    encodeCleartextPassword,
    xorPasswordWithNonce,
    SSLRequest,
} from "./auth.cp";
export const AuthError = authMod.Error;

// --- 2. Capabilities & StatusFlags bitset (./caps.cp) ---
export {
    CapabilityFlag,
    StatusFlag,
} from "./caps.cp";
export const CapsCapabilities = capsMod.Capabilities;
export const CapsStatusFlags = capsMod.StatusFlags;

// --- 3. Control packets (./control.cp) ---
export {
    CommandType,
    HandshakeV10,
    OKPacket,
    ErrorPacket,
    EOFPacket,
    AuthSwitchRequest,
} from "./control.cp";
export const ControlError = controlMod.Error;

// --- 4. Field types & ColumnDefinition41 (./fieldtypes.cp) ---
export {
    CHARSET_LATIN1_SWEDISH_CI,
    CHARSET_UTF8MB3_GENERAL_CI,
    CHARSET_UTF8MB4_GENERAL_CI,
    CHARSET_UTF8MB4_BIN,
    CHARSET_BINARY,
    CHARSET_UTF8MB4_UNICODE_CI,
    MySqlWireError,
    FieldType,
    fieldTypeFromByte,
    ColumnFlags,
    DecodedLenEncInt,
    DecodedLenEncString,
    lenEncIntSize,
    encodeLenEncInt,
    decodeLenEncInt,
    encodeLenEncString,
    decodeLenEncString,
    MySqlWireBuffer,
    ColumnDefinition41,
    DecodedColumnDef41,
    encodeColumnDefinition41,
    decodeColumnDefinition41,
} from "./fieldtypes.cp";

// --- 5. HandshakeResponse41 codec (./handshake.cp) ---
export {
    DEFAULT_MAX_PACKET_SIZE,
    DEFAULT_CHARSET_UTF8MB4,
    CAP_LONG_PASSWORD,
    CAP_CONNECT_WITH_DB,
    CAP_PROTOCOL_41,
    CAP_SECURE_CONNECTION,
    CAP_PLUGIN_AUTH,
    CAP_CONNECT_ATTRS,
    CAP_PLUGIN_AUTH_LENENC,
    HandshakeError,
    ConnectAttr,
    HandshakeResponse41,
    EncodedHandshake,
    DecodedHandshake41,
    encodeHandshakeResponse41,
    decodeHandshakeResponse41,
} from "./handshake.cp";

// --- 6. Packet Reader / Writer (./packet.cp) ---
export {
    PACKET_HEADER_SIZE,
    MAX_PACKET_PAYLOAD,
    MySqlPacketError,
    PacketHeader,
    DecodedLenInt,
    decodeLenInt,
    MySqlPacketReader,
    PacketHandle,
    MySqlPacketWriter,
} from "./packet.cp";

// --- 7. Protocol capabilities, status flags & charsets (./proto.cp) ---
export {
    CAP_CLIENT_LONG_PASSWORD,
    CAP_CLIENT_FOUND_ROWS,
    CAP_CLIENT_LONG_FLAG,
    CAP_CLIENT_CONNECT_WITH_DB,
    CAP_CLIENT_NO_SCHEMA,
    CAP_CLIENT_COMPRESS,
    CAP_CLIENT_ODBC,
    CAP_CLIENT_LOCAL_FILES,
    CAP_CLIENT_IGNORE_SPACE,
    CAP_CLIENT_PROTOCOL_41,
    CAP_CLIENT_INTERACTIVE,
    CAP_CLIENT_SSL,
    CAP_CLIENT_IGNORE_SIGPIPE,
    CAP_CLIENT_TRANSACTIONS,
    CAP_CLIENT_RESERVED,
    CAP_CLIENT_SECURE_CONNECTION,
    CAP_CLIENT_MULTI_STATEMENTS,
    CAP_CLIENT_MULTI_RESULTS,
    CAP_CLIENT_PS_MULTI_RESULTS,
    CAP_CLIENT_PLUGIN_AUTH,
    CAP_CLIENT_CONNECT_ATTRS,
    CAP_CLIENT_PLUGIN_AUTH_LENENC_CLIENT_DATA,
    CAP_CLIENT_CAN_HANDLE_EXPIRED_PASSWORDS,
    CAP_CLIENT_SESSION_TRACK,
    CAP_CLIENT_DEPRECATE_EOF,
    CAP_CLIENT_OPTIONAL_RESULTSET_METADATA,
    CAP_CLIENT_ZSTD_COMPRESSION_ALGORITHM,
    CAP_CLIENT_QUERY_ATTRIBUTES,
    CAP_MULTI_FACTOR_AUTHENTICATION,
    CAP_CLIENT_CAPABILITY_EXTENSION,
    CAP_CLIENT_SSL_VERIFY_SERVER_CERT,
    CAP_CLIENT_REMEMBER_OPTIONS,
    Capabilities,
    StatusFlags,
    CharacterSet,
} from "./proto.cp";

// --- 8. Wire framing, allocated handshake & text resultset (./wire.cp) ---
export {
    MAX_DEFAULT_PACKET_BYTES,
    MysqlError,
    LenEncInt,
    MysqlPacket,
    MysqlStreamFramer,
    readLenEncInt,
    writeLenEncInt,
    writeLenEncNull,
    readLenEncStr,
    writeLenEncStr,
    readNullTermStr,
    encodePacketFrame,
    CLIENT_LONG_PASSWORD,
    CLIENT_FOUND_ROWS,
    CLIENT_LONG_FLAG,
    CLIENT_CONNECT_WITH_DB,
    CLIENT_PROTOCOL_41,
    CLIENT_TRANSACTIONS,
    CLIENT_SECURE_CONNECTION,
    CLIENT_PLUGIN_AUTH,
    DEFAULT_CLIENT_CAPABILITIES,
    computeNativePasswordAuth,
    COM_QUIT,
    COM_QUERY,
    COM_PING,
    OK_HEADER,
    EOF_HEADER,
    ERR_HEADER,
    OkPacket,
    ErrPacket,
    EofPacket,
    ComPacket,
    ColumnDef41,
    CellValue,
    TextResultRow,
} from "./wire.cp";
export const WireHandshakeV10 = wireMod.HandshakeV10;
export const WireHandshakeResponse41 = wireMod.HandshakeResponse41;

// --- 9. LOCAL INFILE Request (0xFB) codec & path policy (./localInfile.cp) ---
export {
    LOCAL_INFILE_HEADER,
    LOCAL_INFILE_MAX_FILENAME,
    LocalInfileError,
    LocalInfileRequest,
} from "./localInfile.cp";

// --- 10. Bounded Zero-Alloc Wire Cursor (./stackReader.cp) ---
export {
    StackReaderError,
    MysqlFrameSlice,
    StackReader,
} from "./stackReader.cp";

// --- 11. Prepared Statement COM_STMT_PREPARE_OK & Lifecycle Codec (./stmtPrepareOk.cp) ---
export {
    STMT_PREPARE_OK_STATUS,
    STMT_PREPARE_OK_MIN_LEN,
    STMT_PREPARE_OK_STD_LEN,
    STMT_PREPARE_OK_META_LEN,
    COM_STMT_PREPARE_CMD,
    COM_STMT_CLOSE_CMD,
    COM_STMT_RESET_CMD,
    STMT_METADATA_NONE,
    STMT_METADATA_FOLLOWS,
    STMT_MAX_SINGLE_PACKET_PAYLOAD,
    StmtPrepareOkError,
    StmtPrepareOKPacket,
    StmtCommandCodec,
} from "./stmtPrepareOk.cp";

// --- 12. Unified MySQL Error Taxonomy & Diagnostics (./mysqlError.cp) ---
export {
    AnyMySqlError,
    MySqlErrorCategory,
    MySqlErrorKind,
} from "./mysqlError.cp";
