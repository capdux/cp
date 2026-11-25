// Real Web Framework Benchmark: Node.js + Fastify (24 routes, 1c / 8c / 16c node:cluster)
// Usage: node fw_node_fastify.mjs --port 18331 --workers 1

import cluster from "node:cluster";
import fastifyMod from "/data/dev/cp/temp/fw_node_deps/node_modules/fastify/fastify.js";

const Fastify = fastifyMod.default || fastifyMod;

function parseArgs() {
  let port = 18331;
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
  console.log(`[node-fastify] cluster primary listening on http://127.0.0.1:${port} (workers=${workers})`);
} else {
  const app = Fastify({ logger: false, keepAliveTimeout: 60000 });

  app.decorateRequest("authSid", "");
  app.decorateRequest("authToken", "");

  // Middleware/hook: extract Cookie `sid` and `Authorization: Bearer <token>`
  app.addHook("onRequest", (req, _reply, done) => {
    const cookieHeader = req.headers.cookie;
    const authHeader = req.headers.authorization;
    req.authSid = cookieHeader ? extractCookie(cookieHeader, "sid") : "";
    req.authToken = authHeader ? extractBearer(authHeader) : "";
    done();
  });

  // --- 12 Static Routes ---
  app.get("/", async () => ({ service: "cap-web", version: "1.0" }));
  app.get("/health", async () => ({ ok: true, service: "cap-web" }));
  app.get("/ready", async () => ({ ready: true }));
  app.get("/version", async () => ({ version: "1.0.0" }));
  app.get("/metrics", async () => ({ uptime: 1, requests: 0 }));
  app.get("/api/v1/me", async (req) => ({ sid: req.authSid, token: req.authToken }));
  app.get("/api/v1/status", async () => ({ status: "operational" }));
  app.get("/api/v1/config", async () => ({ env: "prod", region: "us-east-1" }));
  app.get("/api/v1/users", async () => ({ items: [], total: 0 }));
  app.get("/api/v1/products", async () => ({ items: [], total: 0 }));
  app.get("/api/v1/orders", async () => ({ items: [], total: 0 }));
  app.get("/api/v1/catalog/categories", async () => ({ categories: ["gpu", "cpu", "net"] }));

  // --- 8 Dynamic :param Routes ---
  app.get("/api/v1/users/:id", async (req) => ({ userId: req.params.id }));
  app.get("/api/v1/users/:id/profile", async (req) => ({ userId: req.params.id, role: "member" }));
  app.get("/api/v1/users/:id/posts/:postId", async (req) => ({
    userId: req.params.id,
    postId: req.params.postId,
    lang: req.query.lang ?? "",
    limit: req.query.limit ?? "",
  }));
  app.get("/api/v1/products/:sku", async (req) => ({ sku: req.params.sku }));
  app.get("/api/v1/products/:sku/reviews/:reviewId", async (req) => ({
    sku: req.params.sku,
    reviewId: req.params.reviewId,
  }));
  app.get("/api/v1/orders/:orderId", async (req) => ({ orderId: req.params.orderId }));
  app.get("/api/v1/teams/:teamId/members/:memberId", async (req) => ({
    teamId: req.params.teamId,
    memberId: req.params.memberId,
  }));
  app.get("/api/v1/orgs/:orgId/projects/:projId", async (req) => ({
    orgId: req.params.orgId,
    projId: req.params.projId,
  }));

  // --- 2 Wildcard Routes ---
  app.get("/static/*", async (req) => ({ asset: req.params["*"] }));
  app.get("/files/*", async (req) => ({ file: req.params["*"] }));

  // --- 2 POST JSON Routes ---
  app.post("/api/v1/orders", async (req, reply) => {
    const b = req.body;
    if (
      !b ||
      typeof b.item !== "string" ||
      b.item.length === 0 ||
      typeof b.qty !== "number" ||
      b.qty <= 0 ||
      typeof b.price !== "number" ||
      b.price <= 0
    ) {
      reply.code(400);
      return { accepted: false, error: "invalid_order" };
    }
    return { accepted: true, item: b.item, qty: b.qty };
  });

  app.post("/api/v1/events", async (req, reply) => {
    const b = req.body;
    if (!b || typeof b.type !== "string" || b.type.length === 0) {
      reply.code(400);
      return { recorded: false, error: "invalid_event" };
    }
    return { recorded: true };
  });

  app.listen({ port, host: "127.0.0.1" }, (err) => {
    if (err) {
      console.error(err);
      process.exit(1);
    }
    if (workers === 1) {
      console.log(`[node-fastify] listening on http://127.0.0.1:${port} (workers=1)`);
    }
  });
}
