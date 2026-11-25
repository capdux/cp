// Real Web Framework Benchmark: Node.js + Koa (24 routes, 1c / 8c / 16c node:cluster)
// Usage: node fw_node_koa.mjs --port 18333 --workers 1

import cluster from "node:cluster";
import { createRequire } from "node:module";

const require = createRequire("/data/dev/cp/temp/fw_node_deps/package.json");
const Koa = require("koa");
const Router = require("@koa/router");

function parseArgs() {
  let port = 18333;
  let workers = 1;
  const argv = process.argv.slice(2);
  for (let i = 0; i < argv.length; i++) {
    if (argv[i] === "--port" && argv[i + 1]) port = parseInt(argv[++i], 10);
    else if (argv[i] === "--workers" && argv[i + 1]) workers = parseInt(argv[++i], 10);
  }
  return { port, workers };
}

function extractCookie(cookieHeader, name) {
  if (!cookieHeader) return "";
  const prefix = name + "=";
  let start = 0;
  while (start < cookieHeader.length) {
    let end = cookieHeader.indexOf(";", start);
    if (end === -1) end = cookieHeader.length;
    let s = start;
    while (s < end && (cookieHeader.charCodeAt(s) === 32 || cookieHeader.charCodeAt(s) === 9)) s++;
    let e = end;
    while (e > s && (cookieHeader.charCodeAt(e - 1) === 32 || cookieHeader.charCodeAt(e - 1) === 9)) e--;
    if (e - s >= prefix.length && cookieHeader.startsWith(prefix, s)) {
      return cookieHeader.slice(s + prefix.length, e);
    }
    start = end + 1;
  }
  return "";
}

function extractBearer(authHeader) {
  if (!authHeader) return "";
  if (authHeader.length > 7 && authHeader.slice(0, 7).toLowerCase() === "bearer ") {
    return authHeader.slice(7).trim();
  }
  return "";
}

function readJsonBody(req) {
  return new Promise((resolve) => {
    const chunks = [];
    req.on("data", (chunk) => chunks.push(chunk));
    req.on("end", () => {
      if (chunks.length === 0) {
        resolve(null);
        return;
      }
      try {
        const raw = chunks.length === 1 ? chunks[0].toString("utf8") : Buffer.concat(chunks).toString("utf8");
        resolve(JSON.parse(raw));
      } catch {
        resolve(null);
      }
    });
    req.on("error", () => resolve(null));
  });
}

const { port, workers } = parseArgs();

