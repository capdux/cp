// Source: Absorbed from /data/dev/bun/src/sql/mysql/protocol/AnyMySQLError.cp | Synced: 2026-11-22T17:54:00Z
// ============================================================================
// lib/mysql/mysqlError.cp — Unified MySQL Protocol Error Taxonomy & Diagnostics
// ============================================================================
// Pure-CP zero-allocation MySQL error set (`AnyMySqlError`), structured
// classifier (`MySqlErrorKind`, `MySqlErrorCategory`), retryability / fatal
// connection-reset policies, and MySQL server errno mapping.

pub err AnyMySqlError {
    ConnectionClosed,
    ConnectionTimedOut,
    LifetimeTimeout,
    IdleTimeout,
    PasswordRequired,
    MissingAuthData,
    AuthenticationFailed,
    FailedToEncryptPassword,
    InvalidPublicKey,
    UnsupportedAuthPlugin,
    UnsupportedProtocolVersion,
    LocalInfileNotSupported,
    OutOfMemory,
    Overflow,
    WrongNumberOfParametersProvided,
    UnsupportedColumnType,
    InvalidLocalInfileRequest,
    InvalidAuthSwitchRequest,
    InvalidQueryBinding,
    InvalidResultRow,
    InvalidBinaryValue,
    InvalidEncodedInteger,
    InvalidEncodedLength,
    InvalidPrepareOKPacket,
    InvalidOKPacket,
    InvalidEOFPacket,
    InvalidErrorPacket,
    UnexpectedPacket,
    ShortRead,
    UnknownError,
    InvalidState,
}

pub enum MySqlErrorCategory: u8 {
    Connection = 0,
    Timeout = 1,
    Auth = 2,
    Protocol = 3,
    Query = 4,
    Resource = 5,
    State = 6,
}

pub enum MySqlErrorKind: u8 {
    ConnectionClosed = 0,
    ConnectionTimedOut = 1,
    LifetimeTimeout = 2,
    IdleTimeout = 3,
    PasswordRequired = 4,
    MissingAuthData = 5,
    AuthenticationFailed = 6,
    FailedToEncryptPassword = 7,
    InvalidPublicKey = 8,
    UnsupportedAuthPlugin = 9,
    UnsupportedProtocolVersion = 10,
    LocalInfileNotSupported = 11,
    OutOfMemory = 12,
    Overflow = 13,
    WrongNumberOfParametersProvided = 14,
    UnsupportedColumnType = 15,
    InvalidLocalInfileRequest = 16,
    InvalidAuthSwitchRequest = 17,
    InvalidQueryBinding = 18,
    InvalidResultRow = 19,
    InvalidBinaryValue = 20,
    InvalidEncodedInteger = 21,
    InvalidEncodedLength = 22,
    InvalidPrepareOKPacket = 23,
    InvalidOKPacket = 24,
    InvalidEOFPacket = 25,
    InvalidErrorPacket = 26,
    UnexpectedPacket = 27,
    ShortRead = 28,
    UnknownError = 29,
    InvalidState = 30,

    fn category(): MySqlErrorCategory {
        return switch (_) {
            .ConnectionClosed => MySqlErrorCategory.Connection,
            .ConnectionTimedOut, .LifetimeTimeout, .IdleTimeout => MySqlErrorCategory.Timeout,
            .PasswordRequired,
            .MissingAuthData,
            .AuthenticationFailed,
            .FailedToEncryptPassword,
            .InvalidPublicKey,
            .UnsupportedAuthPlugin,
            => MySqlErrorCategory.Auth,
            .WrongNumberOfParametersProvided, .InvalidQueryBinding => MySqlErrorCategory.Query,
            .OutOfMemory, .Overflow => MySqlErrorCategory.Resource,
            .InvalidState, .UnknownError => MySqlErrorCategory.State,
            else => MySqlErrorCategory.Protocol,
        };
    }

