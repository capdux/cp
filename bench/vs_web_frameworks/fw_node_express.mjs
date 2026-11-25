// Real Web Framework Benchmark: Node.js + Express (24 routes, 1c / 8c / 16c node:cluster)
// Usage: node fw_node_express.mjs --port 18332 --workers 1

import cluster from "node:cluster";
import { createRequire } from "node:module";

const require = createRequire("/data/dev/cp/temp/fw_node_deps/package.json");
const express = require("express");

function parseArgs() {
  let port = 18332;
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
  console.log(`[node-express] cluster primary listening on http://127.0.0.1:${port} (workers=${workers})`);
} else {
  const app = express();
  app.disable("x-powered-by");
  app.disable("etag");

  // Global middleware: extract Cookie `sid` and `Authorization: Bearer <token>`
  app.use((req, _res, next) => {
    const cookieHeader = req.headers.cookie;
    const authHeader = req.headers.authorization;
    req.authSid = cookieHeader ? extractCookie(cookieHeader, "sid") : "";
    req.authToken = authHeader ? extractBearer(authHeader) : "";
    next();
  });

  app.use(express.json());

  // --- 12 Static Routes ---
  app.get("/", (_req, res) => res.json({ service: "cap-web", version: "1.0" }));
  app.get("/health", (_req, res) => res.json({ ok: true, service: "cap-web" }));
  app.get("/ready", (_req, res) => res.json({ ready: true }));
  app.get("/version", (_req, res) => res.json({ version: "1.0.0" }));
  app.get("/metrics", (_req, res) => res.json({ uptime: 1, requests: 0 }));
  app.get("/api/v1/me", (req, res) => res.json({ sid: req.authSid, token: req.authToken }));
  app.get("/api/v1/status", (_req, res) => res.json({ status: "operational" }));
  app.get("/api/v1/config", (_req, res) => res.json({ env: "prod", region: "us-east-1" }));
  app.get("/api/v1/users", (_req, res) => res.json({ items: [], total: 0 }));
  app.get("/api/v1/products", (_req, res) => res.json({ items: [], total: 0 }));
  app.get("/api/v1/orders", (_req, res) => res.json({ items: [], total: 0 }));
  app.get("/api/v1/catalog/categories", (_req, res) => res.json({ categories: ["gpu", "cpu", "net"] }));

  // --- 8 Dynamic :param Routes ---
  app.get("/api/v1/users/:id", (req, res) => res.json({ userId: req.params.id }));
  app.get("/api/v1/users/:id/profile", (req, res) => res.json({ userId: req.params.id, role: "member" }));
  app.get("/api/v1/users/:id/posts/:postId", (req, res) =>
    res.json({
      userId: req.params.id,
      postId: req.params.postId,
      lang: typeof req.query.lang === "string" ? req.query.lang : "",
      limit: typeof req.query.limit === "string" ? req.query.limit : "",
    })
  );
  app.get("/api/v1/products/:sku", (req, res) => res.json({ sku: req.params.sku }));
  app.get("/api/v1/products/:sku/reviews/:reviewId", (req, res) =>
    res.json({
      sku: req.params.sku,
      reviewId: req.params.reviewId,
    })
  );
  app.get("/api/v1/orders/:orderId", (req, res) => res.json({ orderId: req.params.orderId }));
  app.get("/api/v1/teams/:teamId/members/:memberId", (req, res) =>
    res.json({
      teamId: req.params.teamId,
      memberId: req.params.memberId,
    })
  );
  app.get("/api/v1/orgs/:orgId/projects/:projId", (req, res) =>
    res.json({
      orgId: req.params.orgId,
      projId: req.params.projId,
    })
  );

  // --- 2 Wildcard Routes (Express 5 path-to-regexp v8 syntax) ---
  app.get("/static/*splat", (req, res) => {
    const s = req.params.splat;
    res.json({ asset: Array.isArray(s) ? s.join("/") : (s ?? "") });
  });
  app.get("/files/*splat", (req, res) => {
    const s = req.params.splat;
    res.json({ file: Array.isArray(s) ? s.join("/") : (s ?? "") });
  });

  // --- 2 POST JSON Routes ---
  app.post("/api/v1/orders", (req, res) => {
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
      res.status(400).json({ accepted: false, error: "invalid_order" });
      return;
    }
    res.json({ accepted: true, item: b.item, qty: b.qty });
  });

  app.post("/api/v1/events", (req, res) => {
    const b = req.body;
    if (!b || typeof b.type !== "string" || b.type.length === 0) {
      res.status(400).json({ recorded: false, error: "invalid_event" });
      return;
    }
    res.json({ recorded: true });
  });

  const server = app.listen(port, "127.0.0.1", () => {
    if (workers === 1) {
      console.log(`[node-express] listening on http://127.0.0.1:${port} (workers=1)`);
    }
  });
  server.keepAliveTimeout = 60000;
}
