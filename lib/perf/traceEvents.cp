// Source: Absorbed from /data/dev/bun/src/perf/generated_perf_trace_events.cp | Synced: 2026-11-22T19:25:00Z (Canonical Bun compiler/bundler/resolver/installer/test PerfTraceEvent catalog, bitmask filter & zero-alloc accumulator)
// Pure-CP zero-allocation performance trace event catalog, 64-bit category/event bitmask filter, and O(1) fixed-array accumulator.

/// Total number of canonical compiler/runtime trace events in `PerfTraceEvent`.
pub const PERF_TRACE_EVENT_COUNT: usize = 59;

/// Bitmask covering all 59 canonical trace event bits (`(1 << 59) - 1`).
pub const PERF_TRACE_ALL_MASK: u64 = (@as(u64, 1) << 59) - 1;

/// High-level subsystem category for each canonical `PerfTraceEvent`.
pub enum PerfTraceCategory: u8 {
    Bundler = 0,
    ExtractTarball = 1,
    FolderResolver = 2,
    JSBundler = 3,
    JSGlobalObject = 4,
    JSParser = 5,
    JSPrinter = 6,
    ModuleResolver = 7,
    PackageInstaller = 8,
    PackageManifest = 9,
    RuntimeTranspilerCache = 10,
    StandaloneModuleGraph = 11,
    Symbols = 12,
    TestCommand = 13,

    /// Returns the canonical subsystem prefix string.
    fn name(): []const u8 {
        return switch (_) {
            .Bundler => "Bundler",
            .ExtractTarball => "ExtractTarball",
            .FolderResolver => "FolderResolver",
            .JSBundler => "JSBundler",
            .JSGlobalObject => "JSGlobalObject",
            .JSParser => "JSParser",
            .JSPrinter => "JSPrinter",
            .ModuleResolver => "ModuleResolver",
            .PackageInstaller => "PackageInstaller",
            .PackageManifest => "PackageManifest",
            .RuntimeTranspilerCache => "RuntimeTranspilerCache",
            .StandaloneModuleGraph => "StandaloneModuleGraph",
            .Symbols => "Symbols",
            .TestCommand => "TestCommand",
        };
    }
}

/// Canonical 59 compiler, bundler, resolver, package installer, and test runner trace events.
pub enum PerfTraceEvent: u16 {
    BundlerBindImportsToExports = 0,
    BundlerBreakOutputIntoPieces = 1,
    BundlerCloneAst = 2,
    BundlerCloneLinkerGraph = 3,
    BundlerComputeChunks = 4,
    BundlerCreateNamespaceExports = 5,
    BundlerFigureOutCommonJS = 6,
    BundlerFindAllImportedPartsInJSOrder = 7,
    BundlerFindReachableFiles = 8,
    BundlerGenerateChunksInParallel = 9,
    BundlerGenerateCodeForFileInChunkCss = 10,
    BundlerGenerateCodeForFileInChunkJS = 11,
    BundlerGenerateIsolatedHash = 12,
    BundlerGenerateSourceMapForChunk = 13,
    BundlerMarkFileLiveForTreeShaking = 14,
    BundlerMarkFileReachableForCodeSplitting = 15,
    BundlerMatchImportsWithExports = 16,
    BundlerOnParseTaskComplete = 17,
    BundlerParseJS = 18,
    BundlerParseJSON = 19,
    BundlerParseJSON5 = 20,
    BundlerParseTOML = 21,
    BundlerParseYAML = 22,
    BundlerPostProcessJSChunk = 23,
    BundlerReadFile = 24,
    BundlerRenameSymbolsInChunk = 25,
    BundlerResolveExportStarStatements = 26,
    BundlerScanImportsAndExports = 27,
    BundlerTreeShakingAndCodeSplitting = 28,
    BundlerWorkerCreate = 29,
    BundlerWrapDependencies = 30,
    BundlerWriteChunkToDisk = 31,
    BundlerWriteOutputFilesToDisk = 32,
    ExtractTarballExtract = 33,
    FolderResolverReadPackageJSONFromDiskFolder = 34,
    FolderResolverReadPackageJSONFromDiskWorkspace = 35,
    JSBundlerAddPlugin = 36,
    JSBundlerHasAnyMatches = 37,
    JSBundlerMatchOnLoad = 38,
    JSBundlerMatchOnResolve = 39,
    JSGlobalObjectCreate = 40,
    JSParserAnalyze = 41,
    JSParserParse = 42,
    JSParserPostvisit = 43,
    JSParserVisit = 44,
    JSPrinterPrint = 45,
    JSPrinterPrintWithSourceMap = 46,
    ModuleResolverResolve = 47,
    PackageInstallerInstall = 48,
    PackageManifestSerializerLoadByFile = 49,
    PackageManifestSerializerSave = 50,
    RuntimeTranspilerCacheFromFile = 51,
    RuntimeTranspilerCacheSave = 52,
    RuntimeTranspilerCacheToFile = 53,
    StandaloneModuleGraphSerialize = 54,
    SymbolsFollowAll = 55,
    TestCommandPrintCodeCoverageLCov = 56,
    TestCommandPrintCodeCoverageLCovAndText = 57,
    TestCommandPrintCodeCoverageText = 58,

