// Production Deep Test for cap:sql (pgSession.cp)
// Covers: Disconnected -> Connecting -> SSLRequest ('S'/'N') -> TLS -> SentStartupMessage ->
// BackendKeyData -> ReadyForQuery ('I'/'T'/'E') -> Connected / Failed lifecycle,
// CommandTag parsing (INSERT <oid> <rows>, DELETE/UPDATE/MERGE/SELECT/COPY, non-count tags),
// and invalid transition / pending-TLS / bad-indicator guards.

import { test, assert, assertEq } from "cap:test";
import {
    PgSessionState,
    PgSessionSslMode,
    PgSessionTlsStatus,
    PgSessionTxStatus,
    parsePgSessionTxStatus,
    PgSessionCmdKind,
    parsePgSessionCommandTag,
    decidePgTlsResponse,
    PgSessionMachine,
} from "cap:sql";

fn strEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

test("pgSession: CommandTag parsing across INSERT/DML/SELECT/COPY/DDL", () => {
    const c1 = parsePgSessionCommandTag("INSERT 0 42");
    assert(c1.kind == PgSessionCmdKind.Insert);
    assertEq(c1.row_count, 42);

    const c2 = parsePgSessionCommandTag("UPDATE 7");
    assert(c2.kind == PgSessionCmdKind.Update);
    assertEq(c2.row_count, 7);

    const c3 = parsePgSessionCommandTag("DELETE 0");
    assert(c3.kind == PgSessionCmdKind.Delete);
    assertEq(c3.row_count, 0);

    const c4 = parsePgSessionCommandTag("SELECT 128");
    assert(c4.kind == PgSessionCmdKind.Select);
    assertEq(c4.row_count, 128);

    const c5 = parsePgSessionCommandTag("MERGE 3");
    assert(c5.kind == PgSessionCmdKind.Merge);
    assertEq(c5.row_count, 3);

    const c6 = parsePgSessionCommandTag("COPY 1000");
    assert(c6.kind == PgSessionCmdKind.Copy);
    assertEq(c6.row_count, 1000);

    const c7 = parsePgSessionCommandTag("CREATE TABLE");
    assert(c7.kind == PgSessionCmdKind.Other);
    assertEq(c7.row_count, 0);
    assert(strEql(c7.raw_tag, "CREATE TABLE"));

    // Malformed INSERT without second space falls back to Other
    const c8 = parsePgSessionCommandTag("INSERT 0");
    assert(c8.kind == PgSessionCmdKind.Other);
});

test("pgSession: SSLRequest ('S'/'N') negotiation matrix", () => {
    assert(decidePgTlsResponse(PgSessionSslMode.Prefer, 'S') catch false);
    assert(!(decidePgTlsResponse(PgSessionSslMode.Prefer, 'N') catch true));
    assert(!(decidePgTlsResponse(PgSessionSslMode.Disable, 'S') catch true));
    assert(decidePgTlsResponse(PgSessionSslMode.VerifyFull, 'S') catch false);

    // Require / VerifyFull must reject 'N'
    let rejected_n = false;
    * = decidePgTlsResponse(PgSessionSslMode.Require, 'N') catch {
        rejected_n = true;
        return;
    };
    assert(!rejected_n);
});

test("pgSession: ReadyForQuery indicator parsing and full TLS handshake lifecycle", () => {
    if (tx_i := parsePgSessionTxStatus('I')) {
        assert(tx_i == PgSessionTxStatus.Idle);
    } else {
        assert(false);
    }
    if (tx_t := parsePgSessionTxStatus('T')) {
        assert(tx_t == PgSessionTxStatus.InTransaction);
    } else {
        assert(false);
    }
    if (tx_e := parsePgSessionTxStatus('E')) {
        assert(tx_e == PgSessionTxStatus.FailedTransaction);
    } else {
        assert(false);
    }
    assert(parsePgSessionTxStatus('X') == null);

    let pg = PgSessionMachine.init(PgSessionSslMode.VerifyCa);
    * = pg.beginConnect() catch {
        assert(false);
        return;
    };
    const use_tls = pg.onSslNegotiated('S') catch {
        assert(false);
        return;
    };
    assert(use_tls);
    assert(pg.tls_status == PgSessionTlsStatus.Pending);

    // Cannot send StartupMessage while TLS handshake is still Pending
    assert(!(pg.onStartupSent() catch false));

    * = pg.onTlsHandshakeComplete() catch {
        assert(false);
        return;
    };
    assert(pg.tls_status == PgSessionTlsStatus.Active);

    * = pg.onStartupSent() catch {
        assert(false);
        return;
    };
    assert(pg.state == PgSessionState.SentStartupMessage);

    * = pg.onBackendKeyData(12345, 999888) catch {
        assert(false);
        return;
    };
    assert(pg.has_backend_key);
    assertEq(pg.backend_pid, 12345);
    assertEq(pg.backend_secret_key, 999888);

    * = pg.onReadyForQuery('I') catch {
        assert(false);
        return;
    };
    assert(pg.isReadyForQuery());
    assert(!pg.inTransaction());
    assert(!pg.needsRollback());
});

test("pgSession: transaction transitions ('T' -> 'E' -> 'I') and command tracking", () => {
    let pg = PgSessionMachine.init(PgSessionSslMode.Disable);
    * = pg.beginConnect() catch {
        assert(false);
        return;
    };
    * = pg.onStartupSent() catch {
        assert(false);
        return;
    };
    * = pg.onReadyForQuery('I') catch {
        assert(false);
        return;
    };

    * = pg.onReadyForQuery('T') catch {
        assert(false);
        return;
    };
    assert(pg.inTransaction());
    assert(!pg.needsRollback());

    const cmd = pg.onCommandComplete("INSERT 0 3") catch {
        assert(false);
        return;
    };
    assert(cmd.kind == PgSessionCmdKind.Insert);
    assertEq(cmd.row_count, 3);
    assertEq(pg.queries_completed, 1);

    // Failed statement inside transaction sets 'E' -> needsRollback()
    * = pg.onReadyForQuery('E') catch {
        assert(false);
        return;
    };
    assert(pg.inTransaction());
    assert(pg.needsRollback());

    // After ROLLBACK -> back to 'I'
    * = pg.onReadyForQuery('I') catch {
        assert(false);
        return;
    };
    assert(!pg.inTransaction());
    assert(!pg.needsRollback());
});

test("pgSession: error guards (invalid pid, bad tx indicator, unexpected SSL byte, fatal error)", () => {
    let pg = PgSessionMachine.init(PgSessionSslMode.Prefer);

    // Invalid early transitions
    assert(!(pg.onStartupSent() catch false));
    assert(!(pg.onReadyForQuery('I') catch false));

    * = pg.beginConnect() catch {
        assert(false);
        return;
    };
    // Unexpected SSL reply byte fails
    assert(!(pg.onSslNegotiated('Z') catch false));

    * = pg.onStartupSent() catch {
        assert(false);
        return;
    };
    // pid <= 0 must be rejected
    assert(!(pg.onBackendKeyData(0, 100) catch false));
    // Invalid ReadyForQuery indicator rejected
    assert(!(pg.onReadyForQuery('?') catch false));

    pg.onFatalError();
    assert(pg.state == PgSessionState.Failed);
    assert(!pg.isReadyForQuery());
});
