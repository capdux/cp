# `cap:catalog` — 纯 CP 零分配 Monorepo 依赖目录协议与解析库

> 参考来源：`/data/dev/bun/src/install/lockfile/CatalogMap.cp`、`/data/dev/bun/docs/pm/catalogs.mdx` | 吸收日期：2026-11-22

100% 纯 CP、零外部依赖、零堆分配的 Monorepo 依赖目录（Dependency Catalogs）协议解析与版本解析引擎，完整支持 Bun / pnpm Workspaces 的默认目录（`catalog:`）与命名分组目录（`catalog:<group>`）。

## 导入方式

```cp
import {
    CatalogError,
    CatalogRef,
    CatalogEntry,
    CatalogMap,
    isCatalogProtocol,
    isValidCatalogName,
    parseCatalogRef,
    formatCatalogRef,
} from "cap:catalog";
```

## API 契约

### 1. 协议解析与校验

- `isCatalogProtocol(spec: []const u8): bool`：判断是否以 `"catalog:"` 开头
- `isValidCatalogName(name: []const u8): bool`：校验命名目录名合法性（非空、不含空白/控制字符、不含 `/`、`\`、`:`）
- `parseCatalogRef(spec: []const u8): CatalogRef?`：解析 `"catalog:"`（`is_default = true`）或 `"catalog:<name>"`（`is_default = false`）
- `formatCatalogRef(catalog_name: []const u8, out: *[]u8): ([]const u8)?`：零分配格式化回协议字符串

### 2. 零分配目录存储与版本解析（`CatalogMap`）

- `CatalogError`：`ok`, `capacity_full`, `duplicate_entry`, `invalid_package_name`, `invalid_catalog_name`, `nested_catalog_ref`, `invalid_version`, `not_found`
- `CatalogEntry`：`{ catalog_name, package_name, version }`（空 `catalog_name` 表示默认 `catalog`）
- `CatalogMap.init(storage: []CatalogEntry): CatalogMap`
  - `hasAny()`、`len()`、`defaultCount()`、`groupCount(catalog_name)`
  - `putDefault(*_, package_name, version): CatalogError` / `putNamed(*_, catalog_name, package_name, version): CatalogError` / `upsert(*_, catalog_name, package_name, version): CatalogError`
  - `get(catalog_name, package_name): ([]const u8)?`
  - `resolveSpecifier(package_name, spec): ([]const u8)?`：对 `catalog:` 协议解析为真实版本，对普通 semver / `workspace:` 等原样零拷贝透传
  - `sortEntries(*_)`：原地按 `(catalog_name, package_name)` 字典序排序
  - `formatLockfileSection(out: *[]u8): ([]const u8)?`：零分配输出确定性 `"catalog"` / `"catalogs"` JSON 片段

## 上游行为契约与边界注意事项

1. **默认目录与命名分组隔离（对齐 Bun `CatalogMap`）**：
   - 根 `package.json` 的 `catalog`（单数）与 `catalogs.<group>`（复数）彼此独立，同名包可在不同命名目录中指向不同版本（如 `catalog:` 的 `react: ^19.0.0` 与 `catalog:legacy` 的 `react: ^18.2.0`）。
2. **禁止嵌套 `catalog:` 与同组重复定义**：
   - `putDefault` / `putNamed` 拒绝在版本值中再嵌套 `"catalog:..."`（返回 `CatalogError.nested_catalog_ref`），并拒绝同组同包重复定义（返回 `CatalogError.duplicate_entry`，与 Bun lockfile 导入校验一致）；需要覆盖时显式使用 `upsert`。
3. **确定性 Lockfile 排序输出**：
   - 调用 `sortEntries()` 后再调用 `formatLockfileSection(out)`，可确保跨平台/任意插入顺序下产出比特完全一致的 lockfile 目录段。

## 示例

```cp
import { CatalogEntry, CatalogMap } from "cap:catalog";

fn demo() {
    let entries: [8]CatalogEntry = [{ catalog_name: "", package_name: "", version: "" }; 8];
    let map = CatalogMap.init(entries[0..]);
    * = map.putDefault("react", "^19.0.0");
    * = map.putNamed("testing", "jest", "30.0.0");

    if (ver := map.resolveSpecifier("jest", "catalog:testing")) {
        * = ver; // "30.0.0"
    }
}
```
