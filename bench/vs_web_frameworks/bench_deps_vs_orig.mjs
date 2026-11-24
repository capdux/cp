// Same-machine JS baseline benchmark (Bun & Node) matching CP bench_web.cp & bench_schema.cp payloads 1:1
// Covers 4 core dependency equivalents:
//   1. fast-decode-uri-component (pure ASCII fast-path 5M + UTF-8 percent-decoding 2M)
//   2. memoirist-style Radix/Map router (static 5M + dynamic :uid/:pid 2M)
//   3. cookie Set-Cookie serializer (RFC 6265, 2M)
//   4. Schema validation + exact-mirror object stripping (1M each)

const hrNowNs = () => process.hrtime.bigint();

// ============================================================================
// 1. fast-decode-uri-component equivalent
// ============================================================================
function fastDecodeUriComponent(str) {
  // Fast-path check for '%' just like fast-decode-uri-component / SIMD path
  if (str.indexOf("%") === -1) return str;
  try {
    return decodeURIComponent(str);
  } catch {
    return null;
  }
}

// ============================================================================
// 2. memoirist-style RadixRouter (Static Map + Parametric Radix Tree)
// ============================================================================
class RadixNode {
  constructor(part = "") {
    this.part = part;
    this.children = new Map();
    this.paramChild = null;
    this.paramName = "";
    this.wildcardChild = null;
    this.wildcardName = "";
    this.handler = null;
  }
}

class MemoiristRouter {
  constructor() {
    this.staticRoutes = new Map();
    this.root = new RadixNode("");
  }

  add(method, path, handler) {
    if (path.indexOf(":") === -1 && path.indexOf("*") === -1 && path.indexOf("{") === -1) {
      this.staticRoutes.set(method + ":" + path, handler);
      return;
    }
    const segs = path.split("/").filter(Boolean);
    let node = this.root;
    for (let i = 0; i < segs.length; i++) {
      const seg = segs[i];
      if (seg.startsWith(":")) {
        if (!node.paramChild) {
          node.paramChild = new RadixNode(":");
          node.paramChild.paramName = seg.slice(1);
        }
        node = node.paramChild;
      } else if (seg.startsWith("{") && seg.endsWith("}")) {
        if (!node.paramChild) {
          node.paramChild = new RadixNode(":");
          node.paramChild.paramName = seg.slice(1, -1);
        }
        node = node.paramChild;
      } else if (seg.startsWith("*")) {
        if (!node.wildcardChild) {
          node.wildcardChild = new RadixNode("*");
          node.wildcardChild.wildcardName = seg.slice(1);
        }
        node = node.wildcardChild;
        break;
      } else {
        let child = node.children.get(seg);
        if (!child) {
          child = new RadixNode(seg);
          node.children.set(seg, child);
        }
        node = child;
      }
    }
    node.handler = handler;
  }

  lookup(method, path) {
    const st = this.staticRoutes.get(method + ":" + path);
    if (st !== undefined) {
      return { matched: true, handler: st, params: null, paramCount: 0 };
    }
    let node = this.root;
    let params = null;
    let paramCount = 0;
    let start = path.charCodeAt(0) === 47 ? 1 : 0;
    const len = path.length;
    while (start < len) {
      let end = path.indexOf("/", start);
      if (end === -1) end = len;
      const seg = path.slice(start, end);
      let next = node.children.get(seg);
      if (next !== undefined) {
        node = next;
      } else if (node.paramChild !== null) {
        node = node.paramChild;
        if (params === null) params = {};
        params[node.paramName] = seg;
        paramCount++;
      } else if (node.wildcardChild !== null) {
        node = node.wildcardChild;
        if (params === null) params = {};
        params[node.wildcardName] = path.slice(start);
        paramCount++;
        break;
      } else {
        return { matched: false, handler: null, params: null, paramCount: 0 };
      }
      start = end + 1;
    }
    if (node.handler !== null) {
      return { matched: true, handler: node.handler, params, paramCount };
    }
    return { matched: false, handler: null, params: null, paramCount: 0 };
  }
}

