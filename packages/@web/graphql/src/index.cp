// Official @web/graphql Plugin — GraphQL Endpoint, Depth Guard & GraphiQL Playground for cap:web
// Pure CP: zero zig{}, zero var, zero anyopaque, zero heap GC.

import { App, AppMethod, Context, Response, Plugin, PluginScope } from "../../../../lib/web/app.cp";

pub const GQL_MAX_RESOLVERS: usize = 64;
pub const GQL_QUERY_BUF_CAP: usize = 8192;
pub const GQL_RESP_BUF_CAP: usize = 16384;
pub const GQL_HTML_BUF_CAP: usize = 4096;

pub struct GraphQLConfig {
    endpoint_path: string = "/graphql",
    enable_playground: bool = true,
    max_depth: usize = 8,
}

pub type GraphQLResolverFn = *const fn (*Context, string): string;

struct GqlResolverEntry {
    name: string = "",
    resolver: GraphQLResolverFn? = null,
}

let gql_cfg: GraphQLConfig = GraphQLConfig{};
let gql_resolvers: [GQL_MAX_RESOLVERS]GqlResolverEntry = [GqlResolverEntry{}; GQL_MAX_RESOLVERS];
let gql_resolver_len: usize = 0;
let gql_query_buf: [GQL_QUERY_BUF_CAP]u8 = [0; GQL_QUERY_BUF_CAP];
let gql_resp_buf: [GQL_RESP_BUF_CAP]u8 = [0; GQL_RESP_BUF_CAP];
let gql_html_buf: [GQL_HTML_BUF_CAP]u8 = [0; GQL_HTML_BUF_CAP];
let gql_html_len: usize = 0;

fn gqlBytesEql(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn gqlWriteByte(out_buf: []u8, pos: *usize, b: u8): bool {
    if (pos.* >= out_buf.len) return false;
    out_buf[pos.*] = b;
    pos.* += 1;
    return true;
}

fn gqlWriteRaw(out_buf: []u8, pos: *usize, s: string): bool {
    if (s.len > out_buf.len || pos.* > out_buf.len - s.len) return false;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        out_buf[pos.* + i] = s[i];
    }
    pos.* += s.len;
    return true;
}

fn gqlHexLower(n: u8): u8 {
    return if (n < 10) ('0' + n) else ('a' + (n - 10));
}

fn gqlWriteJsonEscaped(out_buf: []u8, pos: *usize, s: string): bool {
    if (!gqlWriteByte(out_buf, pos, '"')) return false;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        const b = s[i];
        if (b == '"') {
            if (!gqlWriteRaw(out_buf, pos, "\\\"")) return false;
        } else if (b == '\\') {
            if (!gqlWriteRaw(out_buf, pos, "\\\\")) return false;
        } else if (b == '\n') {
            if (!gqlWriteRaw(out_buf, pos, "\\n")) return false;
        } else if (b == '\r') {
            if (!gqlWriteRaw(out_buf, pos, "\\r")) return false;
        } else if (b == '\t') {
            if (!gqlWriteRaw(out_buf, pos, "\\t")) return false;
        } else if (b < 0x20) {
            if (!gqlWriteRaw(out_buf, pos, "\\u00")) return false;
            if (!gqlWriteByte(out_buf, pos, gqlHexLower((b >> 4) & 0x0F))) return false;
            if (!gqlWriteByte(out_buf, pos, gqlHexLower(b & 0x0F))) return false;
        } else {
            if (!gqlWriteByte(out_buf, pos, b)) return false;
        }
    }
    return gqlWriteByte(out_buf, pos, '"');
}

fn gqlIsWs(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == ',';
}

fn gqlIsIdentStart(c: u8): bool {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_';
}

fn gqlIsIdentCont(c: u8): bool {
    return gqlIsIdentStart(c) || (c >= '0' && c <= '9');
}

fn gqlSkipWsAndComments(s: string, start: usize): usize {
    let i: usize = start;
    while (i < s.len) {
        if (gqlIsWs(s[i])) {
            i += 1;
        } else if (s[i] == '#') {
            while (i < s.len && s[i] != '\n') : (i += 1) {}
        } else {
            break;
        }
    }
    return i;
}

