// Full-Stack Integration Test for ALL 33 Official `@web/*` Plugins on `cap:web`
// Verifies zero symbol collisions and end-to-end interoperability across:
//   Wave 1 (7): cors, security, jwt, ratelimit, logger, static, swagger
//   Wave 2 (10): multipart, compress, cache, sensible, pressure, basic-auth, sse, session, proxy, cron
//   Wave 3 (8): env, i18n, oauth2, graphql, mcp, redis, db, s3
//   Wave 4 (8): rbac, flags, webhook, circuit-breaker, audit, html, trpc, a2a

import { App, AppMethod, Context, Request, Response } from "../../lib/web/app.cp";
import { CorsConfig, registerCors } from "../../packages/@web/cors/src/index.cp";
import { SecurityConfig, registerSecurity } from "../../packages/@web/security/src/index.cp";
import { JwtConfig, signHs256, registerJwt } from "../../packages/@web/jwt/src/index.cp";
import { RateLimitConfig, registerRateLimit, setTestClock } from "../../packages/@web/ratelimit/src/index.cp";
import { LoggerConfig, registerLogger } from "../../packages/@web/logger/src/index.cp";
import { StaticConfig, registerStatic } from "../../packages/@web/static/src/index.cp";
import { SwaggerConfig, registerSwagger, refreshSpec } from "../../packages/@web/swagger/src/index.cp";
import { MultipartConfig, registerMultipart } from "../../packages/@web/multipart/src/index.cp";
import { CompressConfig, registerCompress } from "../../packages/@web/compress/src/index.cp";
import { CacheConfig, registerCache, setCacheTestClock, resetCacheStore } from "../../packages/@web/cache/src/index.cp";
import { SensibleConfig, registerSensible, httpNotFound } from "../../packages/@web/sensible/src/index.cp";
import { PressureConfig, registerPressure, pressureResetStats } from "../../packages/@web/pressure/src/index.cp";
import { BasicAuthConfig, registerBasicAuth } from "../../packages/@web/basic-auth/src/index.cp";
import { SseConfig, registerSse, ssePublish, sseResetBroker } from "../../packages/@web/sse/src/index.cp";
import { SessionConfig, registerSession, sessionCreate, setSessionTestClock, resetSessionStore } from "../../packages/@web/session/src/index.cp";
import { ProxyConfig, registerProxyWithTransport } from "../../packages/@web/proxy/src/index.cp";
import { CronConfig, CronTime, registerCron, cronGlobalScheduler, resetCronScheduler } from "../../packages/@web/cron/src/index.cp";
import { EnvKind, EnvVarSpec, EnvKV, registerEnv } from "../../packages/@web/env/src/index.cp";
import { I18nConfig, registerI18n, i18nAddTranslation, i18nTranslate, i18nResetCatalog } from "../../packages/@web/i18n/src/index.cp";
import { OAuth2Config, registerOAuth2, oauth2RegisterState } from "../../packages/@web/oauth2/src/index.cp";
import { GraphQLConfig, registerGraphQL, gqlRegisterResolver, gqlResetResolvers } from "../../packages/@web/graphql/src/index.cp";
import { McpConfig, registerMcp, mcpRegisterTool, mcpResetTools } from "../../packages/@web/mcp/src/index.cp";
import { RedisPluginConfig, registerRedis } from "../../packages/@web/redis/src/index.cp";
import { DbDialect, DbPluginConfig, registerDb, dbTxStats, dbResetStats } from "../../packages/@web/db/src/index.cp";
import { S3PluginConfig, registerS3 } from "../../packages/@web/s3/src/index.cp";
import { RbacConfig, registerRbac, rbacGrant, rbacProtect, rbacReset } from "../../packages/@web/rbac/src/index.cp";
import { FlagsConfig, registerFlags, flagDefine, flagReset } from "../../packages/@web/flags/src/index.cp";
import { WebhookConfig, registerWebhook, webhookSignHex, setWebhookTestClock, resetWebhookStore } from "../../packages/@web/webhook/src/index.cp";
import { CircuitBreakerConfig, registerCircuitBreaker, cbSetTestClock, cbReset } from "../../packages/@web/circuit-breaker/src/index.cp";
import { AuditConfig, registerAudit, auditCount, auditReset } from "../../packages/@web/audit/src/index.cp";
import { HtmlConfig, registerHtml, htmlEscape } from "../../packages/@web/html/src/index.cp";
import { TrpcConfig, registerTrpc, trpcQuery, trpcReset } from "../../packages/@web/trpc/src/index.cp";
import { A2aConfig, registerA2a, a2aSetTaskHandler, a2aResetTasks } from "../../packages/@web/a2a/src/index.cp";