// ============================================================================
// 3. cookie serialize (RFC 6265 Set-Cookie)
// ============================================================================
function serializeCookie(name, value, opts) {
  let str = name + "=" + value;
  if (opts.maxAge !== undefined) str += "; Max-Age=" + Math.floor(opts.maxAge);
  if (opts.domain) str += "; Domain=" + opts.domain;
  if (opts.path) str += "; Path=" + opts.path;
  if (opts.httpOnly) str += "; HttpOnly";
  if (opts.secure) str += "; Secure";
  if (opts.sameSite) str += "; SameSite=" + opts.sameSite;
  return str;
}

// ============================================================================
// 4. Schema validator + exact-mirror object stripping
// ============================================================================
const EMAIL_RE = /^[^\s@]+@[^\s@]+\.[^\s@]+$/;
const VALID_ROLES = new Set(["user", "admin", "moderator"]);

function validateSignupJson(raw) {
  let obj;
  try {
    obj = JSON.parse(raw);
  } catch {
    return false;
  }
  if (!obj || typeof obj !== "object") return false;
  if (typeof obj.username !== "string" || obj.username.length < 3 || obj.username.length > 32) return false;
  if (typeof obj.email !== "string" || !EMAIL_RE.test(obj.email)) return false;
  if (typeof obj.age !== "number" || !Number.isInteger(obj.age) || obj.age < 18 || obj.age > 120) return false;
  if (typeof obj.role !== "string" || !VALID_ROLES.has(obj.role)) return false;
  if (typeof obj.accepted_tos !== "boolean") return false;
  return true;
}

function exactMirrorSignupJson(raw) {
  const obj = JSON.parse(raw);
  const clean = {
    username: obj.username,
    email: obj.email,
    age: obj.age,
    role: obj.role,
    accepted_tos: obj.accepted_tos,
  };
  return JSON.stringify(clean);
}

// ============================================================================
// Benchmark Runner
// ============================================================================
function bench(name, iterations, fn) {
  // Warmup (5% or capped at 50k)
  const warmup = Math.min(50000, Math.max(1000, Math.floor(iterations / 20)));
  for (let i = 0; i < warmup; i++) fn(i);

  const t0 = hrNowNs();
  let acc = 0;
  for (let i = 0; i < iterations; i++) {
    acc += fn(i);
  }
  const dtNs = Number(hrNowNs() - t0);
  const secs = dtNs / 1e9;
  const opsPerSec = iterations / (secs > 0 ? secs : 1e-9);
  const nsPerOp = dtNs / iterations;
  return { name, iterations, dt_ns: dtNs, ns_per_op: nsPerOp, ops_per_sec: opsPerSec, acc };
}

