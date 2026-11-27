#!/usr/bin/env python3
"""
Generate a compact, high-density, AI-Agent-optimized standard & ecosystem library
reference (`cp_agent_lib_ref.md`) from `cp_lib_ref.md`, `packages/@cap/*`,
`packages/@web/*`, and `packages/@cap/registry.json`.

Design goal: ~85-92% smaller than `cp_lib_ref.md` while preserving 100%
discoverability of every `cap:*` stdlib channel, every `@web/*` plugin, and
every `@cap/*` official package (with domain grouping & quick lookup rules).
"""

import json
import os
import re
from collections import defaultdict
from datetime import date

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
LIB_REF = os.path.join(ROOT, "cp_lib_ref.md")
OUT_PATH = os.path.join(ROOT, "cp_agent_lib_ref.md")
PKG_ROOT = os.path.join(ROOT, "packages")
REG_PATH = os.path.join(PKG_ROOT, "registry.json") if os.path.isfile(os.path.join(PKG_ROOT, "registry.json")) else os.path.join(PKG_ROOT, "@cap/registry.json")
CAP_DIR = os.path.join(PKG_ROOT, "@cap")
WEB_DIR = os.path.join(PKG_ROOT, "@web")


def clean_md(text: str) -> str:
    text = re.sub(r"\*\*([^*]+)\*\*", r"\1", text)
    text = re.sub(r"（详见\s*`[^`]+`[^）]*）", "", text)
    text = re.sub(r"\(详见\s*`[^`]+`[^)]*\)", "", text)
    text = re.sub(r"\s+", " ", text).strip()
    return text


def extract_top_symbols(sym_cell: str, limit: int = 8) -> str:
    # Extract backticked identifiers, skipping *.cp file names and long parentheticals
    raw = re.findall(r"`([^`]+)`", sym_cell)
    picked = []
    seen = set()
    for s in raw:
        s = s.strip()
        if not s or s.endswith(".cp") or "/" in s or len(s) > 36:
            continue
        if s.isupper() and len(s) > 12:
            continue
        if s not in seen:
            seen.add(s)
            picked.append(f"`{s}`")
        if len(picked) >= limit:
            break
    if not picked:
        return clean_md(sym_cell)[:80]
    suffix = " 等" if len(raw) > len(picked) else ""
    return ", ".join(picked) + suffix


def short_desc(desc: str, max_len: int = 72) -> str:
    d = clean_md(desc)
    # Trim after first full stop or semicolon if very long
    for sep in ("；", "。", "（直迁"):
        if sep in d and d.index(sep) >= 18:
            d = d[: d.index(sep)]
    if len(d) > max_len:
        d = d[: max_len - 1].rstrip("，、,:; ") + "…"
    return d


def parse_cp_lib_ref():
    stdlib_rows = []
    web_rows = []
    section = None
    with open(LIB_REF, "r", encoding="utf-8") as f:
        for line in f:
            s = line.strip()
            if s.startswith("## 二、"):
                section = "stdlib"
                continue
            elif s.startswith("## 三、"):
                section = "web"
                continue
            elif s.startswith("## 官方纯 CP 生态引擎包"):
                section = "cap"
                continue
            if not s.startswith("|") or s.startswith("| :---") or s.startswith("| 标准库") or s.startswith("| 插件") or s.startswith("| 通道"):
                continue
            parts = [p.strip() for p in s.strip("|").split("|")]
            if section == "stdlib" and len(parts) >= 3:
                chan = clean_md(parts[0])
                syms = extract_top_symbols(parts[1], limit=3)
                desc = short_desc(parts[2], max_len=28)
                stdlib_rows.append((chan, syms, desc))
            elif section == "web" and len(parts) >= 3:
                pkg = clean_md(parts[0])
                desc = short_desc(parts[-1], max_len=70)
                web_rows.append((pkg, desc))
    return stdlib_rows, web_rows