    fn code(): []const u8 {
        return switch (_) {
            .ConnectionClosed => "ERR_MYSQL_CONNECTION_CLOSED",
            .ConnectionTimedOut => "ERR_MYSQL_CONNECTION_TIMEOUT",
            .LifetimeTimeout => "ERR_MYSQL_LIFETIME_TIMEOUT",
            .IdleTimeout => "ERR_MYSQL_IDLE_TIMEOUT",
            .PasswordRequired => "ERR_MYSQL_PASSWORD_REQUIRED",
            .MissingAuthData => "ERR_MYSQL_MISSING_AUTH_DATA",
            .AuthenticationFailed => "ERR_MYSQL_AUTH_FAILED",
            .FailedToEncryptPassword => "ERR_MYSQL_ENCRYPT_PASSWORD_FAILED",
            .InvalidPublicKey => "ERR_MYSQL_INVALID_PUBLIC_KEY",
            .UnsupportedAuthPlugin => "ERR_MYSQL_UNSUPPORTED_AUTH_PLUGIN",
            .UnsupportedProtocolVersion => "ERR_MYSQL_UNSUPPORTED_PROTOCOL",
            .LocalInfileNotSupported => "ERR_MYSQL_LOCAL_INFILE_DISABLED",
            .OutOfMemory => "ERR_MYSQL_OUT_OF_MEMORY",
            .Overflow => "ERR_MYSQL_OVERFLOW",
            .WrongNumberOfParametersProvided => "ERR_MYSQL_WRONG_PARAM_COUNT",
            .UnsupportedColumnType => "ERR_MYSQL_UNSUPPORTED_COLUMN_TYPE",
            .InvalidLocalInfileRequest => "ERR_MYSQL_INVALID_LOCAL_INFILE",
            .InvalidAuthSwitchRequest => "ERR_MYSQL_INVALID_AUTH_SWITCH",
            .InvalidQueryBinding => "ERR_MYSQL_INVALID_QUERY_BINDING",
            .InvalidResultRow => "ERR_MYSQL_INVALID_RESULT_ROW",
            .InvalidBinaryValue => "ERR_MYSQL_INVALID_BINARY_VALUE",
            .InvalidEncodedInteger => "ERR_MYSQL_INVALID_LENENC_INT",
            .InvalidEncodedLength => "ERR_MYSQL_INVALID_LENENC_LEN",
            .InvalidPrepareOKPacket => "ERR_MYSQL_INVALID_PREPARE_OK",
            .InvalidOKPacket => "ERR_MYSQL_INVALID_OK_PACKET",
            .InvalidEOFPacket => "ERR_MYSQL_INVALID_EOF_PACKET",
            .InvalidErrorPacket => "ERR_MYSQL_INVALID_ERROR_PACKET",
            .UnexpectedPacket => "ERR_MYSQL_UNEXPECTED_PACKET",
            .ShortRead => "ERR_MYSQL_SHORT_READ",
            .UnknownError => "ERR_MYSQL_UNKNOWN",
            .InvalidState => "ERR_MYSQL_INVALID_STATE",
        };
    }

