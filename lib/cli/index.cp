// Source: Absorbed from /data/dev/bun/src/cli/cli.cp | Synced: 2026-11-22T17:17:59Z
// lib/cli/index.cp
// Unified Public Exports for cap:cli

export * from "./args.cp";
export * from "./tty.cp";
export * from "./table.cp";
export * from "./ciInfo.cp";
export * from "./subcommand.cp";

export {
    ColorLevel,
    ColorEnv,
    parseForceColor,
    detectColorLevel,
    supportsColor,
    supportsTruecolor,
} from "./color.cp";

export {
    ProgressSpinnerStyle,
    spinnerFrame,
    ProgressMeter,
} from "./progress.cp";

export {
    SpinnerPreset,
    SpinnerSpec,
    formatSpinnerFrame,
    formatSpinnerDone,
} from "./spinner.cp";

export {
    TableBorderStyle,
    CellAlign,
    computeColumnWidths,
    renderBorderLine,
    renderTableRow,
    renderTable,
} from "./tableRender.cp";

export {
    TreeStyle,
    TreePrefix,
    TreeNode,
    renderFlatTree,
} from "./tree.cp";

export {
    ColonListError,
    ColonPair,
    parseColonPair,
    parseColonPairNonEmptyKey,
    parseLoaderMapping,
    parseColonList,
    parseLoaderMappingList,
    findColonValue,
} from "./colonList.cp";

export {
    CompletionError,
    CompletionShell,
    CompletionItem,
    formatCompletions,
    formatCompletionItems,
} from "./completions.cp";

export {
    PmCommandsError,
    YarnCommandEra,
    PmStage,
    yarnCommandEra,
    isYarnCommand,
    mapYarnToCapSubcommand,
    pmStagePlainName,
    pmStageLabel,
    formatPmStageBanner,
} from "./pmCmds.cp";

export {
    PM_VIEW_MAX_PATH_SEGMENTS,
    PM_VIEW_RECENT_VERSIONS_LIMIT,
    PmViewError,
    PmViewSpec,
    PmPropertyPathSegment,
    PmPropertyPath,
    PmViewDistInfo,
    PmViewKv,
    PmViewMaintainer,
    PmViewSummary,
    formatRecentVersionsHint,
    formatPmViewJsonError,
} from "./pmView.cp";

export {
    EDITOR_MAX_ARGS,
    EDITOR_DEFAULT_PREFERENCE_COUNT,
    EditorOpenError,
    OpenerPlatform,
    platformOpener,
    UrlOpenCommand,
    buildOpenUrlCommand,
    EditorKind,
    defaultEditorPreferenceAt,
    EditorResolution,
    resolveEditorFromPreference,
    EditorOpenCommand,
    buildOpenEditorCommand,
    normalizeEditorTempFilename,
} from "./open.cp";

export {
    COMPLETION_MAX_CANDIDATES,
    COMPLETION_PATH_BUF_CAP,
    CompletionInstallError,
    CompletionInstallEnv,
    CompletionCandidateList,
    completionFilename,
    collectCompletionDirs,
    resolveZshrcPath,
    zshrcNeedsCompletionHint,
    formatZshrcSourceLine,
    collectAliasSymlinkTargets,
    isWindowsInstallerBinDir,
} from "./installComp.cp";

export {
    PmVersionError,
    PmVersionType,
    PmSemver,
    PmParsedVersionArg,
    PmVersionOptions,
    parsePmVersionArg,
    normalizeGitTagVersion,
    computeNextPmVersion,
    formatPmCommitMessage,
    formatPmGitTag,
} from "./pmVersion.cp";

export {
    PM_PKG_MAX_SEGMENTS,
    PmPkgError,
    PmPkgSubCommand,
    PmPkgSegmentKind,
    PmPkgPathSegment,
    PmPkgKeyPath,
    PmPkgAssignment,
    PmPkgValueKind,
    classifyPmPkgValue,
} from "./pmPkg.cp";

