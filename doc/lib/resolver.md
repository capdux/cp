# CP 标准库：模块解析引擎与路径别名手册 (`cap:resolver`)

> 参考来源：`/data/dev/bun/src/resolver/`, `/data/dev/bun/docs/guides/runtime/tsconfig-paths.mdx` | 吸收日期：2026-11-22

## 一、 架构总览与设计准则

`cap:resolver` 是 CP 语言官方内置的高性能模块解析引擎标准库，完整吸收自 Bun `src/resolver/`（`data_url.zig`、`package_json.zig`、`tsconfig_json.zig`、`resolver.zig`、`dir_info.cp`、`stat_hash.zig`）。该库面向编译器、打包器与包管理工具提供一站式模块定位能力，支持 RFC 2397 `data:` URL、`package.json` `exports`/`imports`/`browser`/`sideEffects` 路由与自引用、JSONC `tsconfig.json` `paths` 别名与 `extends` 继承链、协议识别（`cap:*` / `node:*` / `bun:*`）、目录元数据缓存（`DirEntryCache` + `StatHash`）、软链接穿透解析（`RealpathResolver` 含 `ELOOP` 环路防护），以及 W3C SRI 校验与 HostedGit 解析。

```text
┌────────────────────────────────────────────────────────────────────────────┐
│                        cap:resolver 模块解析流水线                          │
├──────────────────────┬───────────────────────────┬─────────────────────────┤
│ 1. 协议与数据 URL 层   │  2. 配置与包高级语义层      │  3. FS 缓存与符号链接层   │
│   cap:* / node:*     │    TsConfigChainLoader    │    DirEntryCache        │
│   bun:* 内置协议      │      extends 继承 & 重定位  │      getdents64 + 失效   │
│   DataURL (RFC 2397) │    PackageJson / SelfRef  │    RealpathResolver     │
│     Base64 / %XX 解码 │    BrowserMap / SideEff   │      MAX_SYMLINK_DEPTH=40│
└──────────────────────┴───────────────────────────┴─────────────────────────┘
```

### 1. 模块文件拓扑与谱系来源

| 文件路径 | 上游谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/resolver/data_url.cp` | `// Source: /data/dev/bun/src/resolver/data_url.zig` | RFC 2397 `DataURL` 解析、`mime_type` / `charset` / `is_base64` 提取及 `decodeAlloc` |
| `lib/resolver/package_json.cp` | `// Source: /data/dev/bun/src/resolver/package_json.zig` | `PackageJson` 结构化解析、`exports` 条件路由与通配映射、`#imports` 私有导入及依赖查询 |
| `lib/resolver/tsconfig.cp` | `// Source: /data/dev/bun/src/resolver/tsconfig_json.zig` | JSONC `TsConfigJson` 解析（支持 `//`、`/* */` 与尾逗号）及 `paths` 通配别名展开 `resolveAlias` |
| `lib/resolver/resolver.cp` | `// Source: /data/dev/bun/src/resolver/resolver.zig` | `ResolveKind`、`ResolveConditions`、`StatHash`（64 位文件指纹）与 `ModuleResolver` |
| `lib/resolver/dir_cache.cp` | `// Source: /data/dev/bun/src/resolver/dir_info.cp` | `DirEntryCache`、`DirInfo`（`isStale()` 失效检测）与 `RealpathResolver`（多级软链 + `ELOOP` 防护） |
| `lib/resolver/advanced.cp` | `// Source: /data/dev/bun/src/resolver/tsconfig_json.cp & .../package_json.cp` | `TsConfigChainLoader`、`BrowserFieldMap`、`SideEffectsMatcher`、`SelfReferenceResolver` |
| `lib/resolver/integrity.cp` | `// Source: /data/dev/bun/src/install/integrity.cp` | W3C SRI 完整性校验（`IntegrityTag` / `Integrity` 解析、多算法择强、SHA256/512 计算与恒定时间比对） |
| `lib/resolver/hosted_git.cp` | `// Source: /data/dev/bun/src/install/hosted_git_info.cp` | Git 托管地址解析（`GitHost` / `HostedGitInfo`，支持简写、HTTPS、SSH 与 Tarball URL 生成） |
| `lib/resolver/node_fallbacks.cp` | `// Source: /data/dev/bun/src/resolver/node_fallbacks.cp` | 面向浏览器打包目标的 23 个 Node.js 核心模块虚拟 Polyfill 路由注册表（`NodeFallbackModule`、`lookupNodeFallback`、`hasNodeFallback`、`isNodeFallbackVfsPath`、`resolveNodeFallbackFromVfsPath`、`formatNodeFallbackEntryPath`、`formatNodeFallbackPackageJsonPath`） |
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

