# `cap:pkg` — CP 工业级包管理核心标准库文档 (`lib/pkg/`)

> 参考来源：`/data/dev/bun/docs/pm/{global-cache,isolated-installs}.mdx`, `/data/dev/bun/src/install/`, `/data/dev/bun/src/install_types/` | 吸收日期：2026-11-22
> **设计原则**：零 `@import("bun")`、零 `std.posix`、零 JSC 耦合、100% 纯正 CP 语法，复用 `cap:semver`、`cap:pack`、`cap:patch`、`cap:json`、`cap:encoding`、`cap:hash`、`cap:sys`、`cap:glob`、`cap:io` 与 `lib/{http,conf,resolver}/`。

---

## 一、 模块架构总览与文件清单

| 文件路径 | 行数 | 迁移源（`// Source:`） | 核心职责 |
| :--- | :---: | :--- | :--- |
| [lib/pkg/types.cp](file:///data/dev/cp/lib/pkg/types.cp) | 496 | `install_types/ExternalString.cp` | 紧凑字符串 `SemverString`/`StringBuf`/`ExternalString`/`SlicedString`、`ExternalSlice<T>`、`NodeLinker`、`PackageID`/`DependencyID`/`ResolutionID`、`Behavior` 位域、`PkgError` |
| [lib/pkg/dependency.cp](file:///data/dev/cp/lib/pkg/dependency.cp) | 603 | `install/dependency.cp` | 依赖版本与全协议解析器 `DependencyVersionTag`/`DependencyVersion.parse`、`Dependency`、`DependencyList` |
| [lib/pkg/resolution.cp](file:///data/dev/cp/lib/pkg/resolution.cp) | 562 | `install/resolution.cp` | 解析结果 `ResolutionTag`/`Resolution`、`RepositoryInfo`、平台过滤位掩码 `OperatingSystem`/`Architecture`/`Libc`、`PackageMeta`、`BinLink` |
| [lib/pkg/npm.cp](file:///data/dev/cp/lib/pkg/npm.cp) | 540 | `install/npm.cp` | `.npmrc` 作用域 Registry 与 Bearer/Basic 鉴权路由 `RegistryConfig`/`RegistryScope`、Packument 解析与 Semver 最优匹配 `NpmPackageManifest` |
| [lib/pkg/lockfile.cp](file:///data/dev/cp/lib/pkg/lockfile.cp) | 1057 | `install/lockfile.cp` | 工业级依赖图引擎 `Lockfile`（环检测 `hasCycle`、拓扑排序 `topoOrder`）、文本 `cap.lock` JSON 与紧凑二进制 `CPLK0001`（含 CRC32 校验）双向持久化 |
| [lib/pkg/tarball.cp](file:///data/dev/cp/lib/pkg/tarball.cp) | 294 | `install/extract_tarball.cp` | 安全 Tarball 构建/校验/解包引擎 `TarballBuilder`/`TarballExtractor`（路径穿越拦截、W3C SRI 校验、可执行权限还原） |
| [lib/pkg/linker.cp](file:///data/dev/cp/lib/pkg/linker.cp) | 331 | `install/hoisted_install.cp` | 模块树链接与补丁引擎 `InstallStrategy`（`Hoisted`/`Isolated`）、`BinLinker`（`.bin` 软链与 `0o755`）、`PackageLinker`（硬链/拷贝/软链与 `cap:patch` 补丁注入） |
| [lib/pkg/manager.cp](file:///data/dev/cp/lib/pkg/manager.cp) | 615 | `install/PackageManager.cp` | 端到端包管理引擎 `PackageManager`（`package.json` → 增量 `cap.lock` → 解析 → 缓存/解包+SRI → Hoisted/Isolated 链接 + `.bin` + 补丁 → Lockfile 落盘） |
| [lib/pkg/workspace.cp](file:///data/dev/cp/lib/pkg/workspace.cp) | 723 | `install/lockfile.cp` & `PnpmMatcher.cp` | Monorepo Workspaces 引擎 `WorkspaceGraph`/`WorkspaceMember`/`CatalogMap`（`cap:glob` 扫描、`workspace:*`/`^`/`~`、`catalog:`/`catalog:<name>`、成员 DAG `topoSortMembers()` 与环检测） |
| [lib/pkg/registry_client.cp](file:///data/dev/cp/lib/pkg/registry_client.cp) | 894 | `install/NetworkTask.cp` & `PackageManifestMap.cp` | 工业级 Registry HTTP 客户端与缓存层 `RegistryHttpClient`/`RegistryCacheEntry`/`RetryPolicy`/`MockRegistryTransport`（`Accept`、`If-None-Match`/304、`Bearer`/`Basic`、`<cache_dir>/manifests/<hash>.{json,etag,meta}` 磁盘缓存、429/5xx + `Retry-After` 退避） |
| [lib/pkg/lifecycle.cp](file:///data/dev/cp/lib/pkg/lifecycle.cp) | 567 | `install/lifecycle_script_runner.cp` & `postinstall_optimizer.cp` | 零信任生命周期脚本执行器 `LifecycleRunner`/`TrustedPolicy`/`LifecyclePhase`（`trustedDependencies` 白名单拦截 `blocked_scripts`、`PosixSpawn` + `Pipe` 捕获、注入 `PATH` + `npm_*` + `INIT_CWD`） |
| [lib/pkg/migration.cp](file:///data/dev/cp/lib/pkg/migration.cp) | 1248 | `install/migration.cp` & `yarn.cp` & `pnpm.cp` | 多生态 Lockfile 迁移器 `LockfileMigrator`（`package-lock.json` v2/v3、`yarn.lock` v1、`pnpm-lock.yaml` v6-v9 YAML AST）与依赖图诊断引擎 `DependencyAnalyzer`（`why`/`findDuplicates`/`auditIntegrity`） |
| [lib/pkg/index.cp](file:///data/dev/cp/lib/pkg/index.cp) | 18 | `install/install.cp` | `cap:pkg` 统一导出入口（重导出全部 12 个核心子模块及 `Integrity`/`IntegrityTag`） |
| **核心库合计** | **7,948** | — | **13 个模块全部 `cpc check` 0 报错** |
| [cap/test/test_pkg_install_deep.cp](file:///data/dev/cp/cap/test/test_pkg_install_deep.cp) | 607 | — | Phase B-1 6 大深度端到端测试套件（6/6 100% PASS） |
| [cap/test/test_phase_b3_pkg_production.cp](file:///data/dev/cp/cap/test/test_phase_b3_pkg_production.cp) | 405 | — | Phase B-3 4 大生产子系统端到端测试套件（4/4 100% PASS） |
| **总代码量** | **8,960** | — | — |

---

## 二、 核心子系统详述

### 1. 紧凑基础类型与双态字符串 (`lib/pkg/types.cp`)
- **`SemverString` & `StringBuf`**：8 字节定宽双态字符串。长度 $\le 8$（第 8 字节最高位为 0）时直接内联存储于栈/结构体内部；超长字符串写入 `StringBuf` 并通过 64 位 Wyhash 去重池复用，头部置最高位编码 31-bit 偏移与 31-bit 长度。提供 `SemverString.sliceOf(&s, buf)` 零拷贝切片访问。
- **`ExternalString` & `SlicedString`**：附带 64 位 Wyhash 指纹的双态字符串与缓冲锚定切片视图。
- **`ExternalSlice<T>`**：`(off: u32, len: u32)` 紧缩索引切片，支持 `invalid`、`contains`、`get`/`mut` 边界切片。
- **`Behavior`**：按位编码 `prod`、`optional`、`dev`、`peer`、`workspace`、`bundled` 六态依赖属性，支持 `isOptionalPeer()`、`isEnabled(InstallFeatures)` 与 Bun 规范排序 `cmp()`（`workspace < dev < optional < prod < peer`）。
- **`NodeLinker`**：`Auto` / `Hoisted` / `Isolated` 链接策略枚举。

### 2. 全协议依赖解析 (`lib/pkg/dependency.cp`)
- **`DependencyVersionTag`**：完整覆盖 `Npm`、`DistTag`、`Tarball`、`Folder`、`Symlink`、`Workspace`、`Git`、`GitHub`、`Catalog`、`Uninit` 10 种协议标签，内置精确前缀/语法推断状态机 `DependencyVersionTag.infer(...)`。
- **`DependencyVersion.parse(alias_name, raw_spec)`**：
  - Semver 范围（`^1.2.3`、`~2.0.1`、`>=1.0.0 <2.0.0`、`v1.0.0`），通过 `cap:semver` `SemverRange.isValidRange` 校验；
  - NPM 包别名（`npm:@acme/core@^2.0.0`、`npm:lodash@latest`）；
  - Workspace 协议（`workspace:*`、`workspace:^1.0.0`）；
  - 本地路径与局部 Tarball（`file:../foo`、`file:./pkg.tgz`、`./a.tgz`）；
  - 软链协议（`link:../bar`）；
  - 远程 Tarball（`https://.../pkg.tgz`）；
  - Git / GitHub（`github:owner/repo#ref`、`git+ssh://...`、`git+https://...`、SCP 形式 `git@host:user/repo.git`），集成 `HostedGitInfo.parse`；
  - Dist-tag（`latest`、`next`）与 Catalog（`catalog:`、`catalog:group`）。
- **`DependencyList`**：按包名自动去重并合并 `Behavior` 位域，提供 `addFromPackageJsonRoot` 与规范排序 `sort()`。

### 3. 解析结果、平台位掩码与二进制链接模型 (`lib/pkg/resolution.cp`)
- **`Resolution`**：支持 `Root`、`Npm`、`LocalTarball`、`RemoteTarball`、`Folder`、`Git`、`GitHub`、`Workspace`、`Symlink`、`SingleFileModule`，支持 `fromTextLockfile` 与格式化输出 `format`。
- **`OperatingSystem` / `Architecture` / `Libc`**：位掩码平台匹配引擎，支持 `"os": ["linux", "!win32"]` 等带 `!` 取反规则的 JSON 数组解析与当前宿主平台匹配 `PackageMeta.isDisabledForCurrentPlatform()`。
- **`PackageMeta`**：封装包 `id`、`origin`、平台掩码、`has_install_script`、`is_trusted`、W3C SRI `Integrity` 及 `tarball_url`。
- **`BinLink`**：支持 `File`（字符串路径，自动取 unscoped 包名）、`NamedFile`（单键值对象）、`Map`（多可执行命令映射）、`Dir`（`directories.bin` 目录扫描）。

### 4. NPM Registry 配置与 Packument 解析 (`lib/pkg/npm.cp`)
- **`RegistryConfig` & `RegistryScope`**：
  - 完整解析 `.npmrc`：`registry=`、`@scope:registry=`、`//host/:_authToken=`、`//host/:_auth=`、`//host/:username=`、`//host/:_password=`、`always-auth=` 与注释跳过；
  - `resolveForPackage(pkg_name)` 按包作用域路由 Registry 与凭证，生成 `Bearer` / `Basic`（基于 `cap:encoding` `Base64`）HTTP `Authorization` 头与 `@scope%2Fname` 元数据 URL。
- **`NpmPackageManifest`**：
  - `parsePackument(allocator, json_bytes)` 解析 registry JSON 的 `dist-tags`、全量 `versions`（提取 `dist.tarball`、`dist.integrity`、SHA-1 `dist.shasum` 转 SRI、`dependencies`/`optionalDependencies`/`peerDependencies` + `peerDependenciesMeta` 可选标记、`os`/`cpu`/`libc`、`bin`/`directories.bin`、`hasInstallScript`），并按 Semver 升序排列；
  - `resolveVersion(range_or_tag, include_prerelease)` 优先检查 `latest` dist-tag 是否满足范围，否则回退至满足 `SemverRange` 的最高版本，或按 dist-tag 直接定位。

### 5. 依赖图与双格式 Lockfile (`lib/pkg/lockfile.cp`)
- **图引擎**：`addPackage`、`addDependency`、`findPackageByName`、`findPackageByNameAndVersion`、`hasCycle`（三色 DFS 环检测）、`topoOrder`（依赖先于依赖方输出，遇环报错 `CyclicDependency`）、`computeMetaHash`。
- **文本格式 (`cap.lock`)**：确定性 JSON 序列化 `toTextJsonAlloc` / `saveTextFile` 与反序列化 `fromTextJson` / `loadTextFile`，完整保存 `workspaces`、`overrides`、`trustedDependencies`、`patchedDependencies`、包列表、SRI `integrity`、`bin` 与依赖边表。
- **二进制格式 (`CPLK0001`)**：68 字节紧凑头部（Magic `"CPLK0001"` + 版本 + 计数 + 32 字节 `meta_hash` + 载荷 `CRC32` 校验和），`fromBinary` 严格校验 Magic 与 CRC32，篡改即返回 `error.LockfileCorrupt`。

### 6. 安全 Tarball 解包引擎 (`lib/pkg/tarball.cp`)
- **前置完整性校验**：调用 `Integrity.verify` 验证 `.tgz` 数据，不匹配即返回 `error.IntegrityCheckFailed`，未提供期望 SRI 时自动计算 SHA-512 SRI。
- **路径穿越防御**：在 `strip_components` 前后双向校验 `isSafeArchivePath`（拦截 `..`、前导 `/`、Windows 盘符、`\`、NUL 字节），并对软链目标执行 `isSafeSymlinkTarget` 深度越界检查，违者立刻拦截并返回 `error.PathTraversalDetected`。
- **解压与构建**：结合 `cap:pack`（`gunzip` / `gzip` / `TarReader` / `TarHeader`）与 `cap:sys`（`mkdirAll` / `writeFile` / `readFile`）实现完整文件级解包与 `TarballBuilder` 打包。

### 7. 模块树链接、`.bin` 链接与包补丁 (`lib/pkg/linker.cp`)
- **`Hoisted` 与 `Isolated` 双策略**：
  - `Hoisted`：扁平落盘至 `<modules_dir>/<pkg_name>`；
  - `Isolated`：pnpm/Bun 风格隔离存储于 `<modules_dir>/.cap/<escaped_name>@<ver>/node_modules/<pkg_name>`（将 `@scope/pkg` 转义为 `@scope+pkg`），直接依赖建立顶层软链，传递依赖在各自 `.cap/<store>/node_modules/` 下精确建立隔离软链。
- **`CopyMethod.Auto`**：优先尝试硬链（`link`）实现零拷贝极速落盘，跨文件系统等场景自动回退 `copyFile`。
- **`BinLinker`**：处理 `File`/`NamedFile`/`Map`/`Dir` 四类可执行条目，创建 `<modules_dir>/.bin/<cmd>` 软链并调用 `chmod(target, 0o755)`。
- **`PackageLinker.applyPatchToDir`**：集成 `cap:patch`（`UnifiedDiff.parse` / `UnifiedDiff.applyFilePatch`），在写入补丁文件前主动 `unlink` 打破硬链以防止污染共享全局缓存，并写入 `.cap-patch-<wyhash>` 幂等标记文件。

### 8. 端到端包管理器 (`lib/pkg/manager.cp`)
- **`PackageManager`**：串联 `readRootPackageJson` → `planInstall`（增量 `cap.lock` 对比与缓存命中统计）→ `install`（递归依赖解析、环检测、缓存检查/Tarball 解包+SRI 校验、Hoisted/Isolated 落盘与传递隔离软链、补丁应用、`.bin` 链接、`cap.lock` & `cap.lockb` 双落盘）。

### 9. Monorepo Workspaces 与 Catalogs (`lib/pkg/workspace.cp`)
- **`WorkspaceGraph` / `WorkspaceMember` / `CatalogMap`**：
  - 支持根目录 `package.json` 与 `cap.json` 中的 `workspaces` 数组形式（`["packages/*", "!packages/ignored"]`）及对象形式（`{ packages, catalog, catalogs }`），结合 `cap:glob` 与 `cap:sys` 扫描工作区成员并自动跳过 `node_modules`/隐藏目录；
  - 完整解析 `workspace:*`、`workspace:^`、`workspace:~`、`workspace:<range>` 为真实本地成员版本；
  - 支持 `catalog:` 与 `catalog:<name>` 版本查表解析；
  - 提供 `hasCycle()` 环检测与 `topoSortMembers()` 成员依赖 DAG 拓扑排序（确保被依赖包优先构建/安装），并支持 `applyToLockfile()` 落盘。

### 10. Registry HTTP 客户端与磁盘元数据缓存 (`lib/pkg/registry_client.cp`)
- **`RegistryHttpClient` / `RegistryCacheEntry` / `RetryPolicy` / `MockRegistryTransport`**：
  - 自动生成标准 HTTP/1.1 请求（`Accept: application/vnd.npm.install-v1+json; q=1.0, application/json; q=0.8, */*`、`If-None-Match`、`Authorization: Bearer/Basic`）；
  - 持久化磁盘元数据缓存（`<cache_dir>/manifests/<hash>.{json,etag,meta}`），支持 `Cache-Control: max-age` TTL 新鲜度短路与 `304 Not Modified` 零流量复用；
  - 针对 `429`/`500`/`502`/`503`/`504` 实现指数退避重试与 `Retry-After` 解析，并与 `NpmPackageManifest.parsePackument` 无缝衔接。

### 11. 零信任生命周期脚本执行器 (`lib/pkg/lifecycle.cp`)
- **`LifecycleRunner` / `TrustedPolicy` / `LifecyclePhase`**：
  - 默认零信任安全模型（`allow_all == false`）：仅允许 `trustedDependencies` 白名单内包运行 `preinstall`/`install`/`postinstall`/`prepare` 脚本，未授权脚本拦截并统计至 `stats.blocked_scripts` 与 `stats.blocked_packages`；
  - 通过 `cap:sys` `PosixSpawn` + `cap:io` `Pipe` 在目标包目录内真实执行脚本，自动前置 `node_modules/.bin` 至 `PATH` 并注入 `npm_package_name`/`npm_package_version`/`npm_lifecycle_event`/`npm_lifecycle_script`/`INIT_CWD`，完整捕获 stdout/stderr/exit_code 与纳秒级耗时。

### 12. 多生态 Lockfile 迁移与依赖图诊断 (`lib/pkg/migration.cp`)
- **`LockfileMigrator`**：一键自动探测并迁移 npm `package-lock.json`（v2/v3，支持嵌套 `node_modules` 与 workspace `link`）、Yarn v1 `yarn.lock`（缩进键值与多 spec 合并）与 pnpm `pnpm-lock.yaml`（v6-v9 YAML 解析为 `ConfValue` AST，支持 `importers`/`packages`/`snapshots`/`overrides`/`patchedDependencies`）至原生 `Lockfile`。
- **`DependencyAnalyzer`**：
  - `why(allocator, lf, pkg_name)`：逆向/正向追踪从根包到目标包的全部依赖路径链与命中版本；
  - `findDuplicates(allocator, lf)`：扫描同名多版本重复安装并统计入度以指导 `dedupe`；
  - `auditIntegrity(allocator, lf)`：审计缺失 SRI 哈希（`Missing`）或使用弱算法 `sha1`（`WeakSha1`）的包。

---

## 三、 验证与测试结果

1. **静态语法与借用检查（`cpc check --brief lib/pkg/*.cp`）**：13 个模块 + 2 个测试文件全部 `0 errors`。
2. **耦合隔离检查**：`rg -n '@import\("bun"\)|std\.posix' lib/pkg/` 无任何匹配（`0` 残留）。
3. **Phase B-1 端到端测试套件（`cap test cap/test/test_pkg_install_deep.cp`）**：**6/6 全部 PASS（100%）**：
   - `cap:pkg core types, strings, slices, behaviors & cap hub` — PASS
   - `cap:pkg DependencyVersion.parse all protocols & DependencyList` — PASS
   - `cap:pkg RegistryConfig (.npmrc), Auth & NpmPackageManifest Packument` — PASS
   - `cap:pkg Lockfile graph, cycle detection, topoOrder & text/binary CPLK0001 round-trip` — PASS
   - `cap:pkg TarballBuilder -> gzip -> SRI verify -> safe extract & path traversal defense` — PASS
   - `cap:pkg E2E PackageManager Hoisted & Isolated install + .bin + patch_install` — PASS
4. **Phase B-3 生产子系统测试套件（`cap test cap/test/test_phase_b3_pkg_production.cp`）**：**4/4 全部 PASS（100%）**：
   - `Phase B-3 (1): Monorepo Workspaces, Catalogs, workspace:*^~ & DAG topoSort` — PASS
   - `Phase B-3 (2): RegistryHttpClient Auth, ETag/304 Disk Cache & RetryPolicy` — PASS
   - `Phase B-3 (3): LifecycleRunner Zero-Trust Policy & PosixSpawn Execution` — PASS
   - `Phase B-3 (4): LockfileMigrator (npm/yarn/pnpm) & DependencyAnalyzer (why/dups/audit)` — PASS

---

## 附加零分配子模块

- **`lib/pkg/name.cp`**（npm 包名与模块说明符解析）：`PackageSpecifier`、`isRelativeSpecifier`、`isBareSpecifier`、`isValidPackageName`、`parsePackageSpecifier`
- **`lib/pkg/npmrc.cp`**（`.npmrc` 配置与作用域 Registry 解析）：`DEFAULT_NPM_REGISTRY`、`NpmrcEntry`、`NpmrcIter`、`getNpmrcValue`、`getNpmrcBool`、`resolveRegistryForScope`、`findRegistrySetting`
- **`lib/pkg/exports.cp`**（`package.json` `exports` 条件导出解析）：`ExportTargetKind`、`ExportConditionRule`、`ExportSubpathEntry`、`ResolveStatus`、`ResolveResult`、`ExportsMap`
- **`lib/pkg/matcher.cp`**（包模式通配符匹配器）：`MatcherBehavior`、`PackagePattern`、`PackageMatcher`、`wildcardMatch`
- **`lib/pkg/spdx.cp`**（SPDX 2.3 许可证表达式词法解析与校验）：`SpdxTokenKind`、`SpdxToken`、`SpdxLexer`、`isValidSpdxExpression`、`SpdxIdIter`、`isCommonPermissive`
- **`lib/pkg/postinstall_optimizer.cp`**（吸收自 `/data/dev/bun/src/install/postinstall_optimizer.cp`，零分配 Postinstall 脚本优化器与原生二进制替代包匹配）：`MAX_DYNAMIC_OPTIMIZER_RULES`、`PostinstallMode`、`SemverTriple`、`PostinstallPkgInfo`、`DynamicOptimizerRule`、`PostinstallOptimizerConfig`、`findNativeBinlinkReplacement`（内置 `esbuild` / `@anthropic-ai/claude-code` 原生二进制直连替换与 `sharp >= 0.33.0` 版本门限跳过，Windows 平台自动禁用 Unix binlink 替换，支持 `configureNativeDependencies` / `configureIgnoreScripts` 动态覆盖与根安装树 `tree_id == 0` 约束）
- **`lib/pkg/overrides.cp`**（吸收自 `/data/dev/bun/src/install/lockfile/OverrideMap.cp`，零堆分配 npm `overrides` 与 Yarn `resolutions` 依赖版本覆写映射）：`MAX_OVERRIDE_RULES`、`OverrideError`、`OverrideSourceKind`、`RootDepSpec`、`OverrideRule`、`normalizeOverrideKey`、`OverrideMap`（支持 `$pkg` 根依赖版本解引用、npm 单层 `{ ".": "ver" }` 语法、Yarn `**/` 通配前缀剥离、多级嵌套与 `patch:` 拦截、按包名字典序确定性排序与 JSON 序列化）
- **`lib/pkg/update_request.cp`**（吸收自 `/data/dev/bun/src/install/PackageManager/UpdateRequest.cp`，零堆分配 CLI `add/remove/update/link/unlink` 位置参数说明符解析与去重列表）：`MAX_UPDATE_REQUESTS`、`UpdateRequestError`、`PackageSubcommand`、`UpdateSpecKind`、`UpdateRequest`、`UpdateRequestList`（支持 scoped/unscoped semver、dist-tag、`npm:` 别名、`link:` 自动补全、`file:`/`workspace:`/tarball/git/GitHub 简写与批量去重）
- **`lib/pkg/yarn_lock_printer.cp`**（吸收自 `/data/dev/bun/src/install/lockfile/printer/Yarn.cp`，零堆分配 Yarn Lockfile v1 确定性序列化打印器）：`MAX_YARN_LOCK_PACKAGES`、`YarnPrintError`、`YarnLockDepKind`、`YarnLockDep`、`YarnLockPackage`、`mustQuoteYarnName`、`mustQuoteYarnKey`、`YarnLockPrinter`（支持 YAML 安全键名引号判定、多版本请求去重合并、`integrity` 无引号输出、依赖分组与字典序稳定排序）

### Yarn Lockfile v1 打印示例（`yarn_lock_printer.cp`）

```typescript
import { YarnLockPrinter, YarnLockPackage } from "cap:pkg";

let printer = YarnLockPrinter.init("deadbeef");
const specs = ["^4.17.0", "4.17.21"];
printer.addPackage(YarnLockPackage{
    name: "lodash",
    version: "4.17.21",
    resolved_url: "https://registry.npmjs.org/lodash/-/lodash-4.17.21.tgz",
    integrity: "sha512-abc",
    requested_specs: specs[0..],
})?;
let buf: [512]u8 = undefined;
const out = printer.printToBuffer(buf[0..])?;
```

### CLI `UpdateRequest` 批量解析与去重示例（`update_request.cp`）

```typescript
import { UpdateRequestList, PackageSubcommand } from "cap:pkg";

const args = ["lodash@^4.17.21", "@types/node@22.0.0", "lodash@latest"];
const list = UpdateRequestList.parseAll(args[0..], .add)?;
// list.count() == 2 (duplicate "lodash" deduplicated, first occurrence wins)
```

### `overrides` / `resolutions` 依赖覆写示例（`overrides.cp`）

```typescript
import { OverrideMap, RootDepSpec } from "cap:pkg";

const root_deps = [
    RootDepSpec{ name: "react", spec: "^19.0.0" },
];
let map = OverrideMap.init();
map.parseOverridesObject("{\"lodash\":\"4.17.21\",\"react\":\"$react\",\"chalk\":{\".\":\"5.3.0\"}}", root_deps[0..])?;
map.sort();
// map.get("react") == "^19.0.0", map.get("chalk") == "5.3.0"
```

### 零分配 Monorepo `catalog:` 协议解析与静态映射（`catalog.cp`）

> 归并自原 `lib/catalog/`（Bun `src/install/lockfile/CatalogMap.cp`）；为避免与 `workspace.cp` 既有堆分配 `CatalogMap`/`CatalogEntry` 重名冲突，主包以 `StaticCatalogMap` / `CatalogProtocolEntry` 别名导出，协议工具函数保持原名。

- 核心导出：`CatalogError`, `CatalogRef`, `CatalogProtocolEntry`, `StaticCatalogMap`, `isCatalogProtocol`, `isValidCatalogName`, `parseCatalogRef`, `formatCatalogRef`。

```typescript
import { StaticCatalogMap, parseCatalogRef, isCatalogProtocol } from "cap:pkg";

const ref = parseCatalogRef("catalog:react19")?;
// ref.is_default == false, ref.catalog_name == "react19"
let map = StaticCatalogMap.init();
map.set("react19", "react", "^19.0.0")?;
// map.resolve("react", "catalog:react19") == "^19.0.0"
```

### 零分配 Monorepo 工作区双向映射与确定性排序（`workspace_map.cp`，吸收自 `/data/dev/bun/src/install/lockfile/Package/WorkspaceMap.cp`）

- 核心导出：`MAX_WORKSPACE_MAP_ENTRIES` (128)、`WorkspaceMapError`（`EmptyPathOrName` / `DuplicateWorkspacePath` / `DuplicateWorkspaceName` / `CapacityExceeded`）、`isIgnoredWorkspacePath`、`hasIgnoredWorkspaceSegment`、`normalizeWorkspacePath`、`WorkspaceMapEntry`、`WorkspaceMap`（`init` / `put` / `getByPath` / `getByName` / `containsPath` / `containsName` / `sortByName` / `at` / `count` / `isEmpty`）。

```typescript
import { WorkspaceMap, hasIgnoredWorkspaceSegment } from "cap:pkg";

let wm = WorkspaceMap.init();
* = wm.put("./packages/core/", "@acme/core", "1.2.0", 0)?;
* = wm.put("apps/web", "web-app", null, 0)?;
wm.sortByName();
// wm.getByPath("packages/core")?.name == "@acme/core"
// hasIgnoredWorkspaceSegment("packages/app/node_modules/pkg") == true
```

### Monorepo `--filter` 包名与路径通配过滤器（`filter.cp`，吸收自 `/data/dev/bun/src/cli/filter_arg.cp`）

- 核心导出：`WorkspaceFilterError`、`WorkspaceFilterKind`（`name` / `path`）、`WorkspaceFilterPattern`、`isIgnoredFilterDir`（跳过隐藏目录与 `node_modules`）、`normalizeFilterPath`、`matchFilterGlob`（支持 `*`、`**`、`?` 与路径段隔离）、`WorkspaceFilter`（`init` / `patternCount` / `matchesPath` / `matches` + RAII `~init()`）。

```typescript
import { WorkspaceFilter } from "cap:pkg";

pub fn demoFilter(allocator: std.mem.Allocator): ! {
    const rules: [2][]const u8 = ["!./", "./packages/*"];
    let wf = WorkspaceFilter.init(allocator, rules[0..], "")?;
    defer wf.deinit();

    * = wf.matches("packages/core", "@acme/core"); // true
    * = wf.matches(".", "root-pkg");               // false（被 !./ 排除）
}
```

### 锁文件 `configVersion`、默认链接器策略与 Isolated Store 路径编码（`config_version.cp`，吸收自 `/data/dev/bun/src/install/ConfigVersion.cp`）

- 常量与类型：`PKG_CONFIG_VERSION_CURRENT` (`1`)、`PKG_ISOLATED_STORE_DIR` (`".bun"`)、`ConfigVersionError`、`LockfileMigrationSource`（`none`/`legacy_bun`/`npm`/`yarn`/`pnpm` -> `defaultConfigVersion()`）、`PkgLinkerStrategy`（`hoisted`/`isolated` + `fromStr`/`asStr`）、`PkgConfigVersion`（`v0`/`v1` + `current`/`fromInt`/`fromFloat`/`defaultLinker`）；
- 零堆路径编码：`formatIsolatedStoreEntry`（`@scope/pkg` + `2.1.0` -> `@scope+pkg@2.1.0`）、`formatIsolatedStorePath`（`node_modules/.bun/@scope+pkg@2.1.0/node_modules/@scope/pkg`），内置路径穿越与控制字符校验。

```typescript
import {
    PkgConfigVersion,
    LockfileMigrationSource,
    formatIsolatedStorePath,
} from "cap:pkg";

const ver = LockfileMigrationSource.pnpm.defaultConfigVersion(); // v1
const linker = ver.defaultLinker(true); // isolated (v1 + workspaces)
let buf: [128]u8 = [0; 128];
const store_path = formatIsolatedStorePath(buf[0..], "@scope/pkg", "2.1.0")?;
// "node_modules/.bun/@scope+pkg@2.1.0/node_modules/@scope/pkg"
* = linker;
* = store_path;
```

### 依赖反向解释与 `why` 树形渲染引擎（`why.cp`，吸收自 `/data/dev/bun/src/cli/why_command.cp`）

- 常量与错误集：`WHY_MAX_PACKAGES` (`64`)、`WHY_MAX_EDGES` (`256`)、`WHY_MAX_DEPTH` (`32`)、`WhyError`；
- 通配与版本匹配：`WhyGlobKind`（`Exact`/`Prefix`/`Suffix`/`Middle`/`Contains`/`Invalid`）、`WhyGlobPattern`（`parse` / `matchesName` / `matchesVersion` / `matches`，支持 `@types/*`、`*-lodash`、`*react*` 与 `@scope/pkg@^18.0.0`）；
- 确定性排序与图引擎：`WhyDepType`（`Prod`/`Peer`/`OptionalPeer`/`Optional`/`Dev`）、`specifierSpecificity`（精确版本 `10` > `workspace:` `9` > `^` `8` > `~` `7` > `<>=` `6` > `.x` `5` > `*` `1`）、`compareWhyDependents`、`WhyOptions`（`top_only` / `max_depth`）、`WhyGraph`（`init` / `addPackage` / `addDependency` / `findMatchingPackages` / `collectDependents` / `renderExplain`，内置环路检测与 `(deeper dependencies hidden)` 截断）。

```typescript
import { WhyGraph, WhyDepType, WhyOptions } from "cap:pkg";

let g = WhyGraph.init();
const app = g.addPackage("my-app", "1.0.0", false)?;
const react = g.addPackage("react", "18.2.0", false)?;
g.addDependency(app, react, "^18.0.0", .Prod)?;

let out: [512]u8 = [0; 512];
const n = g.renderExplain("react", WhyOptions{}, out[0..])?;
// out[0..n] == "react@18.2.0\n  └─ my-app@1.0.0 (requires ^18.0.0)\n"
```

### 生命周期脚本清单、路径裁剪与来源感知信任白名单（`script_list.cp`，吸收自 `/data/dev/bun/src/install/lockfile/Package/Scripts.cp`）

- 常量与类型：`PKG_SCRIPT_SLOT_COUNT` (`6`)、`PkgScriptListError`、`PkgScriptSlotKind`（`Preinstall`/`Install`/`Postinstall`/`Preprepare`/`Prepare`/`Postprepare`）、`PkgScriptFormatMode`（`Completed` `✓` / `Untrusted` `»` / `Info`）、`PkgScriptTrustSource`（`NpmRegistry`/`LocalFile`/`Symlink`/`Git`/`Tarball`）、`isScriptExecutionAllowed`、`formatNodeModulesRelativeCwd`、`PkgScriptList`（`init` / `set` / `get` / `count` / `isEmpty` / `first` / `eql` / `format`）。

```typescript
import { PkgScriptList, PkgScriptTrustSource, isScriptExecutionAllowed } from "cap:pkg";

const explicit: [0][]const u8 = [];
const defaults: [1][]const u8 = ["esbuild"];
// 默认白名单仅对 npm Registry 生效；file:/git: 来源同名包必须显式加入 trustedDependencies
const ok_npm = isScriptExecutionAllowed("esbuild", .NpmRegistry, false, explicit[0..], defaults[0..]); // true
const ok_git = isScriptExecutionAllowed("esbuild", .Git, false, explicit[0..], defaults[0..]);         // false
* = ok_npm;
* = ok_git;
```

---

## 上游 Bun 包管理行为契约与边界注意事项

1. **全局缓存与版本目录去重策略**：
   - 参考 `/data/dev/bun/docs/pm/global-cache.mdx`：缓存目录按 `${name}@${version}` 组织以支持多版本并存；当版本包含超长预发布（pre-release）或构建元数据（`+build`）后缀时通过哈希缩短路径避免超长文件名错误；`RegistryHttpClient` 通过 `If-None-Match` / HTTP `304` 校验磁盘缓存元数据最小化重复下载。
2. **Hoisted vs Isolated 链接策略与 `configVersion` 矩阵（参考 `/data/dev/bun/docs/pm/{isolated-installs,lockfile}.mdx`）**：
   - **默认策略矩阵**：仅当 `configVersion == 1` 且启用 Workspaces 时默认采用 `isolated`（中心存储 `node_modules/.bun/<entry>/node_modules/<pkg>` + 顶层符号链接，Scoped 包在 store 目录中以 `+` 替换 `/`）；`configVersion == 0` 或非 Workspace 项目默认采用 `hoisted`；
   - **锁文件迁移与前向兼容**：新建工程与从 `pnpm-lock.yaml` 迁移默认 `configVersion = 1`，旧版 Bun 锁文件与从 `package-lock.json` / `yarn.lock` 迁移默认 `configVersion = 0` 以保持向后兼容；解析高于当前版本的 `configVersion` 整数时前向兼容钳位至 `current`。
   - 跨文件系统链接时，优先使用硬链接（Linux）/ 拷贝回退，并对 `file:`/`link:` 软链跳过嵌套 `node_modules` 以防止符号链接环路。
3. **零信任生命周期脚本与 `trustedDependencies` 来源隔离（吸收自 `/data/dev/bun/docs/pm/lifecycle.mdx` & `/data/dev/bun/docs/guides/install/trusted.mdx`）**：
   - 默认不执行任意依赖的 `preinstall`/`install`/`postinstall`/`prepare` 脚本（未授权脚本记入 `blocked_scripts`）；`--ignore-scripts`（或 `install.ignoreScripts = true`）具备最高优先级，全局禁用全部安装脚本；
   - 默认内置白名单（Top 500 常见原生扩展包）**仅适用于 npm Registry 来源**，`file:`、`link:`、`git:`、`github:` 等非 Registry 来源必须显式写入根 `package.json` 的 `trustedDependencies` 才允许执行，防止本地路径或外部仓库仿冒知名包名绕过检查；且 `trustedDependencies` 仅授权指定包自身，**不传递**至其子依赖。
4. **Tarball 解包安全防线（`TarballExtractor`）**：
   - 解包前校验 W3C SRI 完整性摘要（`Integrity`），并在写入每个归档条目时拦截 `..` 路径穿越与绝对路径逃逸。
5. **Monorepo `--filter` 匹配与排除约定（吸收自 `/data/dev/bun/docs/pm/filter.mdx`）**：
   - 以 `./` 或 `/` 开头的模式按工作区路径匹配（`./` 仅匹配根 `package.json`，`./packages/**` 递归匹配子目录）；其余模式按 `package.json` 的 `name` 字段匹配（支持 `*` 通配）；
   - 否定规则（`!pkg-c`、`!./`）优先级高于正向规则：当全部规则均为否定规则时，默认先选中全部工作区包再执行排除；当正负规则混用时，须命中至少一条正向规则且不命中任何否定规则。
6. **`bun why` 依赖反向解释输出契约与选项（吸收自 `/data/dev/bun/docs/pm/cli/why.mdx`）**：
   - 默认展示从目标包到顶层依赖的完整嵌套依赖树，缩进符号明确区分中间节点（`├─` / `│`）与末位节点（`└─`），并在节点前标注依赖类型（`dev`、`peer`、`optional`、`optional peer`，生产依赖省略前缀）与 `(requires <spec>)` 版本约束；
   - `--top`（`WhyOptions{ top_only: true }`）仅展示直接顶层依赖方（等效 `max_depth = 1`）；`--depth <N>` 限制最大递归深度并在超深子树位置输出 `(deeper dependencies hidden)`；
   - 查询参数支持单通配符模式（如 `@types/*`、`*-lodash`、`*react*`）以及 `@version` 后缀（如 `@types/react@^18.0.0`），同层多个依赖方按「版本说明符精确度降序 → 依赖类型优先级降序 → 包名字典序升序」确定性排列。
7. **安装差异树渲染与日志级别约定（`InstallTreePrinter`，吸收自 `/data/dev/bun/src/install/lockfile/printer/tree_printer.cp` 与 `/data/dev/bun/docs/pm/cli/{install,pm}.mdx`）**：
   - **三档日志级别（`TreePrintLogLevel`）**：
     - `Silent`（对应 `bun install --silent`）：静默输出 0 字节；
     - `Default`：仅渲染当前工作区（默认 root workspace）的变更摘要；
     - `Verbose`（对应 `bun install --verbose`）：先输出根工作区变更，再逐个按 `<workspace>:` 分段标题展开所有子工作区的升级与新增依赖。
   - **双通道渲染与去重**：同一工作区内先集中输出版本升级项（ANSI 模式为 `↑ pkg old → new`，纯文本为 `^ pkg old -> new`），再空行分隔输出新增安装包（`+ pkg@ver`，若本地缓存存在更新版本则追加 `(vX.Y.Z available)`），并对同一工作区下重复解析的同名依赖自动去重，同时跳过被 `--omit=dev` 等策略禁用的依赖边。
   - **显式请求与二进制清单**：通过 `addUpdateRequest` 注册的显式安装目标（如 `bun add <pkg>`）会从常规 `+` 列表中剔除，改为在末尾统一输出 `installed <pkg>@<ver>`；若包携带命名可执行文件（`File`/`NamedFile`/`Map`），自动展开 `with binaries:` 与逐行 ` - <bin>` 清单。若任一显式请求标记 `failed = true`，渲染器立即中止并返回 `TreePrintError.UpdateFailed`。

### 安装差异树渲染示例（`InstallTreePrinter`）

```cp
import {
    InstallTreePrinter,
    TreeDepEdge,
    TreePackageBinKind,
    TreePrintOptions,
    TreePrintLogLevel,
} from "cap:pkg";

let printer = InstallTreePrinter.init();
const root_id = printer.addPackage("my-app", "1.0.0", true, true)?;
const react_id = printer.addPackage("react", "19.1.1", false, true)?;
const tsc_id = printer.addPackage("typescript", "5.8.2", false, true)?;
printer.setPackageLaterVersion(react_id, "19.2.0")?;
printer.addPackageBin(tsc_id, TreePackageBinKind.Map, "tsc")?;
printer.addPackageBin(tsc_id, TreePackageBinKind.Map, "tsserver")?;

* = printer.addDep(TreeDepEdge{
    owner_pkg_id: root_id,
    dep_name: "react",
    target_pkg_id: react_id,
    original_version: "18.3.1",
})?;
* = printer.addDep(TreeDepEdge{
    owner_pkg_id: root_id,
    dep_name: "typescript",
    target_pkg_id: tsc_id,
})?;
* = printer.addUpdateRequest("typescript", false)?;

let buf: [1024]u8 = [0; 1024];
const stats = printer.render(
    TreePrintOptions{ ansi_colors: false, log_level: TreePrintLogLevel.Default },
    buf[0..],
)?;
// 输出:
// \n^ react 18.3.1 -> 19.1.1\n\ninstalled typescript@5.8.2 with binaries:\n - tsc\n - tsserver\n\n
* = stats;
```

8. **包可执行文件 Shebang、`.bunx` 垫片编解码与 `bunx` 运行契约（`lib/pkg/bin_shim.cp`，吸收自 `/data/dev/bun/src/install/windows-shim/BinLinkingShim.cp` 与 `/data/dev/bun/docs/pm/bunx.mdx`）**：
   - **Shebang 尊重与 `--bun` 覆盖**：默认尊重包可执行文件头部的 `#!` Shebang（如 `#!/usr/bin/env node` 标记 `is_node = true` 并在缺少 `node` 时可自动回退至 `bun`）；无 Shebang 时按扩展名回退启动器（`.js`/`.mjs`/`.cjs`/`.ts`/`.tsx`/`.sh` → `bun run`，`.cmd`/`.bat` → `cmd /c`，`.ps1` → `powershell -ExecutionPolicy Bypass -File`）。使用 `bunx --bun <cli>` 时 `--bun` 必须位于可执行文件名之前（其后的标志透传给目标 CLI）；当包名与可执行名不同时使用 `-p <pkg>` / `--package <pkg>`。
   - **`.bunx` 二进制布局与抗损坏校验**：尾部 16 位 `BinShimFlags` 携带 `v1=5474..v5=5478` 非零魔数版本标签（防偏移错位读取），配合 `bin_path_byte_len` / `arg_byte_len` 偶数字节长度与边界校验实现零分配安全解码。

### `.bunx` 垫片编解码示例（`BinLinkingShim`）

```cp
import { BinLinkingShim, BinShimShebang, decodeShimUtf16LeToUtf8 } from "cap:pkg";

const sb = BinShimShebang.parse("#!/usr/bin/env node\nconsole.log(1);\n", "cowsay/cli.js");
const shim = BinLinkingShim{ bin_path: "cowsay/cli.js", shebang: sb };
let wire: [256]u8 = [0; 256];
const n = shim.encodeInto(wire[0..])?;
const decoded = BinLinkingShim.decode(wire[0..n])?;
let path_utf8: [64]u8 = [0; 64];
const pn = decodeShimUtf16LeToUtf8(decoded.bin_path_utf16_bytes, path_utf8[0..])?;
// path_utf8[0..pn] == "cowsay/cli.js", decoded.flags.is_node == true
* = pn;
```

9. **安装/发布运行选项、全局虚拟存储与供应链冷却期门禁（`lib/pkg/install_options.cp`，吸收自 `/data/dev/bun/src/install/PackageManager/PackageManagerOptions.cp`、`/data/dev/bun/docs/pm/cli/publish.mdx` 与 `/data/dev/bun/docs/pm/global-store.mdx`）**：
   - **`bun publish` 规范校验**：默认 dist-tag 为 `latest`；非 scoped 包（不以 `@` 开头）始终公开，尝试以 `--access restricted` 发布会被 `PkgPublishConfig.validateForPackage` 拒绝（`UnscopedRestrictedPublish`）；CI 重跑场景建议启用 `--tolerate-republish` 以幂等退出码 0 忽略已存在版本；直接传入预打包 `.tgz` 路径发布时不执行 `prepublishOnly`/`prepack` 等生命周期脚本。
   - **全局虚拟存储（Global Virtual Store）**：默认关闭且**仅在 `isolated` 链接器下生效**（`enableGlobalVirtualStore(true)`，hoisted 下自动拒绝或禁用）；热安装路径将逐文件 `clonefileat` 替换为单次 `symlink()` 指向共享 `<cache>/links/` 目录，显著缩短 CI 温缓存安装耗时并压缩单项目 `node_modules` 体积。
   - **供应链冷却期门禁（`--minimum-release-age`）**：通过 `isReleaseAgeSatisfied(published_unix_sec, now_unix_sec)` 拒绝发布未满设定秒数的新版本，缓解供应链恶意抢发风险。

### 安装与发布选项配置示例（`PkgInstallOptions`）

```cp
import { PkgInstallOptions, PkgPublishAccess } from "cap:pkg";

let opts = PkgInstallOptions.init();
opts.applyFrozenLockfile();
opts.applyProduction();
opts.enableGlobalVirtualStore(true)?;
opts.minimum_release_age_secs = 86400; // 24h 冷却期
opts.publish.access = PkgPublishAccess.Restricted;
opts.publish.validateForPackage("@acme/sdk")?;
```

10. **安装前供应链安全扫描协议与决策引擎（`lib/pkg/security_scanner.cp`，吸收自 `/data/dev/bun/src/install/PackageManager/security_scanner.cp` & `/data/dev/bun/docs/pm/security-scanner-api.mdx`）**：
    - **双级严重度与运行环境决策矩阵（`SecurityInstallDecision`）**：
      - `fatal`：无论交互终端或 CI，一律立即中止安装（`cancel_fatal`）；
      - `warn`：交互式非 CI 终端提示 `[y/N]` 确认（`prompt_user`，配合 `parseWarningConfirmation` 仅放行显式 `y`/`Y` 单字符输入），在 CI 或非 TTY 管道环境下自动安全取消（`cancel_non_interactive_warn`）；
      - 配置安全扫描器后，包管理器应自动禁用 `auto-install` 以强化供应链防护。
    - **协议版本与严格 JSON 通告解析（`parseSecurityAdvisoriesJson`）**：
      - `validateScannerProtocolVersion("1")` 严格校验 Bun Security Scanner API v1 协议；
      - 零堆分配解析扫描器返回的顶层数组或 `{"advisories":[...]}` 结构，强制校验 `package` 非空字符串、`level` 仅允许 `"fatal"`/`"warn"`、`description`/`url` 仅允许字符串或 `null`，拦截畸形字段与类型注入；
      - `formatScanRequestJson` 负责将待扫描包清单安全转义为 JSON 请求，`formatReport` / `formatSummaryLine` 渲染含依赖链（`via a > b > pkg` 或 `(direct dependency)`）与单复数精确统计的可读报告。

### 安全扫描通告解析与安装决策示例（`SecurityScanResults`）

```cp
import {
    SecurityInstallDecision,
    parseSecurityAdvisoriesJson,
    validateScannerProtocolVersion,
    parseWarningConfirmation,
} from "cap:pkg";

validateScannerProtocolVersion("1")?;
const raw = "[{\"package\":\"left-pad\",\"level\":\"warn\",\"description\":\"Deprecated\",\"url\":\"https://example.com/adv/1\"}]";
const res = parseSecurityAdvisoriesJson(raw, "@acme/security-scanner")?;
const ci_decision = res.evaluateInstallDecision(false, true);  // cancel_non_interactive_warn
const tty_decision = res.evaluateInstallDecision(true, false); // prompt_user
const ok = parseWarningConfirmation("y\n");                    // true
let summary: [64]u8 = [0; 64];
const sn = res.formatSummaryLine(summary[0..])?;               // "1 advisory (1 warning)"
* = ci_decision;
* = tty_decision;
* = ok;
* = sn;
```

11. **`bun patch` 持久化补丁定位、命名规范与全局缓存隔离标签（`lib/pkg/patch_spec.cp`，吸收自 `/data/dev/bun/src/install/PackageManager/patchPackage.cp`、`/data/dev/bun/src/install/patch_install.cp` & `/data/dev/bun/docs/pm/cli/patch.mdx`）**：
    - **三步持久化补丁工作流（参考 `/data/dev/bun/docs/pm/cli/patch.mdx`）**：
      1. `bun patch <pkg>`：必须先执行准备步骤以创建与 Global Cache 解除硬链接/软链接的独立副本，避免直接修改污染全局共享缓存；
      2. 本地编辑 `node_modules/<pkg>` 并验证；
      3. `bun patch --commit <pkg|path>`（支持 pnpm 兼容别名 `bun patch-commit` 及自定义 `--patches-dir=<dir>`）：生成 `.patch` 文件并写入 `package.json` `"patchedDependencies"`。
    - **三种定位入参解析（`PatchTargetArg.parse`）**：统一解析纯包名（`react`、`@scope/pkg`）、精确版本（`react@17.0.2`）与目录路径（`node_modules/a/node_modules/@scope/pkg`）。
    - **补丁文件名、目录穿越防护与 `__buntag_<16hex>` 缓存隔离**：
      - `PatchCommitSpec.formatPatchFilePath()` 自动将作用域包 `/` 映射为 `+`（如 `patches/@scope+pkg@1.2.3.patch`），`normalizePatchesDir()` 拦截绝对路径与 `..` 穿越；
      - `formatPatchCacheSubpath()` / `parsePatchCacheHash()` 配合 `computePatchContentHash()` 为已打补丁的包生成 `__buntag_<16hex>` 隔离缓存目录名，确保同一原包不同补丁互不污染且相同补丁跨项目共享 Global Cache。

### 持久化补丁定位与缓存标签示例（`PatchTargetArg` & `PatchCommitSpec`）

```cp
import {
    PatchTargetArg,
    PatchCommitSpec,
    computePatchContentHash,
    formatPatchCacheSubpath,
    parsePatchCacheHash,
} from "cap:pkg";

const arg = PatchTargetArg.parse("node_modules/@acme/ui")?;
const spec = PatchCommitSpec{
    package_name: arg.package_name,
    version: "2.1.0",
    patches_dir: "patches",
};
let path_buf: [128]u8 = [0; 128];
const pn = spec.formatPatchFilePath(path_buf[0..])?; // "patches/@acme+ui@2.1.0.patch"

const h = computePatchContentHash("--- a/index.js\n+++ b/index.js\n@@ -1 +1 @@\n-a\n+b\n");
let cache_buf: [128]u8 = [0; 128];
const cn = formatPatchCacheSubpath("@acme/ui@2.1.0", h, cache_buf[0..])?;
const roundtrip_h = parsePatchCacheHash(cache_buf[0..cn])?; // == h
* = pn;
* = roundtrip_h;
```

12. **严格隔离安装存储路径编码、Peer 哈希与全局虚拟存储闭包图（`lib/pkg/isolated_store.cp`，吸收自 `/data/dev/bun/src/install/isolated_install/Store.cp` & `/data/dev/bun/docs/pm/isolated-installs.mdx`）**：
    - **两层隔离布局防幽灵依赖（参考 `/data/dev/bun/docs/pm/isolated-installs.mdx`）**：
      - 实际包内容落盘于 `node_modules/.bun/<store_key>/node_modules/<pkg_name>`（`formatIsolatedStoreDir`），顶层 `node_modules` 仅创建指向隔离存储的相对软链接（`formatIsolatedSymlinkTarget`，作用域包自动补 `../.bun/...`）；
      - 新建 Workspace 工程（`configVersion = 1`）默认启用 `isolated` 链接器；Workspace 包直接软链至源码目录而不进入中央存储。
    - **Peer 依赖哈希与去重**：`computePeerSetHash()` 对 Peer 依赖集合执行顺序无关去重哈希，相同 `(kind, name, resolution, peer_hash)` 在 `IsolatedStoreGraph.addOrFindEntry()` 中自动去重共享。
    - **Global Virtual Store 闭包哈希（`formatGlobalStorePath`）**：`IsolatedStoreGraph.computeAllEntryHashes()` 迭代计算每个 `Registry` 条目的 `(store_key + sorted resolved dependency hashes)` 闭包哈希（`<store_key>-<16hex>`），使跨项目相同依赖闭包共享 `<cache>/links/` 条目，而 `Root`/`Workspace`/`Folder`/`Patched` 条目自动排除在全局共享之外。

### 隔离存储图与闭包哈希示例（`IsolatedStoreGraph`）

```cp
import {
    IsolatedEntryKind,
    IsolatedPeerRef,
    IsolatedStoreGraph,
    computePeerSetHash,
    formatIsolatedStoreDir,
    formatGlobalStorePath,
} from "cap:pkg";

let g = IsolatedStoreGraph.init();
const peers: [1]IsolatedPeerRef = [IsolatedPeerRef{ name: "react", version: "19.0.0" }];
const ph = computePeerSetHash(peers[0..])?;
const react_id = g.addOrFindEntry(IsolatedEntryKind.Registry, "react", "19.0.0", 0, false)?;
const ui_id = g.addOrFindEntry(IsolatedEntryKind.Registry, "@acme/ui", "2.0.0", ph, false)?;
g.addDependency(ui_id, react_id)?;
g.computeAllEntryHashes()?;

if (ui := g.entryAt(ui_id)) {
    let dir_buf: [160]u8 = [0; 160];
    let g_buf: [160]u8 = [0; 160];
    const dn = formatIsolatedStoreDir(ui, dir_buf[0..])?;
    const gn = formatGlobalStorePath(ui, g_buf[0..])?;
    * = dn;
    * = gn;
}
```

13. **Git / GitHub 依赖规范解析与非交互式 `.gitconfig` 防挂起策略（`lib/pkg/repository.cp`，吸收自 `/data/dev/bun/src/install/repository.cp` & `/data/dev/bun/docs/pm/cli/add.mdx`）**：
    - **多格式 Git 依赖解析（`GitRepoSpec`）**：统一支持 `github:owner/repo#ref`、`gitlab:`、`bitbucket:`、`owner/repo` 简写、`git+https://...#tag` 与 SCP 风格（`git@github.com:owner/repo.git`）；`formatCanonical` 与 `formatStorePath` 自动为 SCP 路径补全 `ssh://` / `ssh++` 并从 `<prefix>-<sha>` 中提取提交哈希尾段。
    - **防 CI 挂起的 `.gitconfig` 探测（`parseGitConfigFlags` & `resolveNonInteractiveGitEnv`）**：轻量扫描用户 `.gitconfig` 中的 `[core]` `askpass`/`sshCommand` 及全局 `core.askpass`/`core.sshCommand`；若未配置且未设置环境变量，自动推导注入 `GIT_ASKPASS=echo` 与 `GIT_SSH_COMMAND="ssh -oStrictHostKeyChecking=accept-new"`，防止子进程交互提示密码或确认主机密钥导致自动化安装挂起。

### Git 依赖规范与防挂起环境推导示例（`GitRepoSpec`）

```cp
import {
    GitRepoSpec,
    parseGitConfigFlags,
    resolveNonInteractiveGitEnv,
} from "cap:pkg";

const spec = GitRepoSpec.parseAny("github:oven-sh/bun#v1.3.0")?;
let buf: [128]u8 = [0; 128];
const n = spec.formatCanonical("github:", buf[0..])?; // "github:oven-sh/bun#v1.3.0"

const cfg = parseGitConfigFlags("[core]\n  editor = vim\n");
const env = resolveNonInteractiveGitEnv(cfg, false, false);
// env.set_askpass == true ("echo"), env.set_ssh_command == true ("ssh -oStrictHostKeyChecking=accept-new")
* = n;
* = env;
```

14. **隔离安装符号链接对账状态机与相对路径规划器（`lib/pkg/isolated_symlink.cp`，吸收自 `/data/dev/bun/src/install/isolated_install/Symlinker.cp` & `/data/dev/bun/docs/pm/isolated-installs.mdx`）**：
    - **双层隔离 `node_modules/.bun/` 链接模型**：顶层 `node_modules/<pkg>`（及作用域 `node_modules/@scope/<pkg>`）均通过相对符号链接指向中央隔离存储 `node_modules/.bun/<pkg>@<ver>/node_modules/<pkg>`（Workspace 包直接软链至其源码目录），彻底杜绝幽灵依赖（phantom dependencies）；`computeRelativeSymlinkTarget` 零分配自动处理作用域层深（`../.bun/...`）、跨 Workspace 路径（`../../core`）与 Windows 反斜杠规范化。
    - **非破坏性 `bun patch <pkg>` 工作区保护与跨平台对账（`reconcileIsolatedSymlink`）**：
      - `ExpectExisting`（增量安装）：当目标路径为实体目录（`RealDirectory`）时返回 `PreservePatchedDir`，严禁删除用户正在 `bun patch <pkg>` 编辑中的独立副本；当目标为符号链接时自动剥离 libuv Windows Junction 附加的尾斜杠（`stripTrailingSlash`），Windows 平台下同时比对 `fallback_junction_target` 绝对路径，目标不一致时 POSIX 返回 `UnlinkAndCreateSymlink`、Windows 返回 `RmdirAndCreateSymlink`；
      - `ExpectMissing`（冷安装）与 `IgnoreFailure`（非关键链接）严格对齐上游策略；`IsolatedLinkPlan` 支持批量记录并统计 `noop_count` / `create_count` / `replace_count` / `preserved_patch_count`。

### 隔离安装符号链接对账与相对目标计算示例（`lib/pkg/isolated_symlink.cp`）

```cp
import {
    IsolatedLinkStrategy,
    IsolatedLinkPlatform,
    IsolatedDestEntryKind,
    IsolatedLinkAction,
    computeRelativeSymlinkTarget,
    IsolatedLinkPlan,
} from "cap:pkg";

let rel_buf: [160]u8 = [0; 160];
const rn = computeRelativeSymlinkTarget(
    "node_modules/@scope",
    "node_modules/.bun/@scope+ui@2.0.0/node_modules/@scope/ui",
    rel_buf[0..],
)?;
// rel_buf[0..rn] == "../.bun/@scope+ui@2.0.0/node_modules/@scope/ui"

let plan = IsolatedLinkPlan.init(IsolatedLinkPlatform.Posix);
// 保护正在 bun patch 编辑中的实体目录
const a1 = plan.planLink(
    IsolatedLinkStrategy.ExpectExisting,
    "node_modules/lodash",
    IsolatedDestEntryKind.RealDirectory,
    "",
    ".bun/lodash@4.17.21/node_modules/lodash",
    "",
)?;
// a1 == IsolatedLinkAction.PreservePatchedDir
const st = plan.stats(); // st.preserved_patch_count == 1
* = rn;
* = a1;
* = st;
```

15. **包清单内存/磁盘缓存状态机与全局缓存路径策略（`lib/pkg/manifest_cache.cp`，吸收自 `/data/dev/bun/src/install/PackageManifestMap.cp` & `/data/dev/bun/docs/pm/global-cache.mdx`）**：
    - **三态清单缓存与扩展清单自动降级（`PackageManifestCache`）**：
      - 维护 `Fresh` / `Expired` / `NotFound` 三种槽位状态：命中 `NotFound` 负缓存直接短路避免重复磁盘探测；当需要完整扩展元数据（`needs_extended_manifest = true`）而缓存仅含精简 corgi 清单（`has_extended_manifest = false`）时，自动将条目从 `Fresh` 降级为 `Expired`（`demoted_for_extended = true`），并支持 `allow_expired = true` 保留 ETag 用于 `If-None-Match` 条件重验；
      - `ingestDiskManifest` 严格遵循 `Cache-Control` 过期时间（`public_max_age > now_sec`）与 `disableManifest`（`enable_manifest_cache = false`）开关。
    - **全局缓存子目录名与复制后端解析（`formatGlobalCacheEntryDir` & `parseInstallCopyBackend`）**：
      - 全局缓存默认位于 `~/.bun/install/cache/<name>@<version>`；若版本含 pre-release（`-`）或 build（`+`）后缀，自动将后缀替换为确定性 8 位十六进制哈希以规避深路径过长错误，并拦截 `..` 路径穿越；
      - `parseInstallCopyBackend` 支持 `hardlink`（Linux/Windows 默认）、`clonefile`（macOS CoW 默认）、`clonefile_each_dir`、`copyfile` 与 `symlink`。

### 清单缓存状态机与全局缓存路径示例（`lib/pkg/manifest_cache.cp`）

```cp
import {
    ManifestCacheBehavior,
    ManifestLookupOutcome,
    PackageManifestCache,
    formatGlobalCacheEntryDir,
} from "cap:pkg";

let cache = PackageManifestCache.init(true, true);
* = cache.insertFresh("react", "\"etag-1\"", "19.0.0", 2_000_000, false)?;

// 普通安装命中 Fresh 缓存
const r1 = cache.lookup("react", ManifestCacheBehavior.LoadFromMemoryFallbackToDisk, true, false);
// r1.outcome == ManifestLookupOutcome.HitFresh

// 需要扩展清单时自动降级为 Expired 并保留 ETag 供条件请求
const r2 = cache.lookup("react", ManifestCacheBehavior.LoadFromMemoryFallbackToDisk, true, true);
// r2.outcome == ManifestLookupOutcome.HitExpired, r2.demoted_for_extended == true

let dir_buf: [64]u8 = [0; 64];
const dn = formatGlobalCacheEntryDir("react", "19.0.0-rc.1", dir_buf[0..])?;
// dir_buf[0..dn] == "react@19.0.0-<8hex>"
* = r1;
* = r2;
* = dn;
```

16. **本地目录 / 软链 / Workspace 解析与发布版本替换（`lib/pkg/folder_resolver.cp`，吸收自 `/data/dev/bun/src/install/resolvers/folder_resolver.cp` & `/data/dev/bun/docs/pm/workspaces.mdx`）**：
    - **`package.json` 路径规范化与去重缓存（`normalizeFolderPackageJsonPath` / `FolderResolverCache`）**：
      - 统一解析 `file:`、`link:`、`workspace:`、全局软链（`GlobalLink`）与缓存目录（`CacheFolder`）路径，自动剥离协议前缀、统一 POSIX `/` 分隔符、折叠 `.`/`..` 段并同步输出绝对 `package.json` 路径与相对项目根的目录路径；
      - `shouldCheckBundledDependencies` 遵循 Bun 契约：`Folder`/`Symlink`/`CacheFolder` 检查 `bundledDependencies`，`Workspace` 跳过；`formatWorkspaceSearchPath` 自动为非 `.`/`/` 相对路径前置 `./` 并支持 JSON 转义引号输出。
    - **发布阶段 `workspace:` 协议版本替换（`resolveWorkspacePublishVersion`）**：
      - 严格对齐 `docs/pm/workspaces.mdx`：`workspace:*` -> `<version>`、`workspace:^` -> `^<version>`、`workspace:~` -> `~<version>`，显式版本（如 `workspace:1.0.2`）优先覆盖当前 `package.json` 版本号。

### Workspace 路径规范化与发布版本替换示例（`lib/pkg/folder_resolver.cp`）

```cp
import {
    FolderResolverKind,
    normalizeFolderPackageJsonPath,
    resolveWorkspacePublishVersion,
    FolderResolverCache,
} from "cap:pkg";

let abs_buf: [160]u8 = [0; 160];
let rel_buf: [128]u8 = [0; 128];
const lens = normalizeFolderPackageJsonPath(
    FolderResolverKind.Workspace,
    "/repo",
    "",
    "workspace:packages/pkg-a/",
    abs_buf[0..],
    rel_buf[0..],
)?;
// abs_buf[0..lens.abs_len] == "/repo/packages/pkg-a/package.json"
// rel_buf[0..lens.rel_len] == "packages/pkg-a"

let ver_buf: [32]u8 = [0; 32];
const vn = resolveWorkspacePublishVersion("workspace:^", "1.0.1", ver_buf[0..])?;
// ver_buf[0..vn] == "^1.0.1"

let cache = FolderResolverCache.init();
* = cache.putResolved(rel_buf[0..lens.rel_len], 7, true)?;
* = vn;
```

### Monorepo Workspace `package.json` 缓存与缩进/JSONC 探测（`lib/pkg/workspace_pkg_cache.cp`）

- 吸收自 `/data/dev/bun/src/install/PackageManager/WorkspacePackageJSONCache.cp` 与 `/data/dev/bun/docs/pm/workspaces.mdx`；
- 导出：`WORKSPACE_PKG_CACHE_MAX_ENTRIES` (`32`)、`WORKSPACE_PKG_PATH_MAX_LEN` (`256`)、`WorkspacePkgCacheError`、`JsonIndentation`、`WorkspacePkgJsonOptions`、`WorkspacePkgJsonSummary`、`WorkspacePkgCacheEntry`、`WorkspacePkgCacheStats`、`isAbsoluteWorkspacePath`、`normalizeWorkspacePkgJsonPath`、`guessJsonIndentation`、`analyzeWorkspacePackageJson`、`WorkspacePkgJsonCache`；
- **最佳实践与边界约束**：
  - 缓存键强制为绝对路径（POSIX `/` 或 Windows `X:\` / `X:/`），并自动将 Windows `\` 规范为 POSIX `/` 且折叠冗余斜杠，确保跨平台路径等价命中；
  - 默认支持 JSONC 注释（`//`、`/* */`）与尾逗号校验，并可通过 `WorkspacePkgJsonOptions` 切换严格 JSON 模式；
  - `guessJsonIndentation` 自动识别 Tab 或 1..8 空格缩进宽度，保障写回 `package.json` 时零格式破坏；
  - 解析失败时递增 `parse_errors` 且**绝不污染缓存**（严格对齐 Bun 失败即 `map.remove` 回滚语义）。

```cp
import {
    WorkspacePkgJsonCache,
    WorkspacePkgJsonOptions,
} from "cap:pkg";

let cache = WorkspacePkgJsonCache.init();
const pkg_src = "{\n  // workspace package\n  \"name\": \"pkg-a\",\n  \"version\": \"1.0.0\",\n}";
const e1 = cache.getOrInsert("C:\\repo\\packages\\pkg-a\\package.json", pkg_src, WorkspacePkgJsonOptions{})?;
// 跨斜杠写法命中同一规范化键 "C:/repo/packages/pkg-a/package.json"
const e2 = cache.getOrInsert("C:/repo/packages/pkg-a/package.json", pkg_src, WorkspacePkgJsonOptions{})?;
// e1.indentation.width == 2, e1.has_comments == true, e1.has_trailing_comma == true, e2.hit_count == 1
* = e1;
* = e2;
```