/// Check that `{` nesting depth in `query` does not exceed `max_depth` (ignores string literals & `#` comments).
pub fn gqlCheckQueryDepth(query: string, max_depth: usize): bool {
    let depth: usize = 0;
    let i: usize = 0;
    while (i < query.len) {
        const c = query[i];
        if (c == '#') {
            while (i < query.len && query[i] != '\n') : (i += 1) {}
            continue;
        }
        if (c == '"') {
            // Handle GraphQL block strings `"""..."""`
            if (i + 2 < query.len && query[i + 1] == '"' && query[i + 2] == '"') {
                i += 3;
                while (i + 2 < query.len) {
                    if (query[i] == '\\' && i + 3 < query.len && query[i + 1] == '"' && query[i + 2] == '"' && query[i + 3] == '"') {
                        i += 4;
                        continue;
                    }
                    if (query[i] == '"' && query[i + 1] == '"' && query[i + 2] == '"') {
                        i += 3;
                        break;
                    }
                    i += 1;
                }
                continue;
            }
            // Normal double-quoted string literal
            i += 1;
            while (i < query.len) {
                if (query[i] == '\\' && i + 1 < query.len) {
                    i += 2;
                    continue;
                }
                if (query[i] == '"') {
                    i += 1;
                    break;
                }
                i += 1;
            }
            continue;
        }
        if (c == '{') {
            depth += 1;
            if (depth > max_depth) return false;
        } else if (c == '}') {
            if (depth > 0) {
                depth -= 1;
            }
        }
        i += 1;
    }
    return true;
}

/// Register a GraphQL field resolver. Overwrites if `field_name` is already registered.
pub fn gqlRegisterResolver(field_name: string, resolver: GraphQLResolverFn): bool {
    if (field_name.len == 0) return false;
    let i: usize = 0;
    while (i < gql_resolver_len) : (i += 1) {
        if (gqlBytesEql(gql_resolvers[i].name, field_name)) {
            gql_resolvers[i].resolver = resolver;
            return true;
        }
    }
    if (gql_resolver_len >= GQL_MAX_RESOLVERS) return false;
    gql_resolvers[gql_resolver_len] = GqlResolverEntry{
        name: field_name,
        resolver: resolver,
    };
    gql_resolver_len += 1;
    return true;
}

/// Remove all registered GraphQL field resolvers.
pub fn gqlResetResolvers() {
    gql_resolver_len = 0;
}

fn gqlFindResolver(field_name: string): GraphQLResolverFn? {
    let i: usize = 0;
    while (i < gql_resolver_len) : (i += 1) {
        if (gqlBytesEql(gql_resolvers[i].name, field_name)) {
            return gql_resolvers[i].resolver;
        }
    }
    return null;
}

fn gqlSkipJsonWs(s: string, start: usize): usize {
    let i: usize = start;
    while (i < s.len && (s[i] == ' ' || s[i] == '\t' || s[i] == '\r' || s[i] == '\n')) : (i += 1) {}
    return i;
}

/// Extract the unescaped string value of `"query"` from a JSON object body into `out_buf`.
fn gqlExtractJsonQuery(body: string, out_buf: []u8): usize? {
    let i: usize = 0;
    while (i < body.len) {
        if (body[i] == '"') {
            const key_start = i + 1;
            i += 1;
            while (i < body.len) {
                if (body[i] == '\\' && i + 1 < body.len) {
                    i += 2;
                    continue;
                }
                if (body[i] == '"') break;
                i += 1;
            }
            if (i >= body.len) return null;
            const key = body[key_start..i];
            i += 1; // skip closing quote
            const after_ws = gqlSkipJsonWs(body, i);
            if (after_ws < body.len && body[after_ws] == ':') {
                const val_start = gqlSkipJsonWs(body, after_ws + 1);
                if (gqlBytesEql(key, "query") && val_start < body.len && body[val_start] == '"') {
                    let p: usize = val_start + 1;
                    let out_pos: usize = 0;
                    while (p < body.len) {
                        const ch = body[p];
                        if (ch == '\\' && p + 1 < body.len) {
                            const nxt = body[p + 1];
                            if (nxt == '"') {
                                if (!gqlWriteByte(out_buf, *out_pos, '"')) return null;
                            } else if (nxt == '\\') {
                                if (!gqlWriteByte(out_buf, *out_pos, '\\')) return null;
                            } else if (nxt == 'n') {
                                if (!gqlWriteByte(out_buf, *out_pos, '\n')) return null;
                            } else if (nxt == 'r') {
                                if (!gqlWriteByte(out_buf, *out_pos, '\r')) return null;
                            } else if (nxt == 't') {
                                if (!gqlWriteByte(out_buf, *out_pos, '\t')) return null;
                            } else {
                                if (!gqlWriteByte(out_buf, *out_pos, nxt)) return null;
                            }
                            p += 2;
                            continue;
                        }
                        if (ch == '"') {
                            return out_pos;
                        }
                        if (!gqlWriteByte(out_buf, *out_pos, ch)) return null;
                        p += 1;
                    }
                    return null;
                }
            }
            continue;
        }
        i += 1;
    }
    return null;
}