    fn message(): []const u8 {
        return switch (_) {
            .ConnectionClosed => "MySQL connection closed",
            .ConnectionTimedOut => "MySQL connection timed out",
            .LifetimeTimeout => "MySQL connection exceeded max lifetime",
            .IdleTimeout => "MySQL connection idle timeout",
            .PasswordRequired => "MySQL server requires a password",
            .MissingAuthData => "Missing MySQL authentication challenge data",
            .AuthenticationFailed => "MySQL authentication failed",
            .FailedToEncryptPassword => "Failed to encrypt MySQL password",
            .InvalidPublicKey => "Invalid MySQL RSA public key",
            .UnsupportedAuthPlugin => "Unsupported MySQL authentication plugin",
            .UnsupportedProtocolVersion => "Unsupported MySQL protocol version",
            .LocalInfileNotSupported => "MySQL LOCAL INFILE is disabled by client policy",
            .OutOfMemory => "Out of memory while processing MySQL protocol",
            .Overflow => "Numeric overflow in MySQL protocol codec",
            .WrongNumberOfParametersProvided => "Wrong number of prepared statement parameters",
            .UnsupportedColumnType => "Unsupported MySQL column field type",
            .InvalidLocalInfileRequest => "Invalid MySQL LOCAL INFILE request packet",
            .InvalidAuthSwitchRequest => "Invalid MySQL AuthSwitchRequest packet",
            .InvalidQueryBinding => "Invalid MySQL query parameter binding",
            .InvalidResultRow => "Malformed MySQL result row",
            .InvalidBinaryValue => "Malformed MySQL binary protocol value",
            .InvalidEncodedInteger => "Malformed MySQL length-encoded integer",
            .InvalidEncodedLength => "Malformed MySQL length-encoded string length",
            .InvalidPrepareOKPacket => "Malformed MySQL COM_STMT_PREPARE_OK packet",
            .InvalidOKPacket => "Malformed MySQL OK packet",
            .InvalidEOFPacket => "Malformed MySQL EOF packet",
            .InvalidErrorPacket => "Malformed MySQL ERR packet",
            .UnexpectedPacket => "Unexpected MySQL protocol packet",
            .ShortRead => "Incomplete MySQL packet frame (short read)",
            .UnknownError => "Unknown MySQL protocol error",
            .InvalidState => "Invalid MySQL connection state",
        };
    }

    /// True if the caller may safely retry the connection or await more wire bytes.
    fn isRetryable(): bool {
        return switch (_) {
            .ConnectionClosed, .ConnectionTimedOut, .IdleTimeout, .ShortRead => true,
            else => false,
        };
    }

    /// True if the wire stream is desynchronized and the connection must be torn down.
    fn requiresConnectionReset(): bool {
        return switch (_) {
            .UnsupportedProtocolVersion,
            .InvalidLocalInfileRequest,
            .InvalidAuthSwitchRequest,
            .InvalidResultRow,
            .InvalidBinaryValue,
            .InvalidEncodedInteger,
            .InvalidEncodedLength,
            .InvalidPrepareOKPacket,
            .InvalidOKPacket,
            .InvalidEOFPacket,
            .InvalidErrorPacket,
            .UnexpectedPacket,
            .InvalidState,
            => true,
            else => false,
        };
    }

    fn isAuthError(): bool {
        return _.category() == MySqlErrorCategory.Auth;
    }

    /// Maps common MySQL server/client numeric error codes (`errno`) into `MySqlErrorKind`.
    static fn fromServerErrno(errno_code: u16): MySqlErrorKind {
        return switch (errno_code) {
            1043 => MySqlErrorKind.UnsupportedProtocolVersion, // ER_HANDSHAKE_ERROR
            1045, 1698 => MySqlErrorKind.AuthenticationFailed, // ER_ACCESS_DENIED_ERROR / ER_ACCESS_DENIED_NO_PASSWORD_ERROR
            1148 => MySqlErrorKind.LocalInfileNotSupported,    // ER_NOT_ALLOWED_COMMAND
            1251 => MySqlErrorKind.UnsupportedAuthPlugin,      // ER_NOT_SUPPORTED_AUTH_MODE
            1835 => MySqlErrorKind.InvalidQueryBinding,        // ER_MALFORMED_PACKET / binding
            2006, 2013 => MySqlErrorKind.ConnectionClosed,     // CR_SERVER_GONE_ERROR / CR_SERVER_LOST
            2014 => MySqlErrorKind.InvalidState,               // CR_COMMANDS_OUT_OF_SYNC
            2027 => MySqlErrorKind.UnexpectedPacket,           // CR_MALFORMED_PACKET
            else => MySqlErrorKind.UnknownError,
        };
    }
}
