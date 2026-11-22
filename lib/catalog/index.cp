// Source: Absorbed from /data/dev/bun/src/install/lockfile/CatalogMap.cp | Synced: 2026-11-22T00:26:00Z (Zero-Alloc Monorepo Dependency Catalogs Protocol & Map)
// Backward-compatible re-export: canonical implementation consolidated into lib/pkg/catalog.cp

export {
    CatalogError,
    CatalogRef,
    CatalogEntry,
    CatalogMap,
    isCatalogProtocol,
    isValidCatalogName,
    parseCatalogRef,
    formatCatalogRef,
} from "../pkg/catalog.cp";
