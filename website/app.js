// CP v1.0.0 Website — Interactive Install Tabs, Live Benchmark Renderer & 43+ Package Explorer

const INSTALL_CMDS = {
  unix: "curl -fsSL https://raw.githubusercontent.com/cp-lang/cp/main/install.sh | sh",
  win: "irm https://raw.githubusercontent.com/cp-lang/cp/main/install.ps1 | iex",
};

const CODE_SAMPLES = {
  rest: `// Elysia-style fluent API + Axum-style zero-reflection extractors
import { App, Context, Response } from "cap:web";

fn getUser(ctx: *Context): Response {
    const id = ctx.param("id") orelse "0";
    return ctx.json(200, "{\\"id\\":\\"" ++ id ++ "\\",\\"status\\":\\"ok\\"}");
}

fn main() {
    let app = App.init();
    app.get("/api/users/:id", getUser);
    // Export OpenAPI 3.1 spec with zero runtime reflection
}`,
  sqlite: `// Built-in embedded SQLite 3.54.1 (zero install, 2.64M point-lookups/s)
import { Database } from "cap:sqlite";
import std from "std";

fn main() {
    let db = Database.openMemory() catch return;
    db.exec("CREATE TABLE agents (id INTEGER PRIMARY KEY, name TEXT, score INT);") catch return;
    db.exec("INSERT INTO agents (name, score) VALUES ('alpha', 98), ('gamma', 100);") catch return;

    let stmt = db.prepare("SELECT name, score FROM agents WHERE score >= ?;") catch return;
    stmt.bindInt(1, 95) catch return;
    while (stmt.step() catch false) {
        std.debug.print("{s}: {d}\\n", .{ stmt.columnText(0), stmt.columnInt(1) });
    }
}`,
  raii: `// Deterministic Zero-GC Stack RAII (~init runs automatically on scope exit)
import std from "std";

struct FileGuard {
    label: string,
    ~init() {
        std.debug.print("[~init] Released {s} deterministically\\n", .{_.label});
    }
}

fn main() {
    const g = FileGuard{ .label = "gpu-buffer" };
    * = g; // Safe silence; discarding an unowned Guard via '* = FileGuard{...}' is rejected (E0220)!
}`,
  ai: `// Local LLM (@cap/llama) + Billion-scale HNSW Vector Search (@cap/usearch)
import { LlamaBackend } from "@cap/llama";
import { VectorIndex } from "@cap/usearch";

fn main() {
    const llm = LlamaBackend.init() catch return;
    let idx = VectorIndex.initCosine(16) catch return;
    idx.reserve(1024) catch return;
    // Zero-GC inference + vector memory retrieval in one native binary
}`,
  native: `// Native C packages with prebuilt -g0 lean archives & AST link tree-shaking
import { compressInto, decompressInto } from "@cap/lz4";
import { JsonDoc } from "@cap/yyjson";
import std from "std";

fn main() {
    const doc = JsonDoc.parse("{\\"lang\\":\\"CP\\",\\"packages\\":43}") catch return;
    const lang: string = doc.root().get("lang").?.asStr().?;
    std.debug.print("Parsed via yyjson: {s}\\n", .{lang});
}`,
};

