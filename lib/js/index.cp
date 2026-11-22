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

export {
    JS_SYMBOL_MAX_ENTRIES,
    JS_SYMBOL_INVALID_INDEX,
    JsSymbolTableError,
    JsSlotNamespace,
    JsImportItemStatus,
    JsSymbolEntryKind,
    JsSymbolRef,
    JsSymbolRecord,
    JsSymbolTable,
} from "./symbolTable.cp";

export {
    TS_NS_MAX_SCOPES,
    TS_NS_MAX_MEMBERS,
    TS_NS_MAX_PROXIES,
    TsNamespaceError,
    TsNamespaceMemberKind,
    TsNamespaceMember,
    TsNamespaceBlock,
    TsPropertyAccessProxy,
    TsNamespaceRegistry,
} from "./tsNamespace.cp";

export {
    TsMetadataError,
    TsMetadataKind,
    TsDecoratorMetadata,
    TsTypeArgLookaheadToken,
    canFollowTsTypeArguments,
} from "./tsMetadata.cp";

export {
    JsToken,
    lookupJsKeyword,
    isStrictModeReserved,
    remapStrictModeReserved,
    JsPropertyModifier,
    isTsAccessibilityModifier,
    lookupJsxEntity,
} from "./lexerTables.cp";

export {
    IMPORT_TRIM_MAX_ITEMS,
    TsImportsNotUsedAsValues,
    ImportTrimPolicy,
    ImportBindingUsage,
    ImportStmtInput,
    ImportStmtDecision,
    ImportTrimResult,
    evaluateImportStmt,
} from "./importTrim.cp";

export {
    REPL_MAX_STMTS,
    REPL_MAX_HOISTED_VARS,
    ReplTransformError,
    ReplStmtKind,
    ReplWrapMode,
    ReplStmtDesc,
    ReplProgramInput,
    ReplTransformPlan,
    planReplTransform,
} from "./replTransform.cp";
