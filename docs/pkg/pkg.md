# `cap:pkg` — CP 工业级包管理核心标准库文档 (`lib/pkg/`)

> **源资产迁移来源**：`/data/dev/bun/src/install/` 与 `/data/dev/bun/src/install_types/`  
> **设计原则**：零 `@import("bun")`、零 `std.posix`、零 JSC 耦合、100% 纯正 CP 语法，复用 `cap:semver`、`cap:pack`、`cap:patch`、`cap:json`、`cap:encoding`、`cap:hash`、`cap:sys` 与 `lib/resolver/{integrity,hosted_git,package_json}.cp`。

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
| [lib/pkg/index.cp](file:///data/dev/cp/lib/pkg/index.cp) | 14 | `install/install.cp` | `cap:pkg` 统一导出入口（重导出全部子模块及 `Integrity`/`IntegrityTag`） |
| **核心库合计** | **4,512** | — | **9 个模块全部 `cpc check` 0 报错** |
| [cap/test/test_pkg_install_deep.cp](file:///data/dev/cp/cap/test/test_pkg_install_deep.cp) | 607 | — | 6 大深度端到端测试套件（6/6 100% PASS） |
| **总代码量** | **5,119** | — | — |

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

---

## 三、 验证与测试结果

1. **静态语法与借用检查（`cpc check --brief lib/pkg/*.cp`）**：9 个文件全部 `0 errors`。
2. **耦合隔离检查**：`rg -n '@import\("bun"\)|std\.posix' lib/pkg/` 无任何匹配（`0` 残留）。
3. **端到端测试套件（`cap test cap/test/test_pkg_install_deep.cp`）**：**6/6 全部 PASS（100%）**：
   - `cap:pkg core types, strings, slices, behaviors & cap hub` — PASS
   - `cap:pkg DependencyVersion.parse all protocols & DependencyList` — PASS
   - `cap:pkg RegistryConfig (.npmrc), Auth & NpmPackageManifest Packument` — PASS
   - `cap:pkg Lockfile graph, cycle detection, topoOrder & text/binary CPLK0001 round-trip` — PASS
   - `cap:pkg TarballBuilder -> gzip -> SRI verify -> safe extract & path traversal defense` — PASS
   - `cap:pkg E2E PackageManager Hoisted & Isolated install + .bin + patch_install` — PASS