// Real verified same-machine benchmarks (Intel Xeon Platinum 8175M, Linux 6.12 x86_64)
// Sources: bench/vs_web_frameworks/http_results.json, bun_official_results.json, bench/vs_runtimes/results.json
// CP v1.0.0 (-OReleaseFast) vs Bun 1.3.14 vs Node v26.1.0 vs Python 3.12.15
const DEFAULT_BENCHMARKS = [
  {
    cat: "net",
    title: "HTTP/1.1 Multi-Core (GET /, Err=0 Keep-Alive)",
    subtitle: "Higher is better (QPS) · CP 16c hits 1.06M QPS in 33.9 MB · CP 8c 1.46x Bun / 22x less RSS",
    higherIsBetter: true,
    rows: [
      { label: "CP v1.0 16c single-proc (P99 0.10ms · 33.9 MB RSS)", key: "cp", value: 1064581, display: "1,064,581 QPS" },
      { label: "CP v1.0 8c single-proc (P99 0.13ms · 19.2 MB RSS)", key: "cp", value: 749811, display: "749,811 QPS" },
      { label: "Bun 1.3.14 8c (8-proc · 409.6 MB RSS)", key: "bun", value: 512632, display: "512,632 QPS" },
      { label: "Node v26.1.0 8c (8-proc · 697.3 MB RSS)", key: "node", value: 328329, display: "328,329 QPS" },
    ],
  },
  {
    cat: "net",
    title: "HTTP/1.1 8-Core (POST /json, Err=0 Keep-Alive)",
    subtitle: "Higher is better (QPS) · 2.11x Bun 8c, 2.95x Node 8c · 22.0x–38.3x lower RSS",
    higherIsBetter: true,
    rows: [
      { label: "CP v1.0 8c (P99 0.09ms · 19.2 MB RSS)", key: "cp", value: 709061, display: "709,061 QPS" },
      { label: "Bun 1.3.14 8c (422.3 MB RSS)", key: "bun", value: 336002, display: "336,002 QPS" },
      { label: "Node v26.1.0 8c (734.4 MB RSS)", key: "node", value: 240005, display: "240,005 QPS" },
    ],
  },
  {
    cat: "net",
    title: "Live TCP WebSocket 8-Core (128B Masked Echo, Err=0)",
    subtitle: "Higher is better (msg/s) · Lower P99 (0.148ms vs 0.160ms) & 23.2x lower RSS than Bun 8c",
    higherIsBetter: true,
    rows: [
      { label: "CP v1.0 8c 32B-SIMD (P99 0.148ms · 17.2 MB RSS)", key: "cp", value: 619221, display: "619,221 msg/s" },
      { label: "Bun 1.3.14 8c (P99 0.160ms · 398.6 MB RSS)", key: "bun", value: 623524, display: "623,524 msg/s" },
      { label: "Node v26.1.0 1c (P99 1.114ms · 77.7 MB RSS)", key: "node", value: 62564, display: "62,564 msg/s" },
    ],
  },
  {
    cat: "db",
    title: "SQLite 8-Core Concurrent WAL Reads (1.6M queries)",
    subtitle: "Higher is better (QPS) · 1.92x Bun 8c, 2.02x Node 8c · 22.8x lower RSS (33.4 vs 760.5 MB)",
    higherIsBetter: true,
    rows: [
      { label: "CP v1.0 8-thread single-proc (33.4 MB RSS)", key: "cp", value: 777631, display: "777,631 QPS" },
      { label: "Bun 1.3.14 8-proc (760.5 MB RSS)", key: "bun", value: 405078, display: "405,078 QPS" },
      { label: "Node v26.1.0 8-proc (546.9 MB RSS)", key: "node", value: 385874, display: "385,874 QPS" },
    ],
  },
  {
    cat: "net",
    title: "WebSocket 128B Frame + 16-Subscriber Broadcast",
    subtitle: "Higher is better (msg/s) · 6.53x Bun, 12.49x Node · 25.3x lower RSS",
    higherIsBetter: true,
    rows: [
      { label: "CP v1.0 (2.48 MB RSS)", key: "cp", value: 8712547, display: "8.71M msg/s" },
      { label: "Bun 1.3.14 (62.65 MB RSS)", key: "bun", value: 1334034, display: "1.33M msg/s" },
      { label: "Node v26.1.0 (56.74 MB RSS)", key: "node", value: 697360, display: "0.70M msg/s" },
    ],
  },
  {
    cat: "data",
    title: "SQLite Northwind Indexed JOIN Point Lookup",
    subtitle: "Higher is better (ops/s) · 1.88x Bun, 3.03x Node · 8.0x lower RSS",
    higherIsBetter: true,
    rows: [
      { label: "CP v1.0 (cap:sqlite · 10.8 MB RSS)", key: "cp", value: 1023123, display: "1.02M ops/s" },
      { label: "Bun 1.3.14 (bun:sqlite · 86.4 MB RSS)", key: "bun", value: 544129, display: "544.1K ops/s" },
      { label: "Node v26.1.0 (node:sqlite · 68.9 MB RSS)", key: "node", value: 337523, display: "337.5K ops/s" },
    ],
  },
  {
    cat: "core",
    title: "Cold Startup Latency (20-run median)",
    subtitle: "Lower is better (ms) · CP 1.61ms (9.38x faster than Bun, 20.01x vs Node)",
    higherIsBetter: false,
    rows: [
      { label: "CP v1.0 (3.78 MB native bin)", key: "cp", value: 1.61, display: "1.61 ms" },
      { label: "Python 3.12.15", key: "py", value: 12.40, display: "12.40 ms" },
      { label: "Bun 1.3.14 (88.5 MB)", key: "bun", value: 15.07, display: "15.07 ms" },
      { label: "Node v26.1.0 (140.6 MB)", key: "node", value: 32.14, display: "32.14 ms" },
    ],
  },
  {
    cat: "data",
    title: "JSON Parsing Throughput (10,000 × 1.75 KB)",
    subtitle: "Higher is better (MB/s) · CP @cap/yyjson 4.93x faster than Bun",
    higherIsBetter: true,
    rows: [
      { label: "CP v1.0 (@cap/yyjson)", key: "cp", value: 755.8, display: "755.8 MB/s (22.07 ms)" },
      { label: "Node v26.1.0 (JSON.parse)", key: "node", value: 205.0, display: "205.0 MB/s (81.37 ms)" },
      { label: "Bun 1.3.14 (JSON.parse)", key: "bun", value: 153.4, display: "153.4 MB/s (108.74 ms)" },
      { label: "Python 3.12 (json.loads)", key: "py", value: 72.4, display: "72.4 MB/s (230.44 ms)" },
    ],
  },
  {
    cat: "data",
    title: "SQLite :memory: 100K Insert + 100K Point Lookup",
    subtitle: "Higher is better (ops/sec) · CP cap:sqlite 1.64x vs Bun, 2.10x vs Node",
    higherIsBetter: true,
    rows: [
      { label: "CP v1.0 (cap:sqlite 3.54.1)", key: "cp", value: 745400, display: "745.4K ops/s (268.3 ms)" },
      { label: "Bun 1.3.14 (bun:sqlite)", key: "bun", value: 455700, display: "455.7K ops/s (438.9 ms)" },
      { label: "Node v26.1.0 (node:sqlite)", key: "node", value: 355600, display: "355.6K ops/s (562.5 ms)" },
      { label: "Python 3.12 (sqlite3)", key: "py", value: 187500, display: "187.5K ops/s (1066.9 ms)" },
    ],
  },
  {
    cat: "core",
    title: "Zero-GC Compute (fib(38) + 10M u32 Reduction)",
    subtitle: "Lower is better (ms) · CP 1.98x vs Bun, 4.19x vs Node, 25.88x vs Python",
    higherIsBetter: false,
    rows: [
      { label: "CP v1.0 (-OReleaseFast)", key: "cp", value: 401.48, display: "401.5 ms" },
      { label: "Bun 1.3.14", key: "bun", value: 795.17, display: "795.2 ms" },
      { label: "Node v26.1.0", key: "node", value: 1680.87, display: "1,680.9 ms" },
      { label: "Python 3.12.15", key: "py", value: 10391.92, display: "10,391.9 ms" },
    ],
  },
];

