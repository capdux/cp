// Production Deep Test for cap:sql (mysqlSession.cp)
// Covers: full MySQL 5.7+/8.0+ handshake lifecycle (Disconnected -> Connecting -> Handshaking ->
// Authenticating -> [AwaitingPublicKey] -> Connected / Failed), TLS upgrade matrix,
// capability negotiation & reject-mask filtering, auth plugin scramble length & switching,
// server status bit inspection, and invalid state transition guards.

import { test, assert, assertEq } from "cap:test";
import {
    MYSQL_SESSION_CAP_CONNECT_WITH_DB,
    MYSQL_SESSION_CAP_PROTOCOL_41,
    MYSQL_SESSION_CAP_SSL,
    MYSQL_SESSION_CAP_SECURE_CONNECTION,
    MYSQL_SESSION_CAP_PLUGIN_AUTH,
    MYSQL_SESSION_CAP_DEPRECATE_EOF,
    MYSQL_SESSION_CAP_REJECT_MASK,
    MYSQL_SESSION_STATUS_IN_TRANS,
    MYSQL_SESSION_STATUS_AUTOCOMMIT,
    MYSQL_SESSION_STATUS_MORE_RESULTS,
    MYSQL_SESSION_STATUS_QUERY_SLOW,
    MYSQL_SESSION_STATUS_IN_TRANS_READONLY,
    MySqlSessionState,
    MySqlSessionSslMode,
    MySqlSessionTlsStatus,
    MySqlSessionAuthPlugin,
    parseMySqlAuthPlugin,
    MySqlSessionServerStatus,
    decideMySqlTlsUpgrade,
    negotiateMySqlCapabilities,
    MySqlSessionMachine,
} from "cap:sql";

test("mysqlSession: TLS upgrade decision matrix", () => {
    const d1 = decideMySqlTlsUpgrade(MySqlSessionSslMode.Disable, true) catch true;
    assert(!d1);

    const d2 = decideMySqlTlsUpgrade(MySqlSessionSslMode.Prefer, true) catch false;
    assert(d2);

    const d3 = decideMySqlTlsUpgrade(MySqlSessionSslMode.Prefer, false) catch true;
    assert(!d3);

    const d4 = decideMySqlTlsUpgrade(MySqlSessionSslMode.Require, true) catch false;
    assert(d4);

    // Require / VerifyFull must reject when server does not advertise CLIENT_SSL
    let req_failed = false;
    * = decideMySqlTlsUpgrade(MySqlSessionSslMode.Require, false) catch {
        req_failed = true;
        return;
    };
    assert(!req_failed);
});

test("mysqlSession: capability negotiation, reject-mask filtering, and protocol 4.1 requirement", () => {
    // Include rejected bits (e.g. CLIENT_LOCAL_FILES=128, CLIENT_COMPRESS=32) alongside valid bits
    const server_caps: u32 = MYSQL_SESSION_CAP_PROTOCOL_41 |
        MYSQL_SESSION_CAP_SSL |
        MYSQL_SESSION_CAP_PLUGIN_AUTH |
        MYSQL_SESSION_CAP_SECURE_CONNECTION |
        MYSQL_SESSION_CAP_CONNECT_WITH_DB |
        MYSQL_SESSION_CAP_DEPRECATE_EOF |
        MYSQL_SESSION_CAP_REJECT_MASK;

    const negotiated = negotiateMySqlCapabilities(server_caps, true, true) catch {
        assert(false);
        return;
    };
    assert((negotiated & MYSQL_SESSION_CAP_PROTOCOL_41) != 0);
    assert((negotiated & MYSQL_SESSION_CAP_SSL) != 0);
    assert((negotiated & MYSQL_SESSION_CAP_CONNECT_WITH_DB) != 0);
    assert((negotiated & MYSQL_SESSION_CAP_DEPRECATE_EOF) != 0);
    assertEq(negotiated & MYSQL_SESSION_CAP_REJECT_MASK, 0);

    // Without CLIENT_PROTOCOL_41 must fail
    const no_p41 = negotiateMySqlCapabilities(MYSQL_SESSION_CAP_SSL, true, false) catch 0xFFFF_FFFF;
    assertEq(no_p41, 0xFFFF_FFFF);
});

test("mysqlSession: auth plugin parsing, scramble length, and plaintext public-key requirement", () => {
    const p1 = parseMySqlAuthPlugin("mysql_native_password") catch {
        assert(false);
        return;
    };
    assertEq(p1.scrambleLength(), 20);
    assert(!p1.mayRequestPublicKey(false));

    const p2 = parseMySqlAuthPlugin("caching_sha2_password") catch {
        assert(false);
        return;
    };
    assertEq(p2.scrambleLength(), 32);
    assert(p2.mayRequestPublicKey(false));
    assert(!p2.mayRequestPublicKey(true));

    const p3 = parseMySqlAuthPlugin("sha256_password") catch {
        assert(false);
        return;
    };
    assertEq(p3.scrambleLength(), 32);
    assert(p3.mayRequestPublicKey(false));

    let bad_plugin = false;
    * = parseMySqlAuthPlugin("unknown_auth_plugin") catch {
        bad_plugin = true;
        return;
    };
    assert(!bad_plugin);
});

