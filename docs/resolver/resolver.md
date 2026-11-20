# CP 标准库：模块解析引擎与路径别名手册 (`cap:resolver`)

## 一、 架构总览与设计准则

`cap:resolver` 是 CP 语言官方内置的高性能模块解析引擎标准库，完整吸收自 Bun `src/resolver/`（`data_url.zig`、`package_json.zig`、`tsconfig_json.zig`、`resolver.zig`、`stat_hash.zig`）。该库面向编译器、打包器与包管理工具提供一站式模块定位能力，支持 RFC 2397 `data:` URL、`package.json` `exports`/`imports` 条件路由、JSONC `tsconfig.json` `paths` 别名重写、协议识别（`cap:*` / `node:*` / `bun:*`）、扩展名/目录索引探测以及 64 位元数据缓存指纹（`StatHash`）。

```text
┌─────────────────────────────────────────────────────────────────────────┐
│                        cap:resolver 模块解析流水线                       │
├──────────────────────┬─────────────────────────┬────────────────────────┤
│ 1. 协议与数据 URL 层   │  2. 配置与别名重写层      │  3. 路径探测与指纹层    │
│   cap:* / node:*     │    TsConfigJson (JSONC) │    probeFile()         │
│   bun:* 内置协议      │      paths: @/* -> src/*│      .cp/.ts/.js/.json │
│   DataURL (RFC 2397) │    PackageJson          │      index.* / pkg.main│
│     Base64 / %XX 解码 │      exports / imports  │    StatHash (XxHash64) │
└──────────────────────┴─────────────────────────┴────────────────────────┘
```

### 1. 模块文件拓扑与谱系来源

| 文件路径 | 上游谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/resolver/data_url.cp` | `// Source: /data/dev/bun/src/resolver/data_url.zig` | RFC 2397 `DataURL` 解析、`mime_type` / `charset` / `is_base64` 提取及 `decodeAlloc` |
| `lib/resolver/package_json.cp` | `// Source: /data/dev/bun/src/resolver/package_json.zig` | `PackageJson` 结构化解析、`exports` 条件路由与通配映射、`#imports` 私有导入及依赖查询 |
| `lib/resolver/tsconfig.cp` | `// Source: /data/dev/bun/src/resolver/tsconfig_json.zig` | JSONC `TsConfigJson` 解析（支持 `//`、`/* */` 与尾逗号）及 `paths` 通配别名展开 `resolveAlias` |
| `lib/resolver/resolver.cp` | `// Source: /data/dev/bun/src/resolver/resolver.zig` | `ResolveKind`、`ResolveConditions`、`StatHash`（64 位文件指纹）与 `ModuleResolver` |
| `lib/resolver/integrity.cp` | `// Source: /data/dev/bun/src/install/integrity.cp` | W3C SRI 完整性校验（`IntegrityTag` / `Integrity` 解析、多算法择强、SHA256/512 计算与恒定时间比对） |
| `lib/resolver/hosted_git.cp` | `// Source: /data/dev/bun/src/install/hosted_git_info.cp` | Git 托管地址解析（`GitHost` / `HostedGitInfo`，支持简写、HTTPS、SSH 与 Tarball URL 生成） |
| `lib/resolver/index.cp` | `// Source: /data/dev/bun/src/resolver/` | `cap:resolver` 对外统一导出入口 |

---

## 二、 核心 API 与设计语义

### 1. RFC 2397 Data URL (`DataURL`)

- `DataURL.parse(url)`：若不以 `data:` 开头返回 `null`，否则解析 `data:[<mediatype>][;charset=...][;base64],<data>`。
- 字段：`mime_type`（缺省为 `"text/plain"`）、`charset`（可空，`effectiveCharset()` 缺省返回 `"US-ASCII"`）、`is_base64`、`raw_data`。
- `decodeAlloc(allocator)`：先执行百分号解码（复用 `cap:url` `PercentEncoding`），若 `is_base64 == true` 则进一步调用 `cap:encoding` `Base64.decodeAlloc` 解码字节流。

### 2. `package.json` 解析与 `exports` / `imports` 路由 (`PackageJson`)

- `PackageJson.parse(allocator, json_src)`：提取 `name`、`version`、`module_type`（`"module"` / `"commonjs"`）、`main`、`module`、`browser`、`exports`、`imports`、`dependencies`、`devDependencies`、`peerDependencies`。
- `resolveExport(allocator, subpath, conditions)`：支持字符串映射、子路径映射（`"./foo"`）、嵌套条件对象（`import` / `require` / `bun` / `cp` / `default` / `types` / `node` / `browser`）、数组回退、`null` 显式禁用以及通配前缀/后缀（`"./*"` -> `"./dist/*.js"`）。
- `resolveImport(allocator, specifier, conditions)`：解析 `#internal` 私有导入映射。
- `getDependency(name)` / `getDevDependency(name)` / `hasDependency(name)`：依赖版本查找。

### 3. JSONC `tsconfig.json` 解析 (`TsConfigJson`)

