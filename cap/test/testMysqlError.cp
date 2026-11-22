import {
    AnyMySqlError,
    MySqlErrorCategory,
    MySqlErrorKind,
} from "cap:mysql";
import { expect, test } from "cap:test";

fn sliceEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn simulateError(kind: MySqlErrorKind): AnyMySqlError!u32 {
    return switch (kind) {
        .AuthenticationFailed => AnyMySqlError.AuthenticationFailed,
        .ShortRead => AnyMySqlError.ShortRead,
        .InvalidPrepareOKPacket => AnyMySqlError.InvalidPrepareOKPacket,
        else => 1,
    };
}

test("mysql mysqlError: category classification across all domains", fn () {
    expect(MySqlErrorKind.ConnectionClosed.category()).toBe(MySqlErrorCategory.Connection);
    expect(MySqlErrorKind.ConnectionTimedOut.category()).toBe(MySqlErrorCategory.Timeout);
    expect(MySqlErrorKind.LifetimeTimeout.category()).toBe(MySqlErrorCategory.Timeout);
    expect(MySqlErrorKind.IdleTimeout.category()).toBe(MySqlErrorCategory.Timeout);
    expect(MySqlErrorKind.AuthenticationFailed.category()).toBe(MySqlErrorCategory.Auth);
    expect(MySqlErrorKind.UnsupportedAuthPlugin.category()).toBe(MySqlErrorCategory.Auth);
    expect(MySqlErrorKind.WrongNumberOfParametersProvided.category()).toBe(MySqlErrorCategory.Query);
    expect(MySqlErrorKind.OutOfMemory.category()).toBe(MySqlErrorCategory.Resource);
    expect(MySqlErrorKind.InvalidState.category()).toBe(MySqlErrorCategory.State);
    expect(MySqlErrorKind.InvalidPrepareOKPacket.category()).toBe(MySqlErrorCategory.Protocol);
    expect(MySqlErrorKind.ShortRead.category()).toBe(MySqlErrorCategory.Protocol);
});

test("mysql mysqlError: standardized ERR_MYSQL_* codes and diagnostic messages", fn () {
    expect(sliceEql(MySqlErrorKind.ConnectionClosed.code(), "ERR_MYSQL_CONNECTION_CLOSED")).toBe(true);
    expect(sliceEql(MySqlErrorKind.AuthenticationFailed.code(), "ERR_MYSQL_AUTH_FAILED")).toBe(true);
    expect(sliceEql(MySqlErrorKind.LocalInfileNotSupported.code(), "ERR_MYSQL_LOCAL_INFILE_DISABLED")).toBe(true);
    expect(sliceEql(MySqlErrorKind.ShortRead.code(), "ERR_MYSQL_SHORT_READ")).toBe(true);
    expect(sliceEql(MySqlErrorKind.InvalidPrepareOKPacket.code(), "ERR_MYSQL_INVALID_PREPARE_OK")).toBe(true);

    expect(MySqlErrorKind.ConnectionClosed.message().len > 0).toBe(true);
    expect(MySqlErrorKind.AuthenticationFailed.message().len > 0).toBe(true);
    expect(MySqlErrorKind.InvalidState.message().len > 0).toBe(true);
});