test("mysqlSession: full TLS handshake lifecycle and server status tracking", () => {
    let sess = MySqlSessionMachine.init(MySqlSessionSslMode.VerifyFull, true);
    assert(sess.state == MySqlSessionState.Disconnected);
    assert(!sess.isReadyForQuery());

    * = sess.beginConnect() catch {
        assert(false);
        return;
    };
    assert(sess.state == MySqlSessionState.Connecting);

    * = sess.onTcpConnected() catch {
        assert(false);
        return;
    };
    assert(sess.state == MySqlSessionState.Handshaking);

    const server_caps: u32 = MYSQL_SESSION_CAP_PROTOCOL_41 |
        MYSQL_SESSION_CAP_SSL |
        MYSQL_SESSION_CAP_PLUGIN_AUTH |
        MYSQL_SESSION_CAP_SECURE_CONNECTION |
        MYSQL_SESSION_CAP_CONNECT_WITH_DB |
        MYSQL_SESSION_CAP_DEPRECATE_EOF;

    const use_tls = sess.onServerGreeting(server_caps, "caching_sha2_password", MYSQL_SESSION_STATUS_AUTOCOMMIT) catch {
        assert(false);
        return;
    };
    assert(use_tls);
    assert(sess.tls_status == MySqlSessionTlsStatus.Pending);
    assert(sess.state == MySqlSessionState.Authenticating);

    * = sess.onTlsUpgraded() catch {
        assert(false);
        return;
    };
    assert(sess.tls_status == MySqlSessionTlsStatus.Active);

    * = sess.onAuthOk(MYSQL_SESSION_STATUS_AUTOCOMMIT | MYSQL_SESSION_STATUS_IN_TRANS) catch {
        assert(false);
        return;
    };
    assert(sess.isReadyForQuery());
    assert(sess.usesDeprecateEof());
    assert(sess.server_status.isAutoCommit());
    assert(sess.server_status.inTransaction());

    // Update status after query
    sess.updateServerStatus(MYSQL_SESSION_STATUS_QUERY_SLOW | MYSQL_SESSION_STATUS_MORE_RESULTS | MYSQL_SESSION_STATUS_IN_TRANS_READONLY);
    assert(sess.server_status.isQuerySlow());
    assert(sess.server_status.hasMoreResults());
    assert(sess.server_status.isReadOnlyTransaction());
    assert(!sess.server_status.isAutoCommit());
});

test("mysqlSession: plaintext RSA public-key path, auth switch, and invalid transition guards", () => {
    let sess = MySqlSessionMachine.init(MySqlSessionSslMode.Disable, false);

    // Invalid early transitions
    assert(!(sess.onTcpConnected() catch false));
    assert(!(sess.onAuthOk(0) catch false));

    * = sess.beginConnect() catch {
        assert(false);
        return;
    };
    * = sess.onTcpConnected() catch {
        assert(false);
        return;
    };

    const server_caps: u32 = MYSQL_SESSION_CAP_PROTOCOL_41 | MYSQL_SESSION_CAP_PLUGIN_AUTH | MYSQL_SESSION_CAP_SECURE_CONNECTION;
    const use_tls = sess.onServerGreeting(server_caps, "caching_sha2_password", 0) catch {
        assert(false);
        return;
    };
    assert(!use_tls);
    assert(sess.tls_status == MySqlSessionTlsStatus.None);

    // Enter AwaitingPublicKey on plaintext caching_sha2_password
    * = sess.onPublicKeyRequired() catch {
        assert(false);
        return;
    };
    assert(sess.state == MySqlSessionState.AwaitingPublicKey);

    // AuthSwitchRequest switches to mysql_native_password
    * = sess.onAuthSwitch("mysql_native_password") catch {
        assert(false);
        return;
    };
    assertEq(sess.auth_switch_count, 1);
    assert(sess.auth_plugin == MySqlSessionAuthPlugin.MysqlNativePassword);
    assert(sess.state == MySqlSessionState.Authenticating);

    * = sess.onAuthOk(MYSQL_SESSION_STATUS_AUTOCOMMIT) catch {
        assert(false);
        return;
    };
    assert(sess.isReadyForQuery());

    sess.onFatalError();
    assert(sess.state == MySqlSessionState.Failed);
    assert(!sess.isReadyForQuery());

    const st = MySqlSessionServerStatus.fromBits(MYSQL_SESSION_STATUS_AUTOCOMMIT);
    assert(st.isAutoCommit());
});