### 3. JSONC `tsconfig.json` 与 `extends` 继承链 (`TsConfigJson` & `TsConfigChainLoader`)

- `TsConfigJson.parse(allocator, jsonc_src)`：自动剥离 `//` 单行注释、`/* ... */` 块注释与对象/数组尾逗号后解析配置。
- `TsConfigJson.resolveAlias(allocator, specifier)`：按最长前缀匹配原则展开 `paths` 别名（如 `@/* -> src/*`），并结合 `baseUrl` 规范化输出路径。
- `TsConfigChainLoader.loadFile(allocator, tsconfig_path)`：
  - 递归加载 `extends` 继承链（支持相对路径、省略 `.json` 后缀与包路径向上查找），按“父 -> 子”顺序合并 `compilerOptions` 与 `paths`（子配置覆盖同名键，保留父配置独有别名）。
  - 自动按声明配置文件所在目录对 `baseUrl` 执行相对路径重定位（Relocation），并在缺失 `baseUrl` 时重定位父配置 `paths` 目标。
  - 内置 `MAX_EXTENDS_DEPTH = 16` 与规范化路径已访问集合检查，检测到环形 `extends` 时拦截返回 `error.CircularExtends`。

### 4. 统一模块解析器与缓存指纹 (`ModuleResolver` & `StatHash`)

- `ResolveKind`：`ImportStmt`、`RequireCall`、`DynamicImport`、`EntryPoint`、`CssAtImport`。
- `ResolveConditions.defaultForKind(kind, is_browser)`：自动生成条件优先级链（`cp`、`bun`、`import`/`require`、`node`/`browser`、`default`）。
- `StatHash`：`fromMetadata(size, mode, mtime, ino, path)`、`fromPath(path)`，基于 `XxHash64` 生成 64 位元数据缓存指纹。
- `ModuleResolver.resolve(allocator, importer_dir, specifier, kind)`：按 `cap:*`/`node:*`/`bun:*` 内置协议 -> `data:` URL -> `#imports` -> `tsconfig` `paths` -> 相对/绝对路径扩展名与 `index.*` 探测 -> 向上遍历 `cap_modules` / `node_modules` 顺序完成解析。

### 5. 目录元信息缓存与软链接解析 (`DirEntryCache` & `RealpathResolver`)

- `DirEntryCache`：
  - 基于 `cap:sys` `DirIterator`（`getdents64`）单次遍历目录并缓存全部 `CachedDirEntry`、`has_package_json`、`has_tsconfig_json`、`has_node_modules` / `has_cap_modules` 标志、`node_modules_dir` / `cap_modules_dir` 路径以及目录自身的 64 位 `dir_stat_hash`。
  - `getOrLoad(dir_path)` / `getFresh(dir_path)` / `lookup(dir_path)` / `invalidate(dir_path)` / `stats()`：支持命中/未命中/失效统计与 `DirInfo.isStale()` 实时磁盘变更探测。
  - `findNearestPackageDir(start_dir)` 与 `findNearestModulesDir(start_dir, prefer_cap_modules)`：基于缓存向上攀升定位工程配置与依赖目录。
- `RealpathResolver`：
  - `resolve(allocator, raw_path)` / `resolveDetailed(allocator, raw_path, max_depth)`：逐段执行 `lstat` + `readlink` 符号链接展开（兼容多级相对/绝对软链接及中间目录软链接），默认深度上限 `MAX_SYMLINK_DEPTH = 40`，遇到符号链接环路时拦截返回 `error.ELOOP`。

### 6. 包高级语义 (`BrowserFieldMap` / `SideEffectsMatcher` / `SelfReferenceResolver`)

- `BrowserFieldMap`：解析 `package.json` `"browser"` 字符串或映射对象（如 `{"./server.js": "./browser.js", "fs": false}`），支持规范化键与无扩展名查询，提供 `lookup(specifier)`（`Unmapped` / `Remap` / `Disabled`）、`isDisabled(specifier)` 与 `remap(specifier)`。
- `SideEffectsMatcher`：解析 `"sideEffects": false | true | ["*.css", "./src/polyfill.js"]`，接入 `cap:glob` `Glob.match` 进行 Tree-Shaking 副作用判定（无 `/` 模式自动匹配任意层级文件名）。
- `SelfReferenceResolver`：支持包内文件通过自身 `package.json` `"name"` 与 `"exports"` 进行自引用导入解析（如 `import "my-lib/subpath"`），提供 `resolveWithPackage(...)` 与向上查找最近 `package.json` 的 `resolveFromDir(...)`。