    /// Returns the stable numeric identifier (`0..58`).
    fn toId(): u16 {
        return @intFromEnum(_);
    }

    /// Converts a numeric identifier (`0..58`) into `PerfTraceEvent`, or `null` if out of range.
    static fn fromId(id: u16): PerfTraceEvent? {
        if (id >= PERF_TRACE_EVENT_COUNT) return null;
        return @enumFromInt(id);
    }

    /// Returns the single-bit mask (`1 << id`) for this trace event.
    fn bit(): u64 {
        const shift: u6 = @intCast(@intFromEnum(_));
        return @as(u64, 1) << shift;
    }

    /// Returns the canonical dot-separated event name (exact match with upstream Bun trace catalog).
    fn name(): []const u8 {
        return switch (_) {
            .BundlerBindImportsToExports => "Bundler.BindImportsToExports",
            .BundlerBreakOutputIntoPieces => "Bundler.breakOutputIntoPieces",
            .BundlerCloneAst => "Bundler.cloneAST",
            .BundlerCloneLinkerGraph => "Bundler.CloneLinkerGraph",
            .BundlerComputeChunks => "Bundler.computeChunks",
            .BundlerCreateNamespaceExports => "Bundler.CreateNamespaceExports",
            .BundlerFigureOutCommonJS => "Bundler.FigureOutCommonJS",
            .BundlerFindAllImportedPartsInJSOrder => "Bundler.findAllImportedPartsInJSOrder",
            .BundlerFindReachableFiles => "Bundler.findReachableFiles",
            .BundlerGenerateChunksInParallel => "Bundler.generateChunksInParallel",
            .BundlerGenerateCodeForFileInChunkCss => "Bundler.generateCodeForFileInChunkCss",
            .BundlerGenerateCodeForFileInChunkJS => "Bundler.generateCodeForFileInChunkJS",
            .BundlerGenerateIsolatedHash => "Bundler.generateIsolatedHash",
            .BundlerGenerateSourceMapForChunk => "Bundler.generateSourceMapForChunk",
            .BundlerMarkFileLiveForTreeShaking => "Bundler.markFileLiveForTreeShaking",
            .BundlerMarkFileReachableForCodeSplitting => "Bundler.markFileReachableForCodeSplitting",
            .BundlerMatchImportsWithExports => "Bundler.MatchImportsWithExports",
            .BundlerOnParseTaskComplete => "Bundler.onParseTaskComplete",
            .BundlerParseJS => "Bundler.ParseJS",
            .BundlerParseJSON => "Bundler.ParseJSON",
            .BundlerParseJSON5 => "Bundler.ParseJSON5",
            .BundlerParseTOML => "Bundler.ParseTOML",
            .BundlerParseYAML => "Bundler.ParseYAML",
            .BundlerPostProcessJSChunk => "Bundler.postProcessJSChunk",
            .BundlerReadFile => "Bundler.readFile",
            .BundlerRenameSymbolsInChunk => "Bundler.renameSymbolsInChunk",
            .BundlerResolveExportStarStatements => "Bundler.ResolveExportStarStatements",
            .BundlerScanImportsAndExports => "Bundler.scanImportsAndExports",
            .BundlerTreeShakingAndCodeSplitting => "Bundler.treeShakingAndCodeSplitting",
            .BundlerWorkerCreate => "Bundler.Worker.create",
            .BundlerWrapDependencies => "Bundler.WrapDependencies",
            .BundlerWriteChunkToDisk => "Bundler.writeChunkToDisk",
            .BundlerWriteOutputFilesToDisk => "Bundler.writeOutputFilesToDisk",
            .ExtractTarballExtract => "ExtractTarball.extract",
            .FolderResolverReadPackageJSONFromDiskFolder => "FolderResolver.readPackageJSONFromDisk.folder",
            .FolderResolverReadPackageJSONFromDiskWorkspace => "FolderResolver.readPackageJSONFromDisk.workspace",
            .JSBundlerAddPlugin => "JSBundler.addPlugin",
            .JSBundlerHasAnyMatches => "JSBundler.hasAnyMatches",
            .JSBundlerMatchOnLoad => "JSBundler.matchOnLoad",
            .JSBundlerMatchOnResolve => "JSBundler.matchOnResolve",
            .JSGlobalObjectCreate => "JSGlobalObject.create",
            .JSParserAnalyze => "JSParser.analyze",
            .JSParserParse => "JSParser.parse",
            .JSParserPostvisit => "JSParser.postvisit",
            .JSParserVisit => "JSParser.visit",
            .JSPrinterPrint => "JSPrinter.print",
            .JSPrinterPrintWithSourceMap => "JSPrinter.printWithSourceMap",
            .ModuleResolverResolve => "ModuleResolver.resolve",
            .PackageInstallerInstall => "PackageInstaller.install",
            .PackageManifestSerializerLoadByFile => "PackageManifest.Serializer.loadByFile",
            .PackageManifestSerializerSave => "PackageManifest.Serializer.save",
            .RuntimeTranspilerCacheFromFile => "RuntimeTranspilerCache.fromFile",
            .RuntimeTranspilerCacheSave => "RuntimeTranspilerCache.save",
            .RuntimeTranspilerCacheToFile => "RuntimeTranspilerCache.toFile",
            .StandaloneModuleGraphSerialize => "StandaloneModuleGraph.serialize",
            .SymbolsFollowAll => "Symbols.followAll",
            .TestCommandPrintCodeCoverageLCov => "TestCommand.printCodeCoverageLCov",
            .TestCommandPrintCodeCoverageLCovAndText => "TestCommand.printCodeCoverageLCovAndText",
            .TestCommandPrintCodeCoverageText => "TestCommand.printCodeCoverageText",
        };
    }