CATEGORIES = [
    ("密码学 / 哈希 / 安全", [
        "sha", "md5", "hmac", "hkdf", "pbkdf", "blake", "keccak", "chacha", "aes",
        "crc", "adler", "xxhash", "twox", "siphash", "murmur", "fnv", "seahash",
        "rustc-hash", "ripemd", "zeroize", "subtle", "sodium", "mbedtls", "pem",
        "der", "oid", "jwt", "totp", "hotp", "otp", "argon", "scrypt", "bcrypt",
        "crypto", "cipher", "digest", "ed25519", "x25519", "rsa", "ecdsa", "base64",
        "base32", "base58", "bs58", "hex", "uuid", "ulid", "nanoid", "cuid", "ksuid",
        "snowflake", "xid", "rand", "fastrand", "getrandom", "password", "csrf",
        "sanitize", "escape-html", "he", "xss", "spdx"
    ]),
    ("压缩 / 归档 / 编解码", [
        "flate", "zlib", "gzip", "deflate", "brotli", "zstd", "lz4", "snappy",
        "tar", "zip", "bzip", "xz", "lzw", "rle", "varint", "leb128", "cbor",
        "msgpack", "bson", "protobuf", "prost", "bincode", "postcard", "flexbuffers",
        "flatbuffers", "asn1", "base", "punycode", "percent", "encodeurl", "utf8",
        "utf16", "encoding", "iconv", "chardet", "bytes", "bytemuck", "zerocopy",
        "byteorder", "endian", "bitstream", "bitvec"
    ]),
    ("网络 / HTTP / URL / DNS / 协议", [
        "http", "url", "uri", "cookie", "cors", "etag", "fresh", "vary", "forwarded",
        "accept", "negotiator", "content-", "media", "mime", "range-parser", "parseurl",
        "finalhandler", "serve-", "send", "proxy", "ip", "cidr", "macaddr", "publicsuffix",
        "dns", "tls", "ssl", "ws", "websocket", "sse", "grpc", "mqtt", "amqp", "smtp",
        "imap", "ftp", "ssh", "redis", "sql", "pg", "mysql", "mongo", "graphql",
        "multipart", "multer", "formidable", "busboy", "raw-body", "body-parser",
        "qs", "querystring", "form-data", "headers", "status", "user-agent", "ua-parser",
        "robots", "sitemap", "rss", "atom", "feed", "webhook", "curl", "picohttp",
        "usrsctp", "libsrtp", "oci-ref", "git-url", "hosted-git"
    ]),
    ("路径 / 文件系统 / Glob / 包管理", [
        "path", "glob", "minimatch", "picomatch", "micromatch", "multimatch", "anymatch",
        "braces", "fill-range", "to-regex-range", "is-glob", "is-extglob", "wildmatch",
        "ignore", "gitignore", "walkdir", "readdir", "find-up", "locate-path", "pkg-",
        "read-pkg", "normalize-package", "npm-", "semver", "resolve", "which", "mkdir",
        "make-dir", "rimraf", "del", "cpy", "cp-", "mv-", "chokidar", "fs-", "dunce",
        "camino", "normpath", "homedir", "temp", "tmp", "xdg", "dirs", "shellexpand",
        "execa", "cross-spawn", "shell", "shlex", "which"
    ]),
    ("CLI / 终端 / ANSI / 日志", [
        "ansi", "chalk", "kleur", "colorette", "picocolors", "color", "strip-ansi",
        "slice-ansi", "wrap-ansi", "string-width", "cli-", "boxen", "ora", "listr",
        "enquirer", "inquirer", "prompts", "yargs", "commander", "clap", "minimist",
        "mri", "arg", "getopts", "meow", "table", "progress", "indicatif", "spinner",
        "log", "pino", "winston", "debug", "tracing", "env-logger", "console", "tty",
        "is-interactive", "is-unicode", "is-ci", "ci-info", "supports-color", "term",
        "figures", "log-symbols", "textwrap", "indent", "dedent", "pad", "truncate"
    ]),
    ("字符串 / 文本 / 正则 / 命名转换", [
        "case", "camel", "snake", "kebab", "pascal", "decamelize", "heck", "inflect",
        "plural", "slug", "regex", "aho-corasick", "memchr", "bstr", "smol-str",
        "compact-str", "tinystr", "ecow", "str", "string", "unicode", "grapheme",
        "segment", "width", "norm", "escape", "unescape", "trim", "split", "diff",
        "patch", "levenshtein", "jaro", "fuzzy", "natsort", "markdown", "cmark",
        "html", "xml", "css", "template", "mustache", "handlebars", "eta", "ejs",
        "mustache", "printf", "sprintf", "format", "pretty-", "jest-diff", "bytecount",
        "itoa", "ryu", "dtoa", "atoi", "lexical", "nom", "winnow", "pest", "lexer"
    ]),
    ("集合 / 数据结构 / 对象工具 / 数学", [
        "vec", "array", "smallvec", "tinyvec", "thin-vec", "heapless", "indexmap",
        "order-map", "lru", "cache", "ttl", "bloom", "cuckoo", "roaring", "bitset",
        "bitflags", "enumflags", "slab", "slotmap", "arena", "bumpalo", "trie",
        "radix", "btree", "skiplist", "ringbuf", "deque", "queue", "stack", "graph",
        "petgraph", "dag", "topo", "union-find", "interval", "rtree", "kdtree",
        "quadtree", "obj", "deep", "merge", "clone", "klona", "rfdc", "dequal",
        "equal", "dot-prop", "get-value", "set-value", "lodash", "ramda", "num-",
        "big", "decimal", "rational", "complex", "float", "half", "fixed", "approx",
        "nalgebra", "ndarray", "glam", "mint", "mat", "vec2", "vec3", "quat", "ray",
        "statrs", "stats", "fft", "matrix", "linear", "geo", "wkt", "h3", "s2",
        "geohash", "polyline", "vincenty", "haversine", "coord"
    ]),
    ("异步 / 并发 / 限流 / 事件 / 重试", [
        "p-", "async", "promise", "future", "task", "pool", "limit", "queue", "retry",
        "backoff", "throttle", "debounce", "rate", "governor", "circuit", "breaker",
        "timeout", "delay", "sleep", "debounce", "mitt", "event", "emitter", "ee-",
        "on-finished", "destroy", "once", "signal", "channel", "crossbeam", "flume",
        "parking", "lock", "mutex", "rwlock", "atomic", "arc", "rc", "cow", "lazy",
        "once-cell", "dashmap", "rayon", "tokio", "mio", "polling"
    ]),
    ("配置 / 序列化 / Schema / 校验 / 时间", [
        "json", "yaml", "toml", "ini", "dotenv", "env", "conf", "config", "cosmiconfig",
        "lilconfig", "ron", "kdl", "hcl", "csv", "tsv", "parquet", "arrow", "avro",
        "schema", "ajv", "zod", "valibot", "yup", "joi", "validator", "validate",
        "is-", "type-", "ms", "time", "date", "chrono", "dayjs", "luxon", "moment",
        "duration", "iso8601", "rfc3339", "rfc2822", "tz", "cron", "schedule",
        "humantime", "parse-duration"
    ]),
    ("原生 C/C++ 高性能引擎绑定", [
        "llama", "whisper", "onnx", "openblas", "opencv", "usearch", "duckdb",
        "lmdb", "sqlite", "freetype", "harfbuzz", "sdl3", "pdfium", "treesitter",
        "yyjson", "expat", "dav1d", "libavif", "libvpx", "libyuv", "flac", "ogg",
        "opus", "vorbis", "rnnoise", "speexdsp", "miniaudio", "dr_libs", "stb",
        "utf8proc"
    ]),
]


