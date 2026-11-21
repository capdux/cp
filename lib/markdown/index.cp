// Source: Absorbed from /data/dev/bun/src/md/ (line_analysis.cp, links.cp, ref_defs.cp)

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