export {
    FILTER_MAX_PATTERNS,
    FILTER_PATH_BUF_CAP,
    FilterArgError,
    FilterPatternKind,
    FilterRule,
    FilterSet,
    isFilterSkippedDir,
    formatWorkspacePackagePattern,
    normalizeFilterPath,
    matchFilterGlob,
    formatFilterScriptPrefix,
} from "./filterArg.cp";

export {
    TEST_SCANNER_MAX_FILTERS,
    TEST_SCANNER_MAX_EXCLUSIONS,
    TEST_SCANNER_MAX_IGNORE_PATTERNS,
    TestScannerError,
    isTestSupportedExtension,
    hasTestNameSuffix,
    couldBeTestFile,
    isExactTestPathArg,
    formatScopedTestName,
    TestFileScanner,
} from "./testScan.cp";

export {
    PM_AUDIT_MAX_IGNORE,
    PM_AUDIT_MAX_PACKAGES,
    PM_AUDIT_MAX_VERSIONS_PER_PKG,
    PM_AUDIT_MAX_ADVISORIES,
    PM_AUDIT_MAX_DEP_EDGES,
    PM_AUDIT_MAX_PATH_DEPTH,
    PmAuditError,
    PmAuditSeverity,
    PmAuditFilter,
    PmAuditPackageEntry,
    PmAuditRequestBuilder,
    PmAuditAdvisory,
    PmAuditCounts,
    PmAuditReport,
    PmAuditDepEdge,
    PmAuditDepGraph,
} from "./pmAudit.cp";

export {
    PM_OUTDATED_MAX_ENTRIES,
    PM_OUTDATED_MAX_PATTERNS,
    PM_OUTDATED_MAX_GROUPED_WS,
    PmOutdatedError,
    PmOutdatedDepKind,
    PmOutdatedUpdateKind,
    PmOutdatedFilterKind,
    PmOutdatedSemver,
    matchOutdatedGlob,
    PmOutdatedFilterRule,
    PmOutdatedFilter,
    PmOutdatedEntry,
    PmOutdatedTable,
} from "./pmOutdated.cp";

export {
    PM_TRUST_MAX_ALLOWLIST,
    PmTrustError,
    PmDepSourceKind,
    PmLifecyclePhase,
    PmTrustDecision,
    PmTrustSpec,
    PmLifecyclePackage,
    PmTrustPolicy,
} from "./pmTrusted.cp";

export {
    FILTER_RUN_MAX_SCRIPTS,
    FILTER_RUN_MAX_EDGES,
    FILTER_RUN_DEFAULT_ELIDE_LINES,
    FILTER_RUN_SIGINT,
    FilterRunError,
    FilterScriptPhase,
    FilterRunMode,
    FilterScriptStatus,
    FilterElideResult,
    FilterScriptNode,
    elideScriptOutput,
    formatLifecycleScriptName,
    formatFilterDuration,
    formatFilterStatusLine,
    formatFilterOutputBlock,
    FilterRunGraph,
} from "./filterRun.cp";

export {
    BUNX_CACHE_TTL_SECS,
    BunxError,
    BunxRuntimeChoice,
    BunxPackageSpec,
    BunxOptions,
    formatBunxCreatePackageName,
    parseBunxPackageSpec,
    resolveBunxDefaultBinName,
    extractBinBasename,
    isBunxCacheStale,
    formatBunxCacheDirName,
    resolveBunxShebangRuntime,
} from "./bunx.cp";

export {
    MULTI_RUN_MAX_SCRIPTS,
    MULTI_RUN_MAX_EDGES,
    MULTI_RUN_LINE_BUF_CAP,
    MULTI_RUN_PALETTE_LEN,
    MultiRunError,
    MultiRunEdgeKind,
    MultiRunStatus,
    MultiRunTask,
    multiRunPaletteColor,
    formatMultiRunPrefix,
    formatMultiRunExitStatus,
    MultiRunLineBuffer,
    MultiRunScheduler,
} from "./multiRun.cp";