const PACKAGES = [
  { name: "@cap/llama", lic: "MIT", cat: "ai", desc: "Local LLM inference (llama.cpp AVX2/AVX512/GPU)" },
  { name: "@cap/whisper", lic: "MIT", cat: "ai", desc: "Local speech-to-text ASR (whisper.cpp, 2.6 MB)" },
  { name: "@cap/onnx", lic: "MIT", cat: "ai", desc: "Microsoft ONNX Runtime neural/embedding engine" },
  { name: "@cap/usearch", lic: "Apache-2.0", cat: "ai", desc: "Billion-scale SIMD HNSW vector search" },
  { name: "@cap/openblas", lic: "BSD-3", cat: "ai", desc: "High-performance BLAS / GEMM linear algebra" },
  { name: "@cap/duckdb", lic: "MIT", cat: "ai", desc: "Embedded OLAP SQL engine (Parquet/CSV/JSON)" },
  { name: "@cap/lmdb", lic: "OLDAP-2.8", cat: "ai", desc: "Zero-copy mmap embedded KV store (124 KB)" },
  { name: "@cap/opus", lic: "BSD-3", cat: "media", desc: "Low-latency real-time voice/audio codec" },
  { name: "@cap/miniaudio", lic: "MIT-0", cat: "media", desc: "Cross-platform audio capture/playback/resample" },
  { name: "@cap/rnnoise", lic: "BSD-3", cat: "media", desc: "Real-time neural voice denoiser & VAD (162 KB)" },
  { name: "@cap/speexdsp", lic: "BSD-3", cat: "media", desc: "Acoustic Echo Cancellation (AEC) & AGC" },
  { name: "@cap/dr_libs", lic: "MIT-0", cat: "media", desc: "Zero-dependency WAV / MP3 / FLAC codecs" },
  { name: "@cap/flac", lic: "BSD-3", cat: "media", desc: "Xiph lossless audio encoder & decoder" },
  { name: "@cap/vorbis", lic: "BSD-3", cat: "media", desc: "Xiph Ogg/Vorbis audio encoder & decoder" },
  { name: "@cap/ogg", lic: "BSD-3", cat: "media", desc: "Xiph Ogg bitstream container (36 KB)" },
  { name: "@cap/libyuv", lic: "BSD-3", cat: "media", desc: "Google SIMD YUV↔RGB conversion & scaling" },
  { name: "@cap/dav1d", lic: "BSD-2", cat: "media", desc: "VideoLAN fastest royalty-free AV1 decoder (1.8 MB)" },
  { name: "@cap/libvpx", lic: "BSD-3", cat: "media", desc: "Google royalty-free VP8/VP9 video codec (1.7 MB)" },
  { name: "@cap/libavif", lic: "BSD-2", cat: "media", desc: "Next-gen AVIF image container & parser" },
  { name: "@cap/libsrtp", lic: "BSD-3", cat: "media", desc: "Cisco SRTP/SRTCP encrypted media transport" },
  { name: "@cap/usrsctp", lic: "BSD-3", cat: "media", desc: "WebRTC DataChannel userspace SCTP stack" },
  { name: "@cap/opencv", lic: "Apache-2.0", cat: "graphics", desc: "Computer vision core + imgproc (10.4 MB)" },
  { name: "@cap/pdfium", lic: "Apache-2.0", cat: "graphics", desc: "Chrome PDFium rendering & text extraction (6.8 MB)" },
  { name: "@cap/sdl3", lic: "Zlib", cat: "graphics", desc: "Cross-platform window, audio, input & SDL_GPU (2.9 MB)" },
  { name: "@cap/stb", lic: "MIT", cat: "graphics", desc: "Single-header image decode/encode/resize" },
  { name: "@cap/freetype", lic: "FTL", cat: "graphics", desc: "Industry-standard font rasterizer & glyph engine" },
  { name: "@cap/harfbuzz", lic: "MIT", cat: "graphics", desc: "Multi-script / CJK / Arabic text shaping" },
  { name: "@cap/cmark", lic: "BSD-2", cat: "graphics", desc: "CommonMark official fast Markdown→HTML renderer" },
  { name: "@cap/zstd", lic: "BSD-3", cat: "core", desc: "Zstandard high-ratio compression (914 KB)" },
  { name: "@cap/lz4", lic: "BSD-2", cat: "core", desc: "GB/s real-time lossless compression (180 KB)" },
  { name: "@cap/snappy", lic: "BSD-3", cat: "core", desc: "Google high-speed compression (67 KB)" },
  { name: "@cap/miniz", lic: "MIT", cat: "core", desc: "Single-file ZIP archive & DEFLATE (140 KB)" },
  { name: "@cap/yyjson", lic: "MIT", cat: "core", desc: "Fastest C JSON reader & writer (303 KB)" },
  { name: "@cap/xxhash", lic: "BSD-2", cat: "core", desc: "Ultra-fast XXH3/XXH64 hashing (51 KB)" },
  { name: "@cap/picohttp", lic: "MIT", cat: "core", desc: "Zero-copy HTTP/1.x parser (7.8 KB)" },
  { name: "@cap/hdrhistogram", lic: "CC0", cat: "core", desc: "Nanosecond high-dynamic-range latency histogram (28 KB)" },
  { name: "@cap/treesitter", lic: "MIT", cat: "core", desc: "Incremental AST parsing runtime (tree-sitter)" },
  { name: "@cap/yaml", lic: "MIT", cat: "core", desc: "Industrial YAML 1.1/1.2 parser (libyaml)" },
  { name: "@cap/expat", lic: "MIT", cat: "core", desc: "Fast streaming XML SAX parser (libexpat)" },
  { name: "@cap/pcre2", lic: "BSD-3", cat: "core", desc: "Full Perl regex with SLJIT native machine-code JIT" },
  { name: "@cap/utf8proc", lic: "MIT", cat: "core", desc: "Unicode 16.0 NFC/NFD/NFKC/Casefold normalization" },
  { name: "@cap/sodium", lic: "ISC", cat: "core", desc: "Modern cryptography (Ed25519 / XChaCha20-Poly1305)" },
  { name: "@cap/mbedtls", lic: "Apache-2.0", cat: "core", desc: "Lightweight ARM Mbed TLS crypto core (80 KB)" },
  { name: "@cap/curl", lic: "curl", cat: "core", desc: "Multiprotocol HTTP/HTTPS/FTP/Proxy client (845 KB)" },
];