test("mysql mysqlError: retryability and connection-reset pool policies", fn () {
    // Transient errors are retryable and do not signal wire corruption
    expect(MySqlErrorKind.ConnectionClosed.isRetryable()).toBe(true);
    expect(MySqlErrorKind.ConnectionTimedOut.isRetryable()).toBe(true);
    expect(MySqlErrorKind.IdleTimeout.isRetryable()).toBe(true);
    expect(MySqlErrorKind.ShortRead.isRetryable()).toBe(true);
    expect(MySqlErrorKind.ShortRead.requiresConnectionReset()).toBe(false);

    // Auth errors are neither retryable nor wire-desync
    expect(MySqlErrorKind.AuthenticationFailed.isAuthError()).toBe(true);
    expect(MySqlErrorKind.AuthenticationFailed.isRetryable()).toBe(false);
    expect(MySqlErrorKind.AuthenticationFailed.requiresConnectionReset()).toBe(false);

    // Malformed packets / out-of-sync require tearing down the pooled connection
    expect(MySqlErrorKind.InvalidPrepareOKPacket.requiresConnectionReset()).toBe(true);
    expect(MySqlErrorKind.InvalidResultRow.requiresConnectionReset()).toBe(true);
    expect(MySqlErrorKind.UnexpectedPacket.requiresConnectionReset()).toBe(true);
    expect(MySqlErrorKind.InvalidState.requiresConnectionReset()).toBe(true);
    expect(MySqlErrorKind.InvalidState.isRetryable()).toBe(false);
});

test("mysql mysqlError: server errno mapping to MySqlErrorKind", fn () {
    expect(MySqlErrorKind.fromServerErrno(1045)).toBe(MySqlErrorKind.AuthenticationFailed);
    expect(MySqlErrorKind.fromServerErrno(1698)).toBe(MySqlErrorKind.AuthenticationFailed);
    expect(MySqlErrorKind.fromServerErrno(1043)).toBe(MySqlErrorKind.UnsupportedProtocolVersion);
    expect(MySqlErrorKind.fromServerErrno(1148)).toBe(MySqlErrorKind.LocalInfileNotSupported);
    expect(MySqlErrorKind.fromServerErrno(1251)).toBe(MySqlErrorKind.UnsupportedAuthPlugin);
    expect(MySqlErrorKind.fromServerErrno(2006)).toBe(MySqlErrorKind.ConnectionClosed);
    expect(MySqlErrorKind.fromServerErrno(2013)).toBe(MySqlErrorKind.ConnectionClosed);
    expect(MySqlErrorKind.fromServerErrno(2014)).toBe(MySqlErrorKind.InvalidState);
    expect(MySqlErrorKind.fromServerErrno(2027)).toBe(MySqlErrorKind.UnexpectedPacket);
    expect(MySqlErrorKind.fromServerErrno(9999)).toBe(MySqlErrorKind.UnknownError);
});

test("mysql mysqlError: AnyMySqlError error union propagation and matching", fn () {
    let caught_auth = false;
    * = simulateError(MySqlErrorKind.AuthenticationFailed) catch |e| {
        caught_auth = e == AnyMySqlError.AuthenticationFailed;
    };
    expect(caught_auth).toBe(true);

    let caught_short = false;
    * = simulateError(MySqlErrorKind.ShortRead) catch |e| {
        caught_short = e == AnyMySqlError.ShortRead;
    };
    expect(caught_short).toBe(true);

    let caught_prep = false;
    * = simulateError(MySqlErrorKind.InvalidPrepareOKPacket) catch |e| {
        caught_prep = e == AnyMySqlError.InvalidPrepareOKPacket;
    };
    expect(caught_prep).toBe(true);

    expect(simulateError(MySqlErrorKind.ConnectionClosed) catch 0).toBe(1);
});

test("mysql mysqlError: auth helper covers all 6 auth variants exclusively", fn () {
    expect(MySqlErrorKind.PasswordRequired.isAuthError()).toBe(true);
    expect(MySqlErrorKind.MissingAuthData.isAuthError()).toBe(true);
    expect(MySqlErrorKind.AuthenticationFailed.isAuthError()).toBe(true);
    expect(MySqlErrorKind.FailedToEncryptPassword.isAuthError()).toBe(true);
    expect(MySqlErrorKind.InvalidPublicKey.isAuthError()).toBe(true);
    expect(MySqlErrorKind.UnsupportedAuthPlugin.isAuthError()).toBe(true);
    expect(MySqlErrorKind.InvalidQueryBinding.isAuthError()).toBe(false);
    expect(MySqlErrorKind.Overflow.isAuthError()).toBe(false);
});