fn gqlLooksLikeJsonValue(val: string): bool {
    if (val.len == 0) return false;
    const c = val[0];
    if (c == '"' || c == '{' || c == '[' || c == '-' || (c >= '0' && c <= '9')) return true;
    if (gqlBytesEql(val, "true") || gqlBytesEql(val, "false") || gqlBytesEql(val, "null")) return true;
    return false;
}

fn gqlSkipBalanced(s: string, start: usize, open_ch: u8, close_ch: u8): usize {
    if (start >= s.len || s[start] != open_ch) return start;
    let depth: usize = 1;
    let i: usize = start + 1;
    while (i < s.len && depth > 0) {
        if (s[i] == '"') {
            i += 1;
            while (i < s.len) {
                if (s[i] == '\\' && i + 1 < s.len) {
                    i += 2;
                    continue;
                }
                if (s[i] == '"') {
                    i += 1;
                    break;
                }
                i += 1;
            }
            continue;
        }
        if (s[i] == open_ch) {
            depth += 1;
        } else if (s[i] == close_ch) {
            depth -= 1;
        }
        i += 1;
    }
    return i;
}

fn gqlRenderPlaygroundHtml(endpoint: string, out_buf: []u8): usize? {
    let pos: usize = 0;
    if (!gqlWriteRaw(out_buf, *pos, "<!doctype html><html lang=\"en\"><head><meta charset=\"utf-8\"/><title>GraphiQL Playground</title></head><body><div id=\"graphiql\" data-endpoint=\"")) return null;
    if (!gqlWriteRaw(out_buf, *pos, endpoint)) return null;
    if (!gqlWriteRaw(out_buf, *pos, "\"><h1>GraphiQL Playground</h1><p>Endpoint: ")) return null;
    if (!gqlWriteRaw(out_buf, *pos, endpoint)) return null;
    if (!gqlWriteRaw(out_buf, *pos, "</p></div></body></html>")) return null;
    return pos;
}

fn gqlGetHandler(ctx: *Context): Response {
    * = ctx;
    if (!gql_cfg.enable_playground) {
        return Response.json(404, "{\"errors\":[{\"message\":\"playground_disabled\"}]}");
    }
    return Response.html(200, gql_html_buf[0..gql_html_len]);
}

