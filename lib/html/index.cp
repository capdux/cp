// Source: Absorbed from /data/dev/bun/src/lolhtml_sys/lol_html.cp | Synced: 2026-11-23T22:01:50Z
// lib/html/index.cp
// Unified Public Exports for cap:html

export * from "./escape.cp";
export * from "./tokenizer.cp";
export * from "./selector.cp";
export * from "./entities.cp";
export * from "./sanitizer.cp";
export * from "./extractor.cp";

export {
    needsHtmlEscape,
    escapeHtml,
    unescapeHtml,
} from "./esc.cp";

// === W29-Sync R7: Consolidated HTML Asset Scanner ===
export {
    AssetKind,
    AssetRef,
    SrcsetCandidate,
    parseSrcset,
    HtmlAssetScanner,
} from "./assetScanner.cp";
