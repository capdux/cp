// Source: Absorbed from /data/dev/bun/src/md/ | Synced: 2026-11-22T12:49:17Z (line_analysis.cp, links.cp, ref_defs.cp)
// Reference-hardened against pulldown-cmark/pulldown-cmark (MIT)

export {
    SetextResult,
    AtxHeaderResult,
    CodeFenceResult,
    TableAlign,
    TableUnderlineResult,
    isSetextUnderline,
    isHrLine,
    isAtxHeaderLine,
    isOpeningCodeFence,
    isClosingCodeFence,
    isHtmlBlockStartCondition,
    isHtmlBlockEndCondition,
    matchHtmlTag,
    isBlockLevelHtmlTag,
    isCompleteHtmlTag,
    isTableUnderline,
} from "./line.cp";

export {
    LinkKind,
    ParsedLink,
    parseAutolink,
    parseInlineLinkOrImage,
    percentEncodeUrl,
    isSafeUrl,
} from "./links.cp";

export {
    RefDef,
    ParseRefDefResult,
    ParseDestResult,
    ParseTitleResult,
    normalizeLabel,
    labelsEqual,
    skipRefDefWhitespace,
    parseRefDefDest,
    parseRefDefTitle,
    parseRefDef,
    RefDefTable,
} from "./refdefs.cp";

export {
    MD_MAX_NESTING_DEPTH,
    RenderOptions,
    simdFindLineEnd,
    simdIndexOfInlineSpecial,
    simdIndexOfHtmlEscape,
    escapeHtmlSimd,
    renderToHtml,
    renderToHtmlWithOptions,
} from "./renderer.cp";