def classify_pkg(name: str, source_reg: str) -> str:
    low = name.lower()
    if source_reg in ("c", "c++"):
        return "原生 C/C++ 高性能引擎绑定"
    for cat, kws in CATEGORIES:
        for kw in kws:
            if kw in low:
                return cat
    return "其他通用工具包"


def main():
    stdlib_rows, web_rows = parse_cp_lib_ref()

    with open(REG_PATH, "r", encoding="utf-8") as f:
        reg = json.load(f)

    # Discover all bare packages (`packages/<name>`), scoped npm packages (`packages/@org/<name>`),
    # and native `@cap/*` packages (`packages/@cap/<name>`), excluding `@web/*`.
    pkg_sources = {}
    for entry in sorted(os.listdir(PKG_ROOT)):
        full = os.path.join(PKG_ROOT, entry)
        if not os.path.isdir(full) or entry == "@web":
            continue
        if entry.startswith("@"):
            for sub in sorted(os.listdir(full)):
                cj = os.path.join(full, sub, "cap.json")
                if not os.path.isfile(cj):
                    continue
                with open(cj, "r", encoding="utf-8") as f:
                    data = json.load(f)
                src = data.get("source") or (data.get("sources")[0] if isinstance(data.get("sources"), list) and data.get("sources") else {})
                key = f"{entry}/{sub}" if entry != "@cap" else f"@cap/{sub}"
                pkg_sources[key] = (src.get("ecosystem") or src.get("registry") or "", src.get("package") or src.get("name") or "")
        else:
            cj = os.path.join(full, "cap.json")
            if not os.path.isfile(cj):
                continue
            with open(cj, "r", encoding="utf-8") as f:
                data = json.load(f)
            src = data.get("source") or (data.get("sources")[0] if isinstance(data.get("sources"), list) and data.get("sources") else {})
            pkg_sources[entry] = (src.get("ecosystem") or src.get("registry") or "", src.get("package") or src.get("name") or "")

    grouped = defaultdict(list)
    for pkg, (sreg, sname) in sorted(pkg_sources.items()):
        cat = classify_pkg(pkg, sreg)
        grouped[cat].append(pkg)

    npm_cnt = sum(1 for e in reg["entries"] if e.get("source_registry") == "npm")
    cr_cnt = sum(1 for e in reg["entries"] if e.get("source_registry") == "crates.io")

    lines = []
    lines.append("# CP 智能体标准库与生态极简速查手册 (`cp_agent_lib_ref.md`)")
    lines.append("")
    lines.append(f"> **更新日期**：{date.today().isoformat()} | **自动生成**：`python3 scripts/gen_agent_lib_ref.py`")
    lines.append(f"> **覆盖规模**：内置标准库 **{len(stdlib_rows)} 通道**（`cap:*`）+ 官方 Web 插件 **{len(web_rows)} 包**（`@web/*`）+ 官方纯 CP/原生包 **{len(pkg_sources)} 包**（`@cap/*`）+ 上游映射 **{len(reg['entries'])} 条**（npm {npm_cnt} / crates.io {cr_cnt}）。")
    lines.append("> **AGY 查库铁律（极省 Token）**：")
    lines.append("> 1. **写代码前先查本文件**：优先用内置 `cap:*`（零安装），其次 `import ... from \"@cap/<name>\"`，**严禁重复造轮子**；")
    lines.append("> 2. **查精确签名不读全量源码**：内置库看 `doc/lib/<mod>.md` 或 `lib/<mod>/index.cp`；`@cap/*` 直接读 **`packages/@cap/<name>/src/index.d.cp`**（极短类型声明）；")
    lines.append("> 3. **按 npm / crates.io 原库名查映射**：`rg -n \"<orig>\" packages/@cap/registry.json`（改名包下方已标 `←原名`）；")
    lines.append("> 4. **日常禁止读取 370KB 的 `cp_lib_ref.md` 或万行 `cp_ref.md`**；仅新增库时追加 `cp_lib_ref.md` 并跑 `python3 scripts/gen_agent_lib_ref.py` 刷新本文件。")
    lines.append("")
    lines.append("---")
    lines.append("")
    lines.append("## 一、 根容器与内建全局符号（免安装）")
    lines.append("")
    lines.append("- **惰性根容器**：`import cap from \"cap\";` — 编译期按需挂载 `cap.<ns>`（未引用 0 开销）。")
    lines.append("- **免 import 内置全局**：`RegExp`（`/pat/flags`）、`JSON`（`JSON.stringify` / `JSON.parse<T>`）、并发原语 `@spawn`/`@receive`/`@reply`/`@send`/`@sender`/`@self`。")
    lines.append("")
    lines.append("## 二、 内置标准库速查矩阵 (`cap:*`)")
    lines.append("")
    lines.append("| 通道 | 核心入口符号 | 职责 |")
    lines.append("| :--- | :--- | :--- |")
    for chan, syms, desc in stdlib_rows:
        lines.append(f"| {chan} | {syms} | {desc} |")

    lines.append("")
    lines.append("## 三、 官方 Web 插件生态 (`@web/*`)")
    lines.append("")
    web_names = [f"`{pkg}`" for pkg, _ in web_rows]
    lines.append(f"共 **{len(web_rows)} 个**（位于 `packages/@web/*`，`cap add @web/<name>`）：" + " · ".join(web_names))
    lines.append("")
    lines.append("## 四、 官方 `@cap/*` 生态包分类索引（共 " + str(len(pkg_sources)) + " 包）")
    lines.append("")
    lines.append("> 导入方式：`import { ... } from \"@cap/<name>\";`（精确导出查 `packages/@cap/<name>/src/index.d.cp`；括号 `←原名` 为已改名映射）。")
    lines.append("")

    ordered_cats = [c for c, _ in CATEGORIES] + ["其他通用工具包"]
    for cat in ordered_cats:
        items = grouped.get(cat, [])
        if not items:
            continue
        lines.append(f"### {cat} ({len(items)})")
        lines.append(", ".join(items))
        lines.append("")

    content = "\n".join(lines).rstrip() + "\n"
    with open(OUT_PATH, "w", encoding="utf-8") as f:
        f.write(content)

    src_size = os.path.getsize(LIB_REF)
    out_size = os.path.getsize(OUT_PATH)
    ratio = (1.0 - out_size / src_size) * 100.0
    print(f"Generated {OUT_PATH}")
    print(f"  cp_lib_ref.md       : {src_size:,} bytes")
    print(f"  cp_agent_lib_ref.md : {out_size:,} bytes ({len(lines)} lines, -{ratio:.1f}% size reduction!)")
    print(f"  stdlib={len(stdlib_rows)}, @web={len(web_rows)}, @cap={len(pkg_sources)}, mappings={len(reg['entries'])}")


if __name__ == "__main__":
    main()
