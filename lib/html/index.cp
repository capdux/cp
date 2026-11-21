// Source: /data/dev/bun/src/lolhtml_sys/lol_html.cp
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
