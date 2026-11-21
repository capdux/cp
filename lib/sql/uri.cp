// Source: Absorbed from /data/dev/bun/src/sql/postgres/{CommandTag,SSLMode}.cp and sql/mysql/SSLMode.cp
// Pure-CP zero-allocation SQL Connection URI parser (PostgreSQL / MySQL / SQLite),
// SSLMode negotiation flags, and PostgreSQL CommandTag parser.

pub enum SqlUriDialect: u8 {
    postgres = 0,
    mysql = 1,
    sqlite = 2,

    pub fn defaultPort(): u16 {
        return switch (_) {
            .postgres => 5432,
            .mysql => 3306,
            .sqlite => 0,
        };
    }

    pub fn asStr(): []const u8 {
        return switch (_) {
            .postgres => "postgres",
            .mysql => "mysql",
            .sqlite => "sqlite",
        };
    }
}

pub enum SslMode: u8 {
    disable = 0,
    allow = 1,
    prefer = 2,
    require = 3,
    verify_ca = 4,
    verify_full = 5,

    pub static fn parse(s: []const u8): SslMode? {
        if (eqIgnoreCase(s, "disable") || eqIgnoreCase(s, "disabled") || eqIgnoreCase(s, "false") || strEq(s, "0")) return .disable;
        if (eqIgnoreCase(s, "allow")) return .allow;
        if (eqIgnoreCase(s, "prefer") || eqIgnoreCase(s, "preferred")) return .prefer;
        if (eqIgnoreCase(s, "require") || eqIgnoreCase(s, "required") || eqIgnoreCase(s, "true") || strEq(s, "1")) return .require;
        if (eqIgnoreCase(s, "verify-ca") || eqIgnoreCase(s, "verify_ca")) return .verify_ca;
        if (eqIgnoreCase(s, "verify-full") || eqIgnoreCase(s, "verify_full") || eqIgnoreCase(s, "verify_identity")) return .verify_full;
        return null;
    }

    pub fn asStr(): []const u8 {
        return switch (_) {
            .disable => "disable",
            .allow => "allow",
            .prefer => "prefer",
            .require => "require",
            .verify_ca => "verify-ca",
            .verify_full => "verify-full",
        };
    }

    pub fn requiresTls(): bool {
        return _ == .require || _ == .verify_ca || _ == .verify_full;
    }

    pub fn verifiesCert(): bool {
        return _ == .verify_ca || _ == .verify_full;
    }

    pub fn verifiesHostname(): bool {
        return _ == .verify_full;
    }
}

pub enum UriPgCommandKind: u8 {
    insert = 0,
    delete = 1,
    update = 2,
    merge = 3,
    select = 4,
    move_cmd = 5,
    fetch = 6,
    copy = 7,
    other = 8,
}

/// Parsed PostgreSQL `CommandComplete` tag (e.g. `INSERT 0 42`, `UPDATE 7`, `CREATE TABLE`).
pub struct UriPgCommandTag {
    kind: UriPgCommandKind,
    rows_affected: u64,
    raw: []const u8,

    pub static fn parse(tag: []const u8): UriPgCommandTag {
        const sp = findChar(tag, ' ');
        if (sp == null) {
            return UriPgCommandTag{ kind: .other, rows_affected: 0, raw: tag };
        }
        const sp_idx = sp.?;
        const cmd = tag[0..sp_idx];
        const rest = tag[sp_idx + 1 ..];

        if (strEq(cmd, "INSERT")) {
            // Format: `INSERT <oid> <rows>`
            const sp2 = findChar(rest, ' ') ?? return UriPgCommandTag{ kind: .other, rows_affected: 0, raw: tag };
            const rows = parseU64(rest[sp2 + 1 ..]) ?? return UriPgCommandTag{ kind: .other, rows_affected: 0, raw: tag };
            return UriPgCommandTag{ kind: .insert, rows_affected: rows, raw: tag };
        }

        let k: UriPgCommandKind = .other;
        if (strEq(cmd, "DELETE")) {
            k = .delete;
        } else if (strEq(cmd, "UPDATE")) {
            k = .update;
        } else if (strEq(cmd, "MERGE")) {
            k = .merge;
        } else if (strEq(cmd, "SELECT")) {
            k = .select;
        } else if (strEq(cmd, "MOVE")) {
            k = .move_cmd;
        } else if (strEq(cmd, "FETCH")) {
            k = .fetch;
        } else if (strEq(cmd, "COPY")) {
            k = .copy;
        } else {
            return UriPgCommandTag{ kind: .other, rows_affected: 0, raw: tag };
        }

        const rows = parseU64(rest) ?? return UriPgCommandTag{ kind: .other, rows_affected: 0, raw: tag };
        return UriPgCommandTag{ kind: k, rows_affected: rows, raw: tag };
    }
}

/// Parsed SQL database connection URI (`postgres://`, `mysql://`, `sqlite://`).
pub struct SqlConnUri {
    dialect: SqlUriDialect,
    username: []const u8,
    password: []const u8,
    host: []const u8,
    port: u16,
    database: []const u8,
    ssl_mode: SslMode,
    application_name: []const u8,