export {
    PM_LINK_MAX_NAME_LEN,
    PM_LINK_MAX_PATH_LEN,
    PM_LINK_MAX_TARGETS,
    PmLinkError,
    PmLinkMode,
    PmLinkMethod,
    isValidNpmLinkName,
    extractLinkScope,
    trimTrailingPathSep,
    formatLinkDependencySpec,
    formatLinkSuccessMessage,
    PmLinkInvocation,
    PmGlobalLinkPlan,
} from "./pmLink.cp";

export {
    PARSE_ARGS_MAX_TOKENS,
    ParseArgsError,
    ParseArgsValueType,
    ParseArgsOptionDef,
    ParseArgsTokenSubtype,
    ParseArgsTokenKind,
    ParseArgsLongSplit,
    ParseArgsToken,
    ParseArgsConfig,
    isOptionLikeValue,
    findOptionByShortName,
    findOptionByLongName,
    classifyArgToken,
    splitLongOption,
    parseArgsTokens,
} from "./parseArgsUtils.cp";

export {
    CLI_COMMAND_TAG_COUNT,
    CliCommandCapabilities,
    CliCommandTag,
} from "./cmdTag.cp";

export {
    CAP_ARGS_MAX_SUGGEST_DIST,
    CAP_SUBCOMMAND_COUNT,
    CapArgsError,
    CliFlagValueKind,
    CliFlagValueSource,
    CapSubcommand,
    CapCompletionShell,
    CliFlagSpec,
    CapEnvBinding,
    CliParsedFlag,
    CapArgsDiagnostic,
    CapArgsResult,
    parseCliInt,
    parseCliUint,
    parseCliByteSize,
    isAllowedEnumValue,
    resolveCapSubcommand,
    capSubcommandName,
    capSubcommandSummary,
    levenshteinDistance,
    suggestClosestSubcommand,
    suggestClosestFlag,
    formatCapArgsDiagnostic,
    getParsedFlag,
    parseCapArgs,
    formatCommandHelp,
    formatShellCompletion,
} from "./capArgs.cp";

export {
    SCAFFOLD_DEFAULT_VERSION,
    SCAFFOLD_DEFAULT_LICENSE,
    SCAFFOLD_DEFAULT_YEAR,
    SCAFFOLD_MAX_NAME_LEN,
    ScaffoldError,
    ScaffoldTemplateKind,
    ScaffoldConflictPolicy,
    ScaffoldFileAction,
    ScaffoldVarBinding,
    ScaffoldRenderOptions,
    ScaffoldRenderDiag,
    CapManifestDep,
    CapManifestScript,
    ScaffoldProjectOptions,
    ScaffoldTemplateFile,
    ScaffoldPlannedFile,
    ScaffoldPlanSummary,
    isReservedProjectName,
    normalizeProjectName,
    parseScaffoldTemplateKind,
    scaffoldTemplateName,
    scaffoldTemplateDescription,
    renderScaffoldTemplate,
    formatCapManifestJson,
    collectTemplateFiles,
    resolveScaffoldConflict,
    planProjectScaffold,
    formatScaffoldPlanReport,
} from "./projectScaffold.cp";

export {
    CREATE_DEFAULT_GIT_REF,
    CREATE_MAX_SPEC_LEN,
    CreateTemplateError,
    hasCreateShellMetachar,
    CreateSourceKind,
    CreateTemplateSpec,
    isOfficialBuiltinTemplate,
    normalizeCreatePackageName,
    parseCreateTemplateSpec,
    CreateDestStatus,
    CreateDestDecision,
    isSafeIgnoredDestEntry,
    isSafeCreateDestPath,
    evaluateCreateDestination,
    CreateStepKind,
    CreateWorkflowStep,
    CreatePostMetadata,
    CreateWorkflowOptions,
    shouldSkipTemplateFile,
    resolveTemplateDestFilename,
    planCreateWorkflow,
    CreateSummaryInfo,
    formatCreateNextSteps,
} from "./createTemplate.cp";