    /// Returns the owning subsystem category for this event.
    fn category(): PerfTraceCategory {
        const id = @intFromEnum(_);
        if (id <= 32) return PerfTraceCategory.Bundler;
        if (id == 33) return PerfTraceCategory.ExtractTarball;
        if (id <= 35) return PerfTraceCategory.FolderResolver;
        if (id <= 39) return PerfTraceCategory.JSBundler;
        if (id == 40) return PerfTraceCategory.JSGlobalObject;
        if (id <= 44) return PerfTraceCategory.JSParser;
        if (id <= 46) return PerfTraceCategory.JSPrinter;
        if (id == 47) return PerfTraceCategory.ModuleResolver;
        if (id == 48) return PerfTraceCategory.PackageInstaller;
        if (id <= 50) return PerfTraceCategory.PackageManifest;
        if (id <= 53) return PerfTraceCategory.RuntimeTranspilerCache;
        if (id == 54) return PerfTraceCategory.StandaloneModuleGraph;
        if (id == 55) return PerfTraceCategory.Symbols;
        return PerfTraceCategory.TestCommand;
    }

    /// Returns the owning subsystem name string.
    fn categoryName(): []const u8 {
        return _.category().name();
    }

    /// Looks up a `PerfTraceEvent` by its exact canonical name, or `null` if unknown.
    static fn fromName(event_name: []const u8): PerfTraceEvent? {
        let id: u16 = 0;
        while (id < PERF_TRACE_EVENT_COUNT) {
            const ev: PerfTraceEvent = @enumFromInt(id);
            if (sliceEql(ev.name(), event_name)) return ev;
            id += 1;
        }
        return null;
    }
}

fn sliceEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

/// Computes the 64-bit event mask containing all events belonging to `cat`.
pub fn categoryEventMask(cat: PerfTraceCategory): u64 {
    let mask: u64 = 0;
    let id: u16 = 0;
    while (id < PERF_TRACE_EVENT_COUNT) {
        const ev: PerfTraceEvent = @enumFromInt(id);
        if (ev.category() == cat) {
            mask |= ev.bit();
        }
        id += 1;
    }
    return mask;
}

/// Zero-allocation 64-bit bitmask filter for O(1) trace event enablement checks.
pub struct PerfTraceFilter {
    mask: u64 = 0,

    static fn none(): PerfTraceFilter {
        return PerfTraceFilter{ mask: 0 };
    }

    static fn all(): PerfTraceFilter {
        return PerfTraceFilter{ mask: PERF_TRACE_ALL_MASK };
    }