fn gqlPostHandler(ctx: *Context): Response {
    let qlen: usize = 0;
    if (n := gqlExtractJsonQuery(ctx.body, gql_query_buf[0..])) {
        qlen = n;
    } else {
        return Response.json(400, "{\"errors\":[{\"message\":\"missing_query\"}]}");
    }

    const query = gql_query_buf[0..qlen];
    if (!gqlCheckQueryDepth(query, gql_cfg.max_depth)) {
        return Response.json(400, "{\"errors\":[{\"message\":\"max_depth_exceeded\"}]}");
    }

    // Locate the opening `{` of the root selection set.
    let i: usize = 0;
    while (i < query.len && query[i] != '{') : (i += 1) {
        if (query[i] == '#') {
            while (i < query.len && query[i] != '\n') : (i += 1) {}
        }
    }
    if (i >= query.len || query[i] != '{') {
        return Response.json(400, "{\"errors\":[{\"message\":\"invalid_query\"}]}");
    }
    i += 1; // step inside root `{`

    let pos: usize = 0;
    if (!gqlWriteRaw(gql_resp_buf[0..], *pos, "{\"data\":{")) {
        return Response.json(500, "{\"errors\":[{\"message\":\"buffer_overflow\"}]}");
    }

    let matched_count: usize = 0;
    while (i < query.len) {
        i = gqlSkipWsAndComments(query, i);
        if (i >= query.len || query[i] == '}') break;

        if (!gqlIsIdentStart(query[i])) {
            i += 1;
            continue;
        }
        const name_start = i;
        while (i < query.len && gqlIsIdentCont(query[i])) : (i += 1) {}
        let field_name = query[name_start..i];
        let resp_key = field_name;

        i = gqlSkipWsAndComments(query, i);
        // Handle GraphQL alias `alias: realField`
        if (i < query.len && query[i] == ':') {
            i = gqlSkipWsAndComments(query, i + 1);
            if (i < query.len && gqlIsIdentStart(query[i])) {
                const real_start = i;
                while (i < query.len && gqlIsIdentCont(query[i])) : (i += 1) {}
                field_name = query[real_start..i];
                i = gqlSkipWsAndComments(query, i);
            }
        }

        // Optional arguments `(...)`
        let args_str: string = "";
        if (i < query.len && query[i] == '(') {
            const args_end = gqlSkipBalanced(query, i, '(', ')');
            if (args_end > i + 1) {
                args_str = query[i + 1 .. args_end - 1];
            }
            i = gqlSkipWsAndComments(query, args_end);
        }

        // Optional nested selection set `{...}`
        if (i < query.len && query[i] == '{') {
            i = gqlSkipBalanced(query, i, '{', '}');
        }

        if (resolver_fn := gqlFindResolver(field_name)) {
            const val = resolver_fn(ctx, args_str);
            if (matched_count > 0) {
                if (!gqlWriteByte(gql_resp_buf[0..], *pos, ',')) {
                    return Response.json(500, "{\"errors\":[{\"message\":\"buffer_overflow\"}]}");
                }
            }
            if (!gqlWriteJsonEscaped(gql_resp_buf[0..], *pos, resp_key)) {
                return Response.json(500, "{\"errors\":[{\"message\":\"buffer_overflow\"}]}");
            }
            if (!gqlWriteByte(gql_resp_buf[0..], *pos, ':')) {
                return Response.json(500, "{\"errors\":[{\"message\":\"buffer_overflow\"}]}");
            }
            if (gqlLooksLikeJsonValue(val)) {
                if (!gqlWriteRaw(gql_resp_buf[0..], *pos, val)) {
                    return Response.json(500, "{\"errors\":[{\"message\":\"buffer_overflow\"}]}");
                }
            } else {
                if (!gqlWriteJsonEscaped(gql_resp_buf[0..], *pos, val)) {
                    return Response.json(500, "{\"errors\":[{\"message\":\"buffer_overflow\"}]}");
                }
            }
            matched_count += 1;
        }
    }

    if (matched_count == 0) {
        return Response.json(200, "{\"data\":null,\"errors\":[{\"message\":\"field_not_found\"}]}");
    }
    if (!gqlWriteRaw(gql_resp_buf[0..], *pos, "}}")) {
        return Response.json(500, "{\"errors\":[{\"message\":\"buffer_overflow\"}]}");
    }
    return Response.json(200, gql_resp_buf[0..pos]);
}

/// Register GraphQL GET (Playground) and POST (Query Execution) endpoints on `app`.
pub fn registerGraphQL(app: *App, cfg: GraphQLConfig): *App {
    gql_cfg = cfg;
    if (hlen := gqlRenderPlaygroundHtml(cfg.endpoint_path, gql_html_buf[0..])) {
        gql_html_len = hlen;
    } else {
        gql_html_len = 0;
    }

    let p = Plugin.init("@web/graphql");
    * = p.withScope(PluginScope.Global);
    * = p.route(AppMethod.GET, cfg.endpoint_path, "GraphiQL Playground", &gqlGetHandler);
    * = p.route(AppMethod.POST, cfg.endpoint_path, "GraphQL Query", &gqlPostHandler);
    return app.usePlugin(&p);
}