if (workers > 1 && cluster.isPrimary) {
  for (let i = 0; i < workers; i++) {
    cluster.fork();
  }
  const cleanup = () => {
    for (const id in cluster.workers) {
      cluster.workers[id]?.kill();
    }
    process.exit(0);
  };
  process.on("SIGINT", cleanup);
  process.on("SIGTERM", cleanup);
  console.log(`[node-koa] cluster primary listening on http://127.0.0.1:${port} (workers=${workers})`);
} else {
  const app = new Koa();
  const router = new Router();

  // Global middleware: extract Cookie `sid` and `Authorization: Bearer <token>`
  app.use(async (ctx, next) => {
    const cookieHeader = ctx.headers.cookie;
    const authHeader = ctx.headers.authorization;
    ctx.state.authSid = cookieHeader ? extractCookie(cookieHeader, "sid") : "";
    ctx.state.authToken = authHeader ? extractBearer(authHeader) : "";
    await next();
  });

  // --- 12 Static Routes ---
  router.get("/", (ctx) => { ctx.body = { service: "cap-web", version: "1.0" }; });
  router.get("/health", (ctx) => { ctx.body = { ok: true, service: "cap-web" }; });
  router.get("/ready", (ctx) => { ctx.body = { ready: true }; });
  router.get("/version", (ctx) => { ctx.body = { version: "1.0.0" }; });
  router.get("/metrics", (ctx) => { ctx.body = { uptime: 1, requests: 0 }; });
  router.get("/api/v1/me", (ctx) => { ctx.body = { sid: ctx.state.authSid, token: ctx.state.authToken }; });
  router.get("/api/v1/status", (ctx) => { ctx.body = { status: "operational" }; });
  router.get("/api/v1/config", (ctx) => { ctx.body = { env: "prod", region: "us-east-1" }; });
  router.get("/api/v1/users", (ctx) => { ctx.body = { items: [], total: 0 }; });
  router.get("/api/v1/products", (ctx) => { ctx.body = { items: [], total: 0 }; });
  router.get("/api/v1/orders", (ctx) => { ctx.body = { items: [], total: 0 }; });
  router.get("/api/v1/catalog/categories", (ctx) => { ctx.body = { categories: ["gpu", "cpu", "net"] }; });

  // --- 8 Dynamic :param Routes ---
  router.get("/api/v1/users/:id", (ctx) => { ctx.body = { userId: ctx.params.id }; });
  router.get("/api/v1/users/:id/profile", (ctx) => { ctx.body = { userId: ctx.params.id, role: "member" }; });
  router.get("/api/v1/users/:id/posts/:postId", (ctx) => {
    ctx.body = {
      userId: ctx.params.id,
      postId: ctx.params.postId,
      lang: typeof ctx.query.lang === "string" ? ctx.query.lang : "",
      limit: typeof ctx.query.limit === "string" ? ctx.query.limit : "",
    };
  });
  router.get("/api/v1/products/:sku", (ctx) => { ctx.body = { sku: ctx.params.sku }; });
  router.get("/api/v1/products/:sku/reviews/:reviewId", (ctx) => {
    ctx.body = {
      sku: ctx.params.sku,
      reviewId: ctx.params.reviewId,
    };
  });
  router.get("/api/v1/orders/:orderId", (ctx) => { ctx.body = { orderId: ctx.params.orderId }; });
  router.get("/api/v1/teams/:teamId/members/:memberId", (ctx) => {
    ctx.body = {
      teamId: ctx.params.teamId,
      memberId: ctx.params.memberId,
    };
  });
  router.get("/api/v1/orgs/:orgId/projects/:projId", (ctx) => {
    ctx.body = {
      orgId: ctx.params.orgId,
      projId: ctx.params.projId,
    };
  });

  // --- 2 Wildcard Routes (@koa/router v15 path-to-regexp v8 syntax) ---
  router.get("/static/*splat", (ctx) => {
    const s = ctx.params.splat;
    ctx.body = { asset: Array.isArray(s) ? s.join("/") : (s ?? "") };
  });
  router.get("/files/*splat", (ctx) => {
    const s = ctx.params.splat;
    ctx.body = { file: Array.isArray(s) ? s.join("/") : (s ?? "") };
  });

  // --- 2 POST JSON Routes ---
  router.post("/api/v1/orders", async (ctx) => {
    const b = await readJsonBody(ctx.req);
    if (
      !b ||
      typeof b.item !== "string" ||
      b.item.length === 0 ||
      typeof b.qty !== "number" ||
      b.qty <= 0 ||
      typeof b.price !== "number" ||
      b.price <= 0
    ) {
      ctx.status = 400;
      ctx.body = { accepted: false, error: "invalid_order" };
      return;
    }
    ctx.body = { accepted: true, item: b.item, qty: b.qty };
  });

  router.post("/api/v1/events", async (ctx) => {
    const b = await readJsonBody(ctx.req);
    if (!b || typeof b.type !== "string" || b.type.length === 0) {
      ctx.status = 400;
      ctx.body = { recorded: false, error: "invalid_event" };
      return;
    }
    ctx.body = { recorded: true };
  });

  app.use(router.routes());
  app.use(router.allowedMethods());

  const server = app.listen(port, "127.0.0.1", () => {
    if (workers === 1) {
      console.log(`[node-koa] listening on http://127.0.0.1:${port} (workers=1)`);
    }
  });
  server.keepAliveTimeout = 60000;
}