- `TsConfigJson.parse(allocator, jsonc_src)`：自动剥离 `//` 单行注释、`/* ... */` 块注释与对象/数组尾逗号后解析配置。
- 字段：`extends`、`base_url`、`root_dir`、`out_dir`、`target`、`jsx`、`jsx_factory`、`jsx_fragment_factory`、`jsx_import_source`、`paths`。
- `resolveAlias(allocator, specifier)`：按最长前缀匹配原则展开 `paths` 别名（如 `@/* -> src/*`），并结合 `baseUrl` 规范化输出路径。

### 4. 统一模块解析器与缓存指纹 (`ModuleResolver` & `StatHash`)

- `ResolveKind`：`ImportStmt`、`RequireCall`、`DynamicImport`、`EntryPoint`、`CssAtImport`。
- `ResolveConditions.defaultForKind(kind, is_browser)`：自动生成条件优先级链（`cp`、`bun`、`import`/`require`、`node`/`browser`、`default`）。
- `StatHash`：`fromMetadata(size, mode, mtime, ino, path)`、`fromStat(st, path)`、`fromPath(path)`，基于 `XxHash64` 生成 64 位元数据缓存指纹。
- `ModuleResolver.resolve(allocator, importer_dir, specifier, kind)`：按 `cap:*`/`node:*`/`bun:*` 内置协议 -> `data:` URL -> `#imports` -> `tsconfig` `paths` -> 相对/绝对路径扩展名与 `index.*` 探测 -> 向上遍历 `cap_modules` / `node_modules`（结合 `package.json` `exports` 条件路由）顺序完成解析。

### 5. W3C Subresource Integrity (SRI) 完整性校验 (`Integrity` & `IntegrityTag`)

- `IntegrityTag`：`Unknown`、`Sha1`、`Sha256`、`Sha384`、`Sha512`，按密码学强度分级（`Sha512 > Sha384 > Sha256 > Sha1`）。
- `Integrity.parse(sri)` / `Integrity.parseMultiple(sri_list)`：解析单个 `"sha512-<base64>"` 或空格分隔的多算法 SRI 列表；`parseMultiple` 遵循 W3C SRI 规范跳过未知算法并自动择取强度最高的有效摘要。
- `computeSha256(data)` / `computeSha512(data)` / `computeSha384(data)` / `computeSha1(data)`：基于 `std.crypto.hash` 计算摘要。
- `verify(data)`：重新计算对应算法摘要，并通过恒定时间（Constant-Time）异或比对防止时序旁路攻击。
- `format(buf)` / `formatAlloc(allocator)`：输出标准 `"<algo>-<base64>"` SRI 字符串。

### 6. Git 托管仓库地址解析 (`GitHost` & `HostedGitInfo`)

- `GitHost`：`GitHub`、`GitLab`、`Bitbucket`、`Gist`、`SourceHut`。
- `HostedGitInfo.parse(input)`：零堆分配解析：
  - GitHub 简写：`"user/repo#main"`
  - 快捷协议：`"github:user/repo#v1.0"`、`"gitlab:org/sub/repo"`、`"bitbucket:user/repo"`、`"gist:id"`、`"sourcehut:~user/repo"`
  - 标准 URL 与 SSH：`"https://github.com/user/repo.git#commit"`、`"git+ssh://git@gitlab.com/org/proj.git"`、`"git@github.com:user/repo.git#tag"`
- URL 生成器：`httpsUrlAlloc(allocator)`、`tarballUrlAlloc(allocator)`、`sshUrlAlloc(allocator)`、`gitSshUrlAlloc(allocator)`、`shortcutAlloc(allocator)`。

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import {
    DataURL,
    PackageJson,
    TsConfigJson,
    ModuleResolver,
    ResolveKind,
    StatHash,
} from "cap:resolver";

pub fn demoResolver(): !void {
    let arena_alloc = std.heap.ArenaAllocator.init(std.heap.page_allocator);
    defer arena_alloc.deinit();
    const alloc = arena_alloc.allocator();

    // 1. 解析 Base64 DataURL
    const du = (DataURL.parse("data:text/plain;charset=utf-8;base64,SGVsbG8=") catch null).?;
    const decoded = du.decodeAlloc(alloc)?;
    * = decoded;

    // 2. 解析 JSONC tsconfig.json 并展开 @/* 别名
    const tsc = TsConfigJson.parse(alloc, "{\n  // 注释与尾逗号支持\n  \"compilerOptions\": {\n    \"baseUrl\": \".\",\n    \"paths\": { \"@/*\": [\"src/*\"], },\n  },\n}")?;
    const mapped = tsc.resolveAlias(alloc, "@/core/engine"); // -> "src/core/engine"
    * = mapped;

    // 3. 解析 package.json exports 条件路由
    const pkg = PackageJson.parse(alloc, "{\"name\":\"demo\",\"exports\":{\".\":{\"import\":\"./esm/index.js\",\"require\":\"./cjs/index.js\"}}}")?;
    const entry = pkg.resolveExportPath(alloc, ".", ["import", "default"]); // -> "./esm/index.js"
    * = entry;

    // 4. ModuleResolver 解析内置协议与元数据指纹
    let resolver = ModuleResolver.init(".");
    const res = resolver.resolve(alloc, ".", "cap:io", ResolveKind.ImportStmt)?;
    const fp = StatHash.fromMetadata(1024, 0o100644, 1700000000, 42, res.path);
    * = fp.value;
}
```