function main() {
  // Payloads matching bench_schema.cp 1:1
  const asciiInputs = [
    "api_v1_users_profile_settings_notifications_preferences_01234567",
    "cdn_assets_static_bundles_main_chunk_9f8e7d6c5b4a3210_abcdef1234",
    "search_query_parameters_sort_by_created_at_order_desc_page_00042",
    "workspace_projects_deployments_production_us_east_1_cluster_0099",
  ];
  const utf8Inputs = [
    "q=%E4%B8%AD%E6%96%87%20search%2Fitem%3Fid%3D1001%26tag%3Dfast_cp",
    "p=%E4%B8%AD%E6%96%87%2D%E6%B5%8B%E8%AF%95%2Fpath%2Fto%2Ffile%21",
    "name=%E4%B8%AD%E6%96%87%20user%40example.com%26role%3Dadmin_001",
    "msg=hello%20%E4%B8%AD%E6%96%87%20world%21%20status%3Dok%26v%3D2",
  ];
  const signupJsons = [
    '{"username":"alice_dev","email":"alice@example.com","age":28,"role":"user","accepted_tos":true}',
    '{"username":"bob_ops","email":"bob@cp-lang.org","age":35,"role":"admin","accepted_tos":true}',
    '{"username":"carol_qa","email":"carol@web.io","age":24,"role":"moderator","accepted_tos":false}',
    '{"username":"dave_sec","email":"dave@cloud.dev","age":41,"role":"user","accepted_tos":true}',
  ];
  const dirtyJsons = [
    '{"username":"alice_dev","password_hash":"$2b$12$secret1","email":"alice@example.com","internal_role":"superadmin","age":28,"role":"user","accepted_tos":true,"api_secret":"sk_live_001"}',
    '{"username":"bob_ops","password_hash":"$2b$12$secret2","email":"bob@cp-lang.org","internal_role":"root","age":35,"role":"admin","accepted_tos":true,"api_secret":"sk_live_002"}',
    '{"username":"carol_qa","password_hash":"$2b$12$secret3","email":"carol@web.io","internal_role":"staff","age":24,"role":"moderator","accepted_tos":false,"api_secret":"sk_live_003"}',
    '{"username":"dave_sec","password_hash":"$2b$12$secret4","email":"dave@cloud.dev","internal_role":"ops","age":41,"role":"user","accepted_tos":true,"api_secret":"sk_live_004"}',
  ];

  // Router setup matching bench_web.cp 1:1
  const router = new MemoiristRouter();
  const hOk = () => 200;
  router.add("GET", "/health", hOk);
  router.add("GET", "/api/v1/status", hOk);
  router.add("GET", "/api/v1/users", hOk);
  router.add("POST", "/api/v1/users", hOk);
  router.add("GET", "/api/v1/users/:uid", hOk);
  router.add("GET", "/api/v1/users/:uid/posts/:pid", hOk);
  router.add("GET", "/api/v1/orgs/{org}/repos/{repo}", hOk);
  router.add("GET", "/static/*filepath", hOk);

  const cookieTokens = [
    "tok_0123456789abcdef",
    "tok_fedcba9876543210",
    "tok_a1b2c3d4e5f60718",
    "tok_9988776655443322",
  ];
  const cookieOpts = {
    path: "/",
    domain: "cp.dev",
    maxAge: 86400,
    httpOnly: true,
    secure: true,
    sameSite: "Lax",
  };

  const results = {};
  const runs = [
    bench("uri_ascii_64B", 5_000_000, (i) => {
      const s = fastDecodeUriComponent(asciiInputs[i & 3]);
      return s.length + s.charCodeAt(i & 31);
    }),
    bench("uri_utf8_63B", 2_000_000, (i) => {
      const s = fastDecodeUriComponent(utf8Inputs[i & 3]);
      return s.length + s.charCodeAt(0);
    }),
    bench("router_static", 5_000_000, () => {
      const m = router.lookup("GET", "/api/v1/status");
      return m.matched ? 1 : 0;
    }),
    bench("router_dynamic_2params", 2_000_000, () => {
      const m = router.lookup("GET", "/api/v1/users/12345/posts/99");
      return m.matched ? m.paramCount : 0;
    }),
    bench("cookie_set_header", 2_000_000, (i) => {
      const s = serializeCookie("session_id", cookieTokens[i & 3], cookieOpts);
      return s.length + s.charCodeAt(15);
    }),
    bench("schema_validate_5f", 1_000_000, (i) => {
      return validateSignupJson(signupJsons[i & 3]) ? 1 : 0;
    }),
    bench("exact_mirror_strip", 1_000_000, (i) => {
      const s = exactMirrorSignupJson(dirtyJsons[i & 3]);
      return s.length + s.charCodeAt(s.length - 1);
    }),
  ];

  for (const r of runs) {
    results[r.name] = r;
  }

  const runtime = typeof Bun !== "undefined" ? `bun ${Bun.version}` : `node ${process.version}`;
  console.log(JSON.stringify({ runtime, results }, null, 2));
}

main();
