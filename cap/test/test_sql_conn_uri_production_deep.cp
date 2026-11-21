import { test, expect } from "cap:test";
import {
    SqlDialect,
    SslMode,
    PgCommandKind,
    PgCommandTag,
    SqlConnUri,
} from "cap:sql_conn_uri";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

test("sql_conn_uri: parse postgres, mysql, sqlite URIs and SSLMode", () => {
    const pg = SqlConnUri.parse("postgresql://alice:s3cr3t@[::1]:5433/mydb?sslmode=verify-full&application_name=cap_srv").?;
    expect(pg.dialect == SqlDialect.postgres);
    expect(strEq(pg.username, "alice") && strEq(pg.password, "s3cr3t"));
    expect(strEq(pg.host, "::1") && pg.port == 5433);
    expect(strEq(pg.database, "mydb"));
    expect(pg.ssl_mode == SslMode.verify_full && pg.ssl_mode.requiresTls() && pg.ssl_mode.verifiesHostname());
    expect(strEq(pg.application_name, "cap_srv"));

    const my = SqlConnUri.parse("mysql://root@db.internal/shop?ssl-mode=required").?;
    expect(my.dialect == SqlDialect.mysql && my.port == 3306);
    expect(strEq(my.host, "db.internal") && strEq(my.database, "shop"));
    expect(my.ssl_mode == SslMode.require && my.ssl_mode.requiresTls() && !my.ssl_mode.verifiesCert());

    const sq = SqlConnUri.parse("sqlite:///var/data/app.db").?;
    expect(sq.dialect == SqlDialect.sqlite && sq.port == 0);
    expect(strEq(sq.database, "/var/data/app.db") && sq.ssl_mode == SslMode.disable);

    expect(SqlConnUri.parse("http://not-sql.com") == null);
    expect(SqlConnUri.parse("postgres://host:99999/db") == null);
});

test("sql_conn_uri: PostgreSQL CommandTag parsing", () => {
    const t1 = PgCommandTag.parse("INSERT 0 42");
    expect(t1.kind == PgCommandKind.insert && t1.rows_affected == 42);

    const t2 = PgCommandTag.parse("UPDATE 7");
    expect(t2.kind == PgCommandKind.update && t2.rows_affected == 7);

    const t3 = PgCommandTag.parse("DELETE 0");
    expect(t3.kind == PgCommandKind.delete && t3.rows_affected == 0);

    const t4 = PgCommandTag.parse("COPY 1024");
    expect(t4.kind == PgCommandKind.copy && t4.rows_affected == 1024);

    const t5 = PgCommandTag.parse("CREATE TABLE");
    expect(t5.kind == PgCommandKind.other && strEq(t5.raw, "CREATE TABLE"));
});