// Install tab switcher + copy
document.querySelectorAll(".install-tab").forEach((btn) => {
  btn.addEventListener("click", () => {
    document.querySelectorAll(".install-tab").forEach((b) => b.classList.remove("active"));
    btn.classList.add("active");
    document.getElementById("install-cmd").textContent = INSTALL_CMDS[btn.dataset.os];
  });
});

document.getElementById("copy-install").addEventListener("click", async (e) => {
  const txt = document.getElementById("install-cmd").textContent;
  try {
    await navigator.clipboard.writeText(txt);
    e.target.textContent = "Copied!";
    setTimeout(() => (e.target.textContent = "Copy"), 1500);
  } catch {
    e.target.textContent = "Select & Copy";
  }
});

// Code showcase tabs
function renderCode(key) {
  document.getElementById("code-display").textContent = CODE_SAMPLES[key] || "";
}
document.querySelectorAll(".code-tab").forEach((btn) => {
  btn.addEventListener("click", () => {
    document.querySelectorAll(".code-tab").forEach((b) => b.classList.remove("active"));
    btn.classList.add("active");
    renderCode(btn.dataset.sample);
  });
});
renderCode("rest");

// Benchmark renderer + category switcher
let currentBenchCat = "all";
let allBenchmarks = DEFAULT_BENCHMARKS;