    pub static fn parse(uri: []const u8): SqlConnUri? {
        let dialect: SqlUriDialect = .postgres;
        let rest: []const u8 = uri;

        if (startsWithIgnoreCase(uri, "postgres://")) {
            dialect = .postgres;
            rest = uri[11..];
        } else if (startsWithIgnoreCase(uri, "postgresql://")) {
            dialect = .postgres;
            rest = uri[13..];
        } else if (startsWithIgnoreCase(uri, "mysql://")) {
            dialect = .mysql;
            rest = uri[8..];
        } else if (startsWithIgnoreCase(uri, "mysql2://")) {
            dialect = .mysql;
            rest = uri[9..];
        } else if (startsWithIgnoreCase(uri, "sqlite://")) {
            dialect = .sqlite;
            rest = uri[9..];
        } else {
            return null;
        }

        let res = SqlConnUri{
            dialect: dialect,
            username: "",
            password: "",
            host: if (dialect == .sqlite) "" else "localhost",
            port: dialect.defaultPort(),
            database: "",
            ssl_mode: .prefer,
            application_name: "",
        };

        // Split query string `?k=v&...`
        let query: []const u8 = "";
        if (q_idx := findChar(rest, '?')) {
            query = rest[q_idx + 1 ..];
            rest = rest[0..q_idx];
        }

        if (dialect == .sqlite) {
            res.database = rest;
            res.ssl_mode = .disable;
            applyQueryParams(&res, query);
            return res;
        }

        // Split authority and `/database`
        let authority = rest;
        if (slash := findChar(rest, '/')) {
            authority = rest[0..slash];
            res.database = rest[slash + 1 ..];
        }

        // Split optional `user[:pass]@host[:port]` (use last '@' to allow '@' in password)
        let hostport = authority;
        if (at := findLastChar(authority, '@')) {
            const userinfo = authority[0..at];
            hostport = authority[at + 1 ..];
            if (colon := findChar(userinfo, ':')) {
                res.username = userinfo[0..colon];
                res.password = userinfo[colon + 1 ..];
            } else {
                res.username = userinfo;
            }
        }

        // Parse host[:port] (handle IPv6 `[::1]:5432`)
        if (hostport.len > 0) {
            if (hostport[0] == '[') {
                const close_b = findChar(hostport, ']') ?? return null;
                res.host = hostport[1..close_b];
                if (close_b + 1 < hostport.len) {
                    if (hostport[close_b + 1] != ':') return null;
                    const p = parseU16(hostport[close_b + 2 ..]) ?? return null;
                    if (p == 0) return null;
                    res.port = p;
                }
            } else if (colon := findLastChar(hostport, ':')) {
                if (colon > 0) res.host = hostport[0..colon];
                const p_str = hostport[colon + 1 ..];
                if (p_str.len > 0) {
                    const p = parseU16(p_str) ?? return null;
                    if (p == 0) return null;
                    res.port = p;
                }
            } else {
                res.host = hostport;
            }
        }

        applyQueryParams(&res, query);
        return res;
    }
}

fn applyQueryParams(res: *SqlConnUri, query: []const u8) {
    let pos: usize = 0;
    while (pos < query.len) {
        const start = pos;
        while (pos < query.len && query[pos] != '&') : (pos += 1) {}
        const pair = query[start..pos];
        if (pos < query.len && query[pos] == '&') pos += 1;

        if (eq := findChar(pair, '=')) {
            const k = pair[0..eq];
            const v = pair[eq + 1 ..];
            if (eqIgnoreCase(k, "sslmode") || eqIgnoreCase(k, "ssl-mode") || eqIgnoreCase(k, "ssl")) {
                if (m := SslMode.parse(v)) res.ssl_mode = m;
            } else if (eqIgnoreCase(k, "application_name")) {
                res.application_name = v;
            }
        }
    }
}

// ---------- Helpers ----------

fn findChar(s: []const u8, ch: u8): usize? {
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        if (s[i] == ch) return i;
    }
    return null;
}

fn findLastChar(s: []const u8, ch: u8): usize? {
    let i = s.len;
    while (i > 0) {
        i -= 1;
        if (s[i] == ch) return i;
    }
    return null;
}

fn parseU16(s: []const u8): u16? {
    if (s.len == 0) return null;
    let acc: u32 = 0;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        if (s[i] < '0' || s[i] > '9') return null;
        acc = acc * 10 + @as(u32, s[i] - '0');
        if (acc > 65535) return null;
    }
    return @intCast(acc);
}

fn parseU64(s: []const u8): u64? {
    if (s.len == 0) return null;
    let acc: u64 = 0;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        if (s[i] < '0' || s[i] > '9') return null;
        acc = acc * 10 + @as(u64, s[i] - '0');
    }
    return acc;
}

fn toLower(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') return c + 32;
    return c;
}

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn eqIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (toLower(a[i]) != toLower(b[i])) return false;
    }
    return true;
}

fn startsWithIgnoreCase(s: []const u8, prefix: []const u8): bool {
    if (s.len < prefix.len) return false;
    return eqIgnoreCase(s[0..prefix.len], prefix);
}
