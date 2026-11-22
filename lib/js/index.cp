// Source: Absorbed from /data/dev/bun/src/js_lexer/identifier.cp | Synced: 2026-11-21T17:06:43Z (Unified Exports for cap:js)

export {
    KeywordKind,
    classifyKeyword,
    isReservedWord,
    isValidAsciiIdent,
    isValidPrivateIdent,
    needsPropertyQuoting,
    escapeJsString,
} from "./ident.cp";

export {
    AssignTarget,
    Code,
    Level,
    OpInfo,
    opInfo,
} from "./op.cp";

export {
    UseDirectiveBoundering,
    UseDirectiveFlags,
    UseDirective,
    scanUseDirectives,
} from "./useDirective.cp";

export {
    FOLD_MAX_STR_LEN,
    FOLD_MAX_PART_TAIL_LEN,
    FOLD_MAX_TEMPLATE_PARTS,
    FoldStringKind,
    JsPrimitiveType,
    JsFoldExprKind,
    JsTemplatePart,
    JsFoldExpr,
    formatJsInt,
    foldStringAddition,
} from "./foldString.cp";

export {
    JS_SCOPE_MAX_SCOPES,
    JS_SCOPE_MAX_MEMBERS,
    JS_SCOPE_NO_PARENT,
    JsScopeError,
    JsScopeKind,
    JsStrictModeKind,
    JsSymbolKind,
    JsSymbolMergeResult,
    canMergeJsSymbols,
    JsScopeMember,
    JsScopeNode,
    JsScopeTree,
} from "./scope.cp";