    static fn forCategory(cat: PerfTraceCategory): PerfTraceFilter {
        return PerfTraceFilter{ mask: categoryEventMask(cat) };
    }

    fn enable(*_, ev: PerfTraceEvent) {
        _.mask |= ev.bit();
    }

    fn disable(*_, ev: PerfTraceEvent) {
        _.mask &= ~ev.bit();
    }

    fn enableCategory(*_, cat: PerfTraceCategory) {
        _.mask |= categoryEventMask(cat);
    }

    fn disableCategory(*_, cat: PerfTraceCategory) {
        _.mask &= ~categoryEventMask(cat);
    }

    fn isEnabled(ev: PerfTraceEvent): bool {
        return (_.mask & ev.bit()) != 0;
    }

    fn enabledCount(): u32 {
        return @as(u32, @popCount(_.mask & PERF_TRACE_ALL_MASK));
    }
}

/// Zero-allocation fixed-table O(1) accumulator for all 59 canonical `PerfTraceEvent` entries.
pub struct PerfTraceAccumulator {
    counts: [PERF_TRACE_EVENT_COUNT]u64 = [0; PERF_TRACE_EVENT_COUNT],
    total_ns: [PERF_TRACE_EVENT_COUNT]u64 = [0; PERF_TRACE_EVENT_COUNT],
    max_ns: [PERF_TRACE_EVENT_COUNT]u64 = [0; PERF_TRACE_EVENT_COUNT],

    static fn init(): PerfTraceAccumulator {
        return PerfTraceAccumulator{};
    }

    /// Records a single event duration in nanoseconds (saturating addition on overflow).
    fn record(*_, ev: PerfTraceEvent, duration_ns: u64) {
        const idx = @as(usize, ev.toId());
        _.counts[idx] +|= 1;
        _.total_ns[idx] +|= duration_ns;
        if (duration_ns > _.max_ns[idx]) {
            _.max_ns[idx] = duration_ns;
        }
    }

    /// Records an event only if enabled by `filter`.
    fn recordFiltered(*_, filter: PerfTraceFilter, ev: PerfTraceEvent, duration_ns: u64) {
        if (!filter.isEnabled(ev)) return;
        _.record(ev, duration_ns);
    }

    fn countOf(ev: PerfTraceEvent): u64 {
        return _.counts[@as(usize, ev.toId())];
    }

    fn totalNsOf(ev: PerfTraceEvent): u64 {
        return _.total_ns[@as(usize, ev.toId())];
    }

    fn maxNsOf(ev: PerfTraceEvent): u64 {
        return _.max_ns[@as(usize, ev.toId())];
    }

    fn avgNsOf(ev: PerfTraceEvent): u64 {
        const idx = @as(usize, ev.toId());
        const c = _.counts[idx];
        if (c == 0) return 0;
        return _.total_ns[idx] / c;
    }

    /// Returns total accumulated nanoseconds across all events in `cat`.
    fn categoryTotalNs(cat: PerfTraceCategory): u64 {
        let sum: u64 = 0;
        let id: u16 = 0;
        while (id < PERF_TRACE_EVENT_COUNT) {
            const ev: PerfTraceEvent = @enumFromInt(id);
            if (ev.category() == cat) {
                sum +|= _.total_ns[@as(usize, id)];
            }
            id += 1;
        }
        return sum;
    }

    /// Returns total accumulated nanoseconds across all 59 canonical events.
    fn grandTotalNs(): u64 {
        let sum: u64 = 0;
        let i: usize = 0;
        while (i < PERF_TRACE_EVENT_COUNT) {
            sum +|= _.total_ns[i];
            i += 1;
        }
        return sum;
    }

    /// Returns the event with the highest accumulated `total_ns`, or `null` if no samples recorded.
    fn hottestEvent(): PerfTraceEvent? {
        let best_id: u16 = 0;
        let best_ns: u64 = 0;
        let found = false;
        let id: u16 = 0;
        while (id < PERF_TRACE_EVENT_COUNT) {
            const ns = _.total_ns[@as(usize, id)];
            if (ns > best_ns) {
                best_ns = ns;
                best_id = id;
                found = true;
            }
            id += 1;
        }
        if (!found) return null;
        return @enumFromInt(best_id);
    }

    /// Resets all counters and timings to zero.
    fn reset(*_) {
        let i: usize = 0;
        while (i < PERF_TRACE_EVENT_COUNT) {
            _.counts[i] = 0;
            _.total_ns[i] = 0;
            _.max_ns[i] = 0;
            i += 1;
        }
    }
}