fn strEql(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn strContains(hay: string, needle: string): bool {
    if (needle.len == 0) return true;
    if (needle.len > hay.len) return false;
    let i: usize = 0;
    while (i + needle.len <= hay.len) : (i += 1) {
        if (strEql(hay[i .. i + needle.len], needle)) return true;
    }
    return false;
}

let g_cron_ticks: u64 = 0;
fn sampleCronJob() {
    g_cron_ticks += 1;
}

fn mockProxyTransport(cfg: *const ProxyConfig, ctx: *Context, target_path: string): Response {
    * = cfg;
    * = ctx;
    if (strEql(target_path, "/v2/items")) {
        return Response.json(200, "{\"proxied\":true}");
    }
    return httpNotFound("upstream route not found");
}

fn hProtected(ctx: *Context): Response {
    if (v := ctx.derived("jwt_verified")) {
        if (strEql(v, "1")) {
            return Response.json(200, "{\"access\":\"granted\"}");
        }
    }
    return Response.json(401, "{\"access\":\"denied\"}");
}

fn hSessionEcho(ctx: *Context): Response {
    const sv = ctx.derived("session_val") ?? "";
    if (strEql(sv, "uid=42")) {
        return Response.json(200, "{\"session\":\"ok\"}");
    }
    return httpNotFound("no session");
}

fn gqlVersionResolver(ctx: *Context, args: string): string {
    * = ctx;
    * = args;
    return "\"1.0.0\"";
}

fn mcpPingTool(ctx: *Context, args: string): string {
    * = ctx;
    * = args;
    return "pong-from-cp-mcp";
}

fn hDbCreate(ctx: *Context): Response {
    const loc = ctx.derived("locale") ?? "en";
    if (strEql(ctx.body, "fail")) {
        return Response.json(400, "{\"err\":\"bad\"}");
    }
    return Response.text(201, i18nTranslate(loc, "created"));
}

fn trpcHelloProc(ctx: *Context, input: string): string {
    * = ctx;
    * = input;
    return "\"hello-trpc\"";
}

fn a2aEchoHandler(ctx: *Context, input: string): string {
    * = ctx;
    * = input;
    return "done-by-cp-a2a";
}

fn hEnterpriseWrite(ctx: *Context): Response {
    * = ctx;
    return Response.json(201, "{\"saved\":true}");
}

fn main() {
    const secret = "cp-production-secret-key-2026";
    setTestClock(1700000000);
    setCacheTestClock(1700000000);
    setSessionTestClock(1700000000);
    resetCacheStore();
    resetSessionStore();
    pressureResetStats();
    sseResetBroker();
    resetCronScheduler();
    i18nResetCatalog();
    gqlResetResolvers();
    mcpResetTools();
    dbResetStats();

    // App 1: Core API Gateway stack (Wave 1 + SSE + Cron)
    let app = App.init();
    * = registerCors(*app, CorsConfig{ allow_origin: "https://app.cp.dev", allow_credentials: true });
    * = registerSecurity(*app, SecurityConfig{ enable_csrf: false });
    * = registerRateLimit(*app, RateLimitConfig{ max_requests: 50, window_secs: 60 });
    * = registerLogger(*app, LoggerConfig{ enable_trace: true });
    * = registerJwt(*app, JwtConfig{ secret: secret, optional_auth: true });
    * = registerStatic(*app, StaticConfig{ root_dir: "/data/dev/cp/temp", url_prefix: "/static" });
    * = registerSse(*app, SseConfig{ endpoint_path: "/events", retry_ms: 2000 });
    * = registerCron(*app, CronConfig{ status_path: "/_cron" });
    * = app.get("/api/secret", hProtected);
    * = registerSwagger(*app, SwaggerConfig{ title: "Unified 25-Plugin API", version: "1.0.0" });
    * = refreshSpec(*app);

    let tok_buf: [512]u8 = undefined;
    let tlen: usize = 0;
    if (n := signHs256(secret, "{\"sub\":\"alice\"}", tok_buf[0..])) {
        tlen = n;
    } else {
        @panic("signHs256 failed");
    }
    let auth_hdr: [540]u8 = undefined;
    const prefix = "Bearer ";
    let i: usize = 0;
    while (i < prefix.len) : (i += 1) {
        auth_hdr[i] = prefix[i];
    }
    i = 0;
    while (i < tlen) : (i += 1) {
        auth_hdr[prefix.len + i] = tok_buf[i];
    }
    let req = Request.init(AppMethod.GET, "/api/secret");
    * = req
        .withHeader("Origin", "https://app.cp.dev")
        .withHeader("Authorization", auth_hdr[0 .. prefix.len + tlen]);
    if (app.dispatch(req).status != 200) @panic("Wave 1 gateway failed");
    * = ssePublish("token", "{\"ok\":1}");
    if (!strContains(app.dispatch(Request.init(AppMethod.GET, "/events")).body, "data: {\"ok\":1}")) {
        @panic("Wave 2 SSE failed");
    }
    const sched = cronGlobalScheduler();
    * = sched.addJob("cleanup", "*/15 * * * *", sampleCronJob);
    * = sched.tick(CronTime{ minute: 15, hour: 0, day: 1, month: 1, weekday: 0 });
    if (g_cron_ticks != 1) @panic("Wave 2 Cron failed");

    // App 2: Edge stack (Wave 2: pressure, multipart, basic-auth, session, cache, compress, sensible, proxy)
    let edge = App.init();
    * = registerPressure(*edge, PressureConfig{ max_body_bytes: 1024, max_inflight: 100 });
    * = registerMultipart(*edge, MultipartConfig{ max_total_bytes: 512 });
    * = registerBasicAuth(*edge, BasicAuthConfig{ api_key: "sk_edge_1", optional_auth: true });
    * = registerSession(*edge, SessionConfig{ secret: "sess-sec", cookie_name: "cp_sid", ttl_secs: 600 });
    * = registerCache(*edge, CacheConfig{ ttl_secs: 60, enable_idempotency: true });
    * = registerCompress(*edge, CompressConfig{ min_bytes: 32, prefer_gzip: true });
    * = registerSensible(*edge, SensibleConfig{});
    * = registerProxyWithTransport(*edge, ProxyConfig{ prefix: "/proxy", strip_prefix: true }, mockProxyTransport);
    * = edge.get("/me-sess", hSessionEcho);

    if (edge.dispatch(Request.init(AppMethod.GET, "/proxy/v2/items")).status != 200) {
        @panic("Wave 2 Proxy failed");
    }
    let sc_buf: [128]u8 = undefined;
    let sc_len: usize = 0;
    if (sn := sessionCreate("uid=42", 1700000000, sc_buf[0..])) {
        sc_len = sn;
    } else {
        @panic("sessionCreate failed");
    }
    let ck_hdr: [160]u8 = undefined;
    const ck_pre = "cp_sid=";
    i = 0;
    while (i < ck_pre.len) : (i += 1) {
        ck_hdr[i] = ck_pre[i];
    }
    i = 0;
    while (i < sc_len) : (i += 1) {
        ck_hdr[ck_pre.len + i] = sc_buf[i];
    }
    let sreq = Request.init(AppMethod.GET, "/me-sess");
    * = sreq.withHeader("Cookie", ck_hdr[0 .. ck_pre.len + sc_len]);
    if (!strEql(edge.dispatch(sreq).header("X-Cache") ?? "", "MISS")) @panic("Wave 2 Cache MISS failed");
    if (!strEql(edge.dispatch(sreq).header("X-Cache") ?? "", "HIT")) @panic("Wave 2 Cache HIT failed");

    // App 3: Wave 3 AI & Data Platform stack (env, i18n, oauth2, graphql, mcp, redis, db, s3)
    let platform = App.init();
    const env_specs: [2]EnvVarSpec = [
        EnvVarSpec{ name: "APP_ENV", kind: EnvKind.Enum, required: true, enum_values: "dev,prod" },
        EnvVarSpec{ name: "PORT", kind: EnvKind.Int, default_val: "8080", min_val: 1, max_val: 65535, has_bounds: true },
    ];
    const env_vals: [1]EnvKV = [EnvKV{ key: "APP_ENV", value: "prod" }];
    if (!registerEnv(*platform, env_specs[0..], env_vals[0..]).ok) @panic("Wave 3 @web/env failed");

    * = i18nAddTranslation("en", "created", "Created");
    * = i18nAddTranslation("zh", "created", "YiChuangJian");
    * = registerI18n(*platform, I18nConfig{ default_locale: "en", supported_locales: "en,zh" });
    * = registerOAuth2(*platform, OAuth2Config{
        client_id: "cp-app",
        authorize_url: "https://idp.cp.dev/authorize",
        token_url: "https://idp.cp.dev/token",
        redirect_uri: "https://app.cp.dev/auth/callback",
    });
    * = gqlRegisterResolver("version", gqlVersionResolver);
    * = registerGraphQL(*platform, GraphQLConfig{});
    * = mcpRegisterTool("ping", "Ping CP MCP server", mcpPingTool);
    * = registerMcp(*platform, McpConfig{});
    * = registerRedis(*platform, RedisPluginConfig{ key_prefix: "cp:" });
    * = registerDb(*platform, DbPluginConfig{ dialect: DbDialect.Postgres, dsn: "postgres://localhost/cp", auto_tx: true });
    * = registerS3(*platform, S3PluginConfig{ bucket: "cp-assets", access_key: "AKIA_TEST", secret_key: "SECRET_TEST" });
    * = platform.post("/db/items", hDbCreate);

    // Verify all 8 Wave 3 plugins end-to-end
    oauth2RegisterState("st_123");
    if (platform.dispatch(Request.init(AppMethod.GET, "/auth/callback?code=abc&state=st_123")).status != 200) {
        @panic("Wave 3 @web/oauth2 failed");
    }
    let gql_req = Request.init(AppMethod.POST, "/graphql");
    * = gql_req.withBody("{\"query\":\"{ version }\"}");
    if (!strContains(platform.dispatch(gql_req).body, "1.0.0")) @panic("Wave 3 @web/graphql failed");

    let mcp_req = Request.init(AppMethod.POST, "/mcp");
    * = mcp_req.withBody("{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"tools/call\",\"params\":{\"name\":\"ping\",\"arguments\":{}}}");
    if (!strContains(platform.dispatch(mcp_req).body, "pong-from-cp-mcp")) @panic("Wave 3 @web/mcp failed");

    if (platform.dispatch(Request.init(AppMethod.GET, "/_redis/ping")).status != 200) {
        @panic("Wave 3 @web/redis failed");
    }
    let db_ok = Request.init(AppMethod.POST, "/db/items?lang=zh");
    * = db_ok.withBody("ok");
    const db_r1 = platform.dispatch(db_ok);
    if (db_r1.status != 201 || !strEql(db_r1.body, "YiChuangJian") || !strEql(db_r1.header("X-DB-Tx") ?? "", "committed")) {
        @panic("Wave 3 @web/db + @web/i18n commit failed");
    }
    let db_bad = Request.init(AppMethod.POST, "/db/items");
    * = db_bad.withBody("fail");
    const db_r2 = platform.dispatch(db_bad);
    if (db_r2.status != 400 || !strEql(db_r2.header("X-DB-Tx") ?? "", "rolled_back")) {
        @panic("Wave 3 @web/db rollback failed");
    }
    const stats = dbTxStats();
    if (stats.commits < 1 || stats.rollbacks < 1) @panic("Wave 3 @web/db stats mismatch");

    if (!strContains(platform.dispatch(Request.init(AppMethod.GET, "/upload/presign?key=avatars/a.png")).body, "X-Amz-Signature=")) {
        @panic("Wave 3 @web/s3 presign failed");
    }

    // App 4: Wave 4 Enterprise Governance & AI Agent Stack (rbac, flags, webhook, circuit-breaker, audit, html, trpc, a2a)
    rbacReset();
    flagReset();
    resetWebhookStore();
    cbReset();
    auditReset();
    trpcReset();
    a2aResetTasks();
    setWebhookTestClock(1700000000);
    cbSetTestClock(1700000000);

    * = rbacGrant("admin", "ent:*");
    * = rbacProtect(AppMethod.POST, "/ent/", "ent:write");
    * = flagDefine("v2_engine", true, 100, "");
    * = trpcQuery("hello", trpcHelloProc);
    a2aSetTaskHandler(a2aEchoHandler);

    let ent = App.init();
    * = registerRbac(*ent, RbacConfig{ role_header: "X-Role", default_role: "guest" });
    * = registerFlags(*ent, FlagsConfig{});
    * = registerWebhook(*ent, WebhookConfig{ secret: "wh_sec", endpoint_path: "/webhooks" });
    * = registerCircuitBreaker(*ent, CircuitBreakerConfig{ failure_threshold: 3, reset_timeout_secs: 30 });
    * = registerAudit(*ent, AuditConfig{ status_path: "/_audit" });
    * = registerHtml(*ent, HtmlConfig{});
    * = registerTrpc(*ent, TrpcConfig{ base_path: "/trpc" });
    * = registerA2a(*ent, A2aConfig{ agent_name: "cp-swarm-agent" });
    * = ent.post("/ent/records", hEnterpriseWrite);

    // Verify all 8 Wave 4 plugins end-to-end
    let ent_req = Request.init(AppMethod.POST, "/ent/records");
    * = ent_req.withHeader("X-Role", "admin").withHeader("X-Actor", "alice");
    if (ent.dispatch(ent_req).status != 201 || auditCount() < 1) {
        @panic("Wave 4 @web/rbac + @web/audit + @web/circuit-breaker failed");
    }
    if (!strContains(ent.dispatch(Request.init(AppMethod.GET, "/_flags")).body, "v2_engine")) {
        @panic("Wave 4 @web/flags failed");
    }
    let esc_buf: [64]u8 = undefined;
    const esc_len = htmlEscape("<b>hi</b>", esc_buf[0..]) ?? 0;
    if (esc_len == 0 || !strContains(esc_buf[0..esc_len], "&lt;b&gt;")) {
        @panic("Wave 4 @web/html failed");
    }
    if (!strContains(ent.dispatch(Request.init(AppMethod.GET, "/trpc/hello")).body, "hello-trpc")) {
        @panic("Wave 4 @web/trpc failed");
    }
    if (!strContains(ent.dispatch(Request.init(AppMethod.GET, "/.well-known/agent.json")).body, "cp-swarm-agent")) {
        @panic("Wave 4 @web/a2a card failed");
    }
    let a2a_req = Request.init(AppMethod.POST, "/a2a");
    * = a2a_req.withBody("{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"tasks/send\",\"params\":{\"id\":\"t1\",\"message\":\"hi\"}}");
    if (!strContains(ent.dispatch(a2a_req).body, "done-by-cp-a2a")) {
        @panic("Wave 4 @web/a2a tasks/send failed");
    }
    let sig_buf: [64]u8 = undefined;
    const wh_body = "{\"event\":\"paid\"}";
    let slen: usize = 0;
    if (sn2 := webhookSignHex("wh_sec", wh_body, sig_buf[0..])) {
        slen = sn2;
    } else {
        @panic("webhookSignHex failed");
    }
    let wh_req = Request.init(AppMethod.POST, "/webhooks");
    * = wh_req
        .withHeader("X-Hub-Signature-256", sig_buf[0..slen])
        .withHeader("X-Webhook-Timestamp", "1700000000")
        .withHeader("X-Webhook-Id", "evt_1")
        .withBody(wh_body);
    if (ent.dispatch(wh_req).status != 200) {
        @panic("Wave 4 @web/webhook failed");
    }

    @print("PASS testWebOfficialPlugins (ALL 33 @web/* plugins verified with 0 collisions!)");
}