export {
    SHELL_COMP_MARKER_BEGIN,
    SHELL_COMP_MARKER_END,
    SHELL_COMP_MAX_PREFIX_LEN,
    SHELL_COMP_NO_SUBCOMMAND,
    ShellCompError,
    ShellCompDialect,
    parseShellCompDialect,
    shellCompDialectName,
    ShellCompArgHint,
    ShellCompFlagRule,
    ShellCompCommandRule,
    generateCapShellCompletion,
    ShellCompCandidateKind,
    ShellCompCandidate,
    ShellCompProjectContext,
    ShellCompEvalSummary,
    computeDynamicCompletions,
    RcCompOperation,
    RcCompAction,
    RcCompPlanResult,
    planRcCompletionInstall,
} from "./shellCompletions.cp";

export {
    INTERACTIVE_UPDATE_DEFAULT_VIEWPORT,
    INTERACTIVE_UPDATE_MAX_FILTER_LEN,
    InteractiveUpdateError,
    IuSemverTriple,
    parseIuSemver,
    IuUpgradeKind,
    IuUpgradeClassification,
    iuUpgradeAnsiColor,
    classifySemverUpgrade,
    formatVersionDiffHighlight,
    IuDepGroup,
    iuDepGroupLabel,
    IuPackageEntry,
    IuKeyEvent,
    IuSessionOutcome,
    IuSelectedUpgrade,
    InteractiveUpdateState,
    IuRenderOptions,
    renderInteractiveUpdateTable,
} from "./interactiveUpdate.cp";

export {
    PKG_UNLINK_MAX_NAME_LEN,
    PKG_UNLINK_MAX_PATH_LEN,
    PKG_UNLINK_GLOBAL_SUBDIR,
    PKG_UNLINK_GLOBAL_BIN_SUBDIR,
    PKG_UNLINK_LOCAL_MODULES_DIR,
    PkgUnlinkError,
    UnlinkParsedPkgName,
    parseUnlinkPackageName,
    normalizeUnlinkPath,
    isUnlinkPathWithinRoot,
    isSafeUnlinkBinName,
    formatGlobalLinkPath,
    formatLocalLinkPath,
    UnlinkMode,
    unlinkModeName,
    UnlinkFsEntryKind,
    UnlinkTargetProbe,
    UnlinkGuardStatus,
    unlinkGuardStatusMessage,
    UnlinkTargetValidation,
    validateUnlinkTarget,
    UnlinkOpKind,
    UnlinkTextSpan,
    getUnlinkOpText,
    UnlinkPlannedOp,
    UnlinkPlanOptions,
    UnlinkPlanSummary,
    planPkgUnlink,
    UnlinkCliArgs,
    parseUnlinkCliArgs,
    formatPkgUnlinkReport,
} from "./pkgUnlink.cp";

export {
    SEC_SCAN_SARIF_VERSION,
    SEC_SCAN_SARIF_SCHEMA,
    SEC_SCAN_MAX_PATTERN_LEN,
    SecScanError,
    SecScanSeverity,
    secScanSeverityRank,
    secScanSeverityName,
    secScanSarifLevel,
    parseSecScanSeverity,
    SecScanOutputFormat,
    parseSecScanOutputFormat,
    validateSecScanIgnoreRule,
    matchSecScanIgnorePattern,
    SecScanCliConfig,
    parseSecScanCliArgs,
    SecScanFinding,
    SecScanAggregatedItem,
    SecScanSeverityCounts,
    SecScanGateSummary,
    evaluateSecurityScan,
    formatSecurityScanSarif,
    formatSecurityScanTable,
} from "./securityScan.cp";
