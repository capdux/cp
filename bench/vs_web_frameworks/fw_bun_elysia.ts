// Real Web Framework Benchmark: Bun + Elysia (24 routes, 1c / 8c / 16c reusePort)
// Usage: bun fw_bun_elysia.ts --port 18321 --workers 1

import { Elysia } from "/data/dev/cp/_ref/elysia/src/index.ts";

function parseArgs() {
  let port = 18321;
  let workers = 1;
  let isWorker = false;
  const argv = process.argv.slice(2);
  for (let i = 0; i < argv.length; i++) {
    if (argv[i] === "--port" && argv[i + 1]) port = parseInt(argv[++i], 10);
    else if (argv[i] === "--workers" && argv[i + 1]) workers = parseInt(argv[++i], 10);
    else if (argv[i] === "--worker") isWorker = true;
  }
  return { port, workers, isWorker };
}

function extractCookie(cookieHeader: string | null, name: string): string {
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

function extractBearer(authHeader: string | null): string {
  if (!authHeader) return "";
  if (authHeader.length > 7 && authHeader.slice(0, 7).toLowerCase() === "bearer ") {
    return authHeader.slice(7).trim();
  }
  return "";
}

const { port, workers, isWorker } = parseArgs();

if (workers > 1 && !isWorker) {
  const procs: ReturnType<typeof Bun.spawn>[] = [];
  for (let i = 0; i < workers; i++) {
    procs.push(
      Bun.spawn([process.execPath, import.meta.path, "--port", String(port), "--workers", "1", "--worker"], {
        stdout: "inherit",
        stderr: "inherit",
      })
    );
  }
  const cleanup = () => {
    for (const p of procs) {
      try { p.kill(); } catch {}
    }
    process.exit(0);
  };
  process.on("SIGINT", cleanup);
  process.on("SIGTERM", cleanup);
  await Promise.all(procs.map((p) => p.exited));
} else {
  const app = new Elysia()
    // Middleware/derive hook: extract Cookie `sid` and `Authorization: Bearer <token>`
    .derive(({ request }) => {
      const authHeader = request.headers.get("authorization");
      const cookieHeader = request.headers.get("cookie");
      return {
        sid: extractCookie(cookieHeader, "sid"),
        token: extractBearer(authHeader),
      };
    })
    // --- 12 Static Routes ---
    .get("/", () => ({ service: "cap-web", version: "1.0" }))
    .get("/health", () => ({ ok: true, service: "cap-web" }))
    .get("/ready", () => ({ ready: true }))
    .get("/version", () => ({ version: "1.0.0" }))
    .get("/metrics", () => ({ uptime: 1, requests: 0 }))
    .get("/api/v1/me", ({ sid, token }) => ({ sid, token }))
    .get("/api/v1/status", () => ({ status: "operational" }))
    .get("/api/v1/config", () => ({ env: "prod", region: "us-east-1" }))
    .get("/api/v1/users", () => ({ items: [], total: 0 }))
    .get("/api/v1/products", () => ({ items: [], total: 0 }))
    .get("/api/v1/orders", () => ({ items: [], total: 0 }))
    .get("/api/v1/catalog/categories", () => ({ categories: ["gpu", "cpu", "net"] }))

    // --- 8 Dynamic :param Routes ---
    .get("/api/v1/users/:id", ({ params }) => ({ userId: params.id }))
    .get("/api/v1/users/:id/profile", ({ params }) => ({ userId: params.id, role: "member" }))
    .get("/api/v1/users/:id/posts/:postId", ({ params, query }) => ({
      userId: params.id,
      postId: params.postId,
      lang: (query.lang as string) ?? "",
      limit: (query.limit as string) ?? "",
    }))
    .get("/api/v1/products/:sku", ({ params }) => ({ sku: params.sku }))
    .get("/api/v1/products/:sku/reviews/:reviewId", ({ params }) => ({
      sku: params.sku,
      reviewId: params.reviewId,
    }))
    .get("/api/v1/orders/:orderId", ({ params }) => ({ orderId: params.orderId }))
    .get("/api/v1/teams/:teamId/members/:memberId", ({ params }) => ({
      teamId: params.teamId,
      memberId: params.memberId,
    }))
    .get("/api/v1/orgs/:orgId/projects/:projId", ({ params }) => ({
      orgId: params.orgId,
      projId: params.projId,
    }))

    // --- 2 Wildcard Routes ---
    .get("/static/*", ({ params }) => ({ asset: params["*"] }))
    .get("/files/*", ({ params }) => ({ file: params["*"] }))

    // --- 2 POST JSON Routes ---
    .post("/api/v1/orders", ({ body, set }) => {
      const b = body as { item?: unknown; qty?: unknown; price?: unknown } | null;
      if (
        !b ||
        typeof b.item !== "string" ||
        b.item.length === 0 ||
        typeof b.qty !== "number" ||
        b.qty <= 0 ||
        typeof b.price !== "number" ||
        b.price <= 0
      ) {
        set.status = 400;
        return { accepted: false, error: "invalid_order" };
      }
      return { accepted: true, item: b.item, qty: b.qty };
    })
    .post("/api/v1/events", ({ body, set }) => {
      const b = body as { type?: unknown; source?: unknown } | null;
      if (!b || typeof b.type !== "string" || b.type.length === 0) {
        set.status = 400;
        return { recorded: false, error: "invalid_event" };
      }
      return { recorded: true };
    });

  const server = Bun.serve({
    port,
    hostname: "127.0.0.1",
    reusePort: true,
    fetch: app.fetch,
  });

  if (!isWorker) {
    console.log(`[bun-elysia] listening on http://127.0.0.1:${server.port} (workers=${workers})`);
  }
}