function renderBenchmarks(cards) {
  const grid = document.getElementById("bench-grid");
  grid.innerHTML = "";
  const visible = currentBenchCat === "all"
    ? cards
    : cards.filter((c) => c.cat === currentBenchCat);
  visible.forEach((c) => {
    const maxVal = Math.max(...c.rows.map((r) => r.value), 1);
    const card = document.createElement("div");
    card.className = "bench-card";
    card.innerHTML = `
      <h3>${c.title}</h3>
      <div class="bench-sub">${c.subtitle}</div>
      ${c.rows
        .map((r) => {
          const pct = Math.max(4, Math.round((r.value / maxVal) * 100));
          return `
            <div class="bar-row">
              <div class="bar-label"><span>${r.label}</span><span>${r.display}</span></div>
              <div class="bar-track"><div class="bar-fill ${r.key}" style="width:${pct}%"></div></div>
            </div>`;
        })
        .join("")}
    `;
    grid.appendChild(card);
  });
}

document.querySelectorAll(".bench-tab").forEach((btn) => {
  btn.addEventListener("click", () => {
    document.querySelectorAll(".bench-tab").forEach((b) => b.classList.remove("active"));
    btn.classList.add("active");
    currentBenchCat = btn.dataset.benchCat || "all";
    renderBenchmarks(allBenchmarks);
  });
});

// Render verified same-machine benchmarks immediately; optionally enrich CPU label from results.json
renderBenchmarks(allBenchmarks);
fetch("../bench/vs_runtimes/results.json")
  .then((r) => (r.ok ? r.json() : null))
  .then((data) => {
    if (data && data.cpu) {
      document.getElementById("bench-meta").innerHTML =
        `Measured on <strong>${data.cpu}</strong> (<code>-OReleaseFast</code>) vs Bun 1.3.14 &amp; Node v26.1.0. Reproducible via <code>bench/vs_web_frameworks/</code> &amp; <code>bench/vs_runtimes/</code>.`;
    }
  })
  .catch(() => {});

// Packages explorer
let currentCat = "all";
function renderPackages() {
  const q = document.getElementById("pkg-search").value.trim().toLowerCase();
  const grid = document.getElementById("pkg-grid");
  const filtered = PACKAGES.filter((p) => {
    if (currentCat !== "all" && p.cat !== currentCat) return false;
    if (!q) return true;
    return p.name.toLowerCase().includes(q) || p.desc.toLowerCase().includes(q) || p.lic.toLowerCase().includes(q);
  });
  grid.innerHTML = filtered
    .map(
      (p) => `
      <div class="pkg-card">
        <div class="pkg-top">
          <span class="pkg-name">${p.name}</span>
          <span class="pkg-lic">${p.lic}</span>
        </div>
        <div class="pkg-desc">${p.desc}</div>
      </div>`
    )
    .join("");
}

document.getElementById("pkg-search").addEventListener("input", renderPackages);
document.querySelectorAll(".filter-btn").forEach((btn) => {
  btn.addEventListener("click", () => {
    document.querySelectorAll(".filter-btn").forEach((b) => b.classList.remove("active"));
    btn.classList.add("active");
    currentCat = btn.dataset.cat;
    renderPackages();
  });
});
renderPackages();