### 7. W3C SRI 校验与 Git 托管仓库解析 (`Integrity` & `HostedGitInfo`)

- `Integrity`：支持 `sha1`/`sha256`/`sha384`/`sha512` 解析、多算法择强 `parseMultiple` 与恒定时间校验 `verify`。
- `HostedGitInfo`：支持 GitHub/GitLab/Bitbucket/Gist/SourceHut 简写、快捷协议、HTTPS、SSH 解析与 URL 生成。

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import {
    DataURL,
    PackageJson,
    TsConfigJson,
    TsConfigChainLoader,
    DirEntryCache,
    RealpathResolver,
    BrowserFieldMap,
    SideEffectsMatcher,
    SelfReferenceResolver,
    ModuleResolver,
    ResolveKind,
    StatHash,
} from "cap:resolver";

pub fn demoResolver(): !void {
    let arena_alloc = std.heap.ArenaAllocator.init(std.heap.page_allocator);
    defer arena_alloc.deinit();
    const alloc = arena_alloc.allocator();

    // 1. DirEntryCache 与 RealpathResolver
    let dcache = DirEntryCache.init(alloc);
    defer dcache.deinit();
    const dinfo = dcache.getOrLoad(".")?;
    const real = RealpathResolver.resolve(alloc, ".")?;
    * = dinfo.entryCount();
    * = real;

    // 2. TsConfigChainLoader 多级 extends 合并与 Browser/SideEffects/SelfRef
    const bmap = BrowserFieldMap.parse(alloc, "{\"browser\":{\"./srv.js\":\"./cli.js\",\"fs\":false}}")?;
    const se = SideEffectsMatcher.parse(alloc, "{\"sideEffects\":[\"*.css\",\"./src/init.cp\"]}", "/proj")?;
    * = bmap.isDisabled("fs");
    * = se.hasSideEffects("/proj/src/theme/main.css");

    // 3. 解析 Base64 DataURL 与内置模块
    const du = (DataURL.parse("data:text/plain;charset=utf-8;base64,SGVsbG8=") catch null).?;
    const decoded = du.decodeAlloc(alloc)?;
    * = decoded;

    let resolver = ModuleResolver.init(".");
    const res = resolver.resolve(alloc, ".", "cap:io", ResolveKind.ImportStmt)?;
    const fp = StatHash.fromMetadata(1024, 0o100644, 1700000000, 42, res.path);
    * = fp.value;
}
```

---

## 上游 Bun 行为契约与安全边界注意事项

1. **`package.json` `exports` 条件匹配顺序与回退（`PackageJson` / `ResolveConditions`）**：
   - 条件对象按 `package.json` 中键的**声明顺序**匹配（而非调用方传入条件集合的顺序），`"default"` 应置于末尾作为兜底；子路径通配模式（如 `"./*"`）按最长静态前缀优先匹配；当目标映射为 `null` 时表示显式封锁该子路径（阻止内部私有文件泄漏）。
2. **`tsconfig.json` `paths` 与 `extends` 继承链（`TsConfigChainLoader`）**：
   - 参考 `/data/dev/bun/docs/guides/runtime/tsconfig-paths.mdx`：`paths` 别名映射相对于声明该 `paths` 的配置文件所在目录（或 `baseUrl`）解析；`TsConfigChainLoader` 强制限制最大继承深度（`MAX_EXTENDS_DEPTH`）以拦截循环 `extends` 链。
3. **符号链接环路与目录缓存失效（`RealpathResolver` & `DirEntryCache`）**：
   - `RealpathResolver` 严格限制最大软链接跳转次数（`MAX_SYMLINK_DEPTH = 40`），遇环路立即返回 `ELOOP` 错误而非无限递归；`DirEntryCache` 通过 `DirInfo.isStale()` 与 `StatHash` 探测目录变更，兼顾高频解析性能与热更新一致性。
4. **W3C SRI 多算法择强与恒定时间校验（`Integrity`）**：
   - 当完整性字符串包含多个算法摘要（如 `sha256-... sha512-...`）时，按 W3C SRI 规范自动选择最强支持算法进行校验，并采用恒定时间字节比对防止侧信道泄露。
