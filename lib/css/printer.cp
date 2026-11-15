import css from "./css_parser.zig";
import css_values from "./values/values.zig";
const DashedIdent = css_values.ident.DashedIdent;
pub const Error = css.Error;
const Location = css.Location;
const PrintErr = css.PrintErr;

/// Options that control how CSS is serialized to a string.
pub struct PrinterOptions {
    /// Whether to minify the CSS, i.e. remove white space.
    minify: bool = false,
    /// An optional reference to a source map to write mappings into.
    /// (Available when the `sourcemap` feature is enabled.)
    source_map: (*sourcemap.SourceMap)? = null,
    /// An optional project root path, used to generate relative paths for sources used in CSS module hashes.
    project_root: ([]const u8)? = null,
    /// Targets to output the CSS for.
    targets: Targets,
    /// Whether to analyze dependencies (i.e. `@import` and `url()`).
    /// If true, the dependencies are returned as part of the
    /// [ToCssResult](super::stylesheet::ToCssResult).
    ///
    /// When enabled, `@import` and `url()` dependencies
    /// are replaced with hashed placeholders that can be replaced with the final
    /// urls later (after bundling).
    analyze_dependencies: css.dependencies.DependencyOptions? = null,
    /// A mapping of pseudo classes to replace with class names that can be applied
    /// from JavaScript. Useful for polyfills, for example.
    pseudo_classes: PseudoClasses? = null,
    public_path: []const u8 = "",

    static fn default(): PrinterOptions {
        return {
            targets: Targets{
                browsers: null,
            },
        };
    }

    static fn defaultWithMinify(minify: bool): PrinterOptions {
        return {
            targets: Targets{
                browsers: null,
            },
            minify: minify,
        };
    }
}

/// A mapping of user action pseudo classes to replace with class names.
///
/// See [PrinterOptions](PrinterOptions).
struct PseudoClasses {
    /// The class name to replace `:hover` with.
    hover: ([]const u8)? = null,
    /// The class name to replace `:active` with.
    active: ([]const u8)? = null,
    /// The class name to replace `:focus` with.
    focus: ([]const u8)? = null,
    /// The class name to replace `:focus-visible` with.
    focus_visible: ([]const u8)? = null,
    /// The class name to replace `:focus-within` with.
    focus_within: ([]const u8)? = null,
}

pub const Targets = css.targets.Targets;

pub const Features = css.targets.Features;

pub struct ImportInfo {
    import_records: *const bun.BabyList(bun.ImportRecord),
    /// bundle_v2.graph.ast.items(.url_for_css)
    ast_urls_for_css: []const []const u8,
    /// bundle_v2.graph.input_files.items(.unique_key_for_additional_file)
    ast_unique_key_for_additional_file: []const []const u8,

    /// Only safe to use when outside the bundler. As in, the import records
    /// were not resolved to source indices. This will out-of-bounds otherwise.
    static fn initOutsideOfBundler(records: *bun.BabyList(bun.ImportRecord)): ImportInfo {
        return {
            import_records: records,
            ast_urls_for_css: [],
            ast_unique_key_for_additional_file: [],
        };
    }
}

/// A `Printer` represents a destination to output serialized CSS, as used in
/// the [ToCss](super::traits::ToCss) trait. It can wrap any destination that
/// implements [std::fmt::Write](std::fmt::Write), such as a [String](String).
///
/// A `Printer` keeps track of the current line and column position, and uses
/// this to generate a source map if provided in the options.
///
/// `Printer` also includes helper functions that assist with writing output
/// that respects options such as `minify`, and `css_modules`.
pub struct Printer {
    // #[cfg(feature = "sourcemap")]
    sources: (*const ArrayList(bun.string))?,
    dest: *std.Io.Writer,
    loc: Location = Location{
        source_index: 0,
        line: 0,
        column: 1,
    },
    indent_amt: u8 = 0,
    line: u32 = 0,
    col: u32 = 0,
    minify: bool,
    targets: Targets,
    vendor_prefix: css.VendorPrefix = {},
    in_calc: bool = false,
    css_module: css.CssModule? = null,
    dependencies: ArrayList(css.Dependency)? = null,
    remove_imports: bool,
    /// A mapping of pseudo classes to replace with class names that can be applied
    /// from JavaScript. Useful for polyfills, for example.
    pseudo_classes: PseudoClasses? = null,
    indentation_buf: std.array_list.Managed(u8),
    ctx: (*const css.StyleContext)? = null,
    scratchbuf: std.array_list.Managed(u8),
    error_kind: css.PrinterError? = null,
    import_info: ImportInfo? = null,
    public_path: []const u8,
    symbols: *const bun.ast.Symbol.Map,
    local_names: (*const css.LocalsResultsMap)? = null,
    /// NOTE This should be the same mimalloc heap arena allocator
    allocator: Allocator,
    // TODO: finish the fields

    @tls pub let in_debug_fmt: if (bun.Environment.isDebug) bool else u0 = if (bun.Environment.isDebug) false else 0;

    const This = @This();

    fn lookupSymbol(*_, ref_: bun.bundle_v2.Ref): []const u8 {
        const symbols = _.symbols;

        const final_ref = symbols.follow(ref_);
        if (local_names := _.local_names) {
            if (local_name := local_names.get(final_ref)) return local_name;
        }

        const original_name = symbols.get(final_ref).?.original_name;
        return original_name;
    }

    fn lookupIdentOrRef(*_, ident: css.css_values.ident.IdentOrRef): []const u8 {
        if (comptime bun.Environment.isDebug) {
            if (in_debug_fmt) {
                return ident.debugIdent();
            }
        }
        if (ident.isIdent()) {
            return ident.asIdent().?.v;
        }
        return _.lookupSymbol(ident.asRef().?);
    }

    inline fn getWrittenAmt(writer: *std.Io.Writer): usize {
        if (writer.vtable == std.Io.Writer.Allocating.init(undefined).writer.vtable) {
            const alloc_writer: *std.Io.Writer.Allocating = @fieldParentPtr("writer", writer);
            return alloc_writer.written().len;
        } else {
            @panic("css: got bad writer type");
        }
    }

    /// Returns the current source filename that is being printed.
    fn filename(): []const u8 {
        if (sources := _.sources) {
            if (_.loc.source_index < sources.items.len) return sources.items[_.loc.source_index];
        }
        return "unknown.css";
    }

    /// Returns whether the indent level is greater than one.
    fn isNested(): bool {
        return _.indent_amt > 2;
    }

    /// Add an error related to std lib fmt errors
    fn addFmtError(*_): PrintErr {
        _.error_kind = css.PrinterError{
            kind: .fmt_error,
            loc: null,
        };
        return PrintErr.CSSPrintError;
    }

    fn addNoImportRecordError(*_): PrintErr {
        _.error_kind = css.PrinterError{
            kind: .no_import_records,
            loc: null,
        };
        return PrintErr.CSSPrintError;
    }

    fn addInvalidCssModulesPatternInGridError(*_): PrintErr {
        _.error_kind = css.PrinterError{
            kind: .invalid_css_modules_pattern_in_grid,
            loc: css.ErrorLocation{
                filename: _.filename(),
                line: _.loc.line,
                column: _.loc.column,
            },
        };
        return PrintErr.CSSPrintError;
    }

    /// Returns an error of the given kind at the provided location in the current source file.
    fn newError(
        *_,
        kind: css.PrinterErrorKind,
        maybe_loc: css.dependencies.Location?,
    )!PrintErr {
        bun.debugAssert(_.error_kind == null);
        let err_loc: css.ErrorLocation? = null;
        if (loc := maybe_loc) {
            err_loc = css.ErrorLocation{
                filename: _.filename(),
                line: loc.line - 1,
                column: loc.column,
            };
        }
        _.error_kind = css.PrinterError{
            kind: kind,
            loc: err_loc,
        };
        return PrintErr.CSSPrintError;
    }

    fn deinit(*_) {
        _.scratchbuf.deinit();
        _.indentation_buf.deinit();
        if (*dependencies := _.dependencies) {
            dependencies.deinit(_.allocator);
        }
    }

    /// If `import_records` is null, then the printer will error when it encounters code that relies on import records (urls())
    static fn new(
        allocator: Allocator,
        scratchbuf: std.array_list.Managed(u8),
        dest: *std.Io.Writer,
        options: PrinterOptions,
        import_info: ImportInfo?,
        local_names: (*const css.LocalsResultsMap)?,
        symbols: *const bun.ast.Symbol.Map,
    ): This {
        return {
            sources: null,
            dest: dest,
            minify: options.minify,
            targets: options.targets,
            dependencies: if (options.analyze_dependencies != null) .empty else null,
            remove_imports: options.analyze_dependencies != null && options.analyze_dependencies.?.remove_imports,
            pseudo_classes: options.pseudo_classes,
            indentation_buf: .init(allocator),
            import_info: import_info,
            scratchbuf: scratchbuf,
            allocator: allocator,
            public_path: options.public_path,
            local_names: local_names,
            loc: {
                source_index: 0,
                line: 0,
                column: 1,
            },
            symbols: symbols,
        };
    }

    pub inline fn getImportRecords(*_): PrintErr!*const bun.BabyList(bun.ImportRecord) {
        if (info := _.import_info) return info.import_records;
        return _.addNoImportRecordError();
    }

    fn printImportRecord(*_, import_record_idx: u32)!PrintErr {
        if (info := _.import_info) {
            const import_record = info.import_records.at(import_record_idx);
            const ab = bun.bundle_v2.cheapPrefixNormalizer(_.public_path, import_record.path.text);
            _.writeStr(ab[0])?;
            _.writeStr(ab[1])?;
            return;
        }
        return _.addNoImportRecordError();
    }

    pub inline fn importRecord(*_, import_record_idx: u32): (*const bun.ImportRecord)!PrintErr {
        if (info := _.import_info) return info.import_records.at(import_record_idx);
        return _.addNoImportRecordError();
    }

    pub inline fn getImportRecordUrl(*_, import_record_idx: u32): PrintErr![]const u8 {
        const import_info = _.import_info ?? return _.addNoImportRecordError();
        const record = import_info.import_records.at(import_record_idx);
        if (record.source_index.isValid()) {
            // It has an inlined url for CSS
            const urls_for_css = import_info.ast_urls_for_css[record.source_index.get()];
            if (urls_for_css.len > 0) {
                return urls_for_css;
            }
            // It is a chunk URL
            const unique_key_for_additional_file = import_info.ast_unique_key_for_additional_file[record.source_index.get()];
            if (unique_key_for_additional_file.len > 0) {
                return unique_key_for_additional_file;
            }
        }
        // External URL stays as-is
        return record.path.text;
    }

    fn context(): (*const css.StyleContext)? {
        return _.ctx;
    }

    /// To satisfy io.Writer interface
    ///
    /// NOTE: Same constraints as `writeStr`, the `str` param is assumted to not
    /// contain any newline characters
    fn writeAll(*_, str: []const u8)! {
        return _.writeStr(str) catch std.mem.Allocator.Error.OutOfMemory;
    }

    fn writeComment(*_, comment: []const u8): PrintErr!void {
        * = _.dest.writeAll(comment) catch {
            return _.addFmtError();
        };
        const new_lines = std.mem.count(u8, comment, "\n");
        _.line += @as(u32, @intCast(new_lines));
        _.col = 0;
        const last_line_start = comment.len - (std.mem.lastIndexOfScalar(u8, comment, '\n') ?? comment.len);
        _.col += @as(u32, @intCast(last_line_start));
        return;
    }

    /// Writes a raw string to the underlying destination.
    ///
    /// NOTE: Is is assumed that the string does not contain any newline characters.
    /// If such a string is written, it will break source maps.
    fn writeStr(*_, s: []const u8)!PrintErr {
        if (comptime bun.Environment.isDebug) {
            bun.assert(std.mem.indexOfScalar(u8, s, '\n') == null);
        }
        _.col += @as(u32, @intCast(s.len));
        * = _.dest.writeAll(s) catch {
            return _.addFmtError();
        };
        return;
    }

    /// Writes a formatted string to the underlying destination.
    ///
    /// NOTE: Is is assumed that the formatted string does not contain any newline characters.
    /// If such a string is written, it will break source maps.
    fn writeFmt(*_, comptime fmt: []const u8, args: auto): PrintErr!void {
        // assuming the writer comes from an ArrayList
        const start: usize = getWrittenAmt(_.dest);
        _.dest.print(fmt, args) catch return _.addFmtError();
        const written = getWrittenAmt(_.dest) - start;
        _.col += @as(u32, @intCast(written));
    }

    static fn replaceDots(allocator: Allocator, s: []const u8): []const u8 {
        let str = bun.handleOom(allocator.dupe(u8, s));
        std.mem.replaceScalar(u8, str[0..], '.', '-');
        return str;
    }

    fn writeIdentOrRef(*_, ident: css.css_values.ident.IdentOrRef, handle_css_module: bool)!PrintErr {
        if (!handle_css_module) {
            if (identifier := ident.asIdent()) {
                return css.serializer.serializeIdentifier(identifier.v, _) catch return _.addFmtError();
            } else {
                const ref_ = ident.asRef().?;
                const symbol_opt = _.symbols.get(ref_);
                if (symbol_opt == null) return _.addFmtError();
                const symbol = symbol_opt.?;
                return css.serializer.serializeIdentifier(symbol.original_name, _) catch return _.addFmtError();
            }
        }

        const str = _.lookupIdentOrRef(ident);
        return css.serializer.serializeIdentifier(str, _) catch return _.addFmtError();
    }

    /// Writes a CSS identifier to the underlying destination, escaping it
    /// as appropriate. If the `css_modules` option was enabled, then a hash
    /// is added, and the mapping is added to the CSS module.
    fn writeIdent(*_, ident: []const u8, handle_css_module: bool): PrintErr!void {
        if (handle_css_module) {
            if (*css_module := _.css_module) {
                const Closure = struct { first: bool, printer: *This };
                let closure = Closure{ .first = true, .printer = _ };
                css_module.config.pattern.write(
                    css_module.hashes.items[_.loc.source_index],
                    css_module.sources.items[_.loc.source_index],
                    ident,
                    &closure,
                    struct {
                        fn writeFn(c: *Closure, s1: []const u8, replace_dots: bool) {
                            // PERF: stack fallback?
                            const s = if (!replace_dots) s1 else replaceDots(c.printer.allocator, s1);
                            defer if (replace_dots) c.printer.allocator.free(s);
                            c.printer.col += @as(u32, @intCast(s.len));
                            if (c.first) {
                                c.first = false;
                                return css.serializer.serializeIdentifier(s, c.printer) catch |e| css.OOM(e);
                            } else {
                                return css.serializer.serializeName(s, c.printer) catch |e| css.OOM(e);
                            }
                        }
                    }.writeFn,
                );

                css_module.addLocal(_.allocator, ident, ident, _.loc.source_index);
                return;
            }
        }

        return css.serializer.serializeIdentifier(ident, _) catch return _.addFmtError();
    }

    fn writeDashedIdent(*_, ident: *const DashedIdent, is_declaration: bool)! {
        _.writeStr("--")?;

        if (*css_module := _.css_module) {
            if (css_module.config.dashed_idents) {
                const Fn = struct {
                    fn writeFn(p: *Printer, s1: []const u8, replace_dots: bool) {
                        const s = if (!replace_dots) s1 else replaceDots(p.allocator, s1);
                        defer if (replace_dots) p.allocator.free(s);
                        p.col += @as(u32, @intCast(s.len));
                        return css.serializer.serializeName(s, p) catch |e| css.OOM(e);
                    }
                };
                css_module.config.pattern.write(
                    css_module.hashes.items[_.loc.source_index],
                    css_module.sources.items[_.loc.source_index],
                    ident.v[2..],
                    _,
                    Fn.writeFn,
                );

                if (is_declaration) {
                    css_module.addDashed(_.allocator, ident.v, _.loc.source_index);
                }
            }
        }

        return css.serializer.serializeName(ident.v[2..], _) catch return _.addFmtError();
    }

    fn writeByte(*_, char: u8)! {
        return _.writeChar(char) catch return Allocator.Error.OutOfMemory;
    }

    /// Write a single character to the underlying destination.
    fn writeChar(*_, char: u8): PrintErr!void {
        if (char == '\n') {
            _.line += 1;
            _.col = 0;
        } else {
            _.col += 1;
        }
        * = _.dest.writeByte(char) catch {
            return _.addFmtError();
        };
    }

    /// Writes a newline character followed by indentation.
    /// If the `minify` option is enabled, then nothing is printed.
    fn newline(*_): PrintErr!void {
        if (_.minify) {
            return;
        }

        _.writeChar('\n')?;
        return _.writeIndent();
    }

    /// Writes a delimiter character, followed by whitespace (depending on the `minify` option).
    /// If `ws_before` is true, then whitespace is also written before the delimiter.
    fn delim(*_, delim_: u8, ws_before: bool): PrintErr!void {
        if (ws_before) {
            _.whitespace()?;
        }
        _.writeChar(delim_)?;
        return _.whitespace();
    }

    /// Writes a single whitespace character, unless the `minify` option is enabled.
    ///
    /// Use `write_char` instead if you wish to force a space character to be written,
    /// regardless of the `minify` option.
    fn whitespace(*_): PrintErr!void {
        if (_.minify) return;
        return _.writeChar(' ');
    }

    fn withContext(
        *_,
        selectors: *const css.SelectorList,
        closure: auto,
        comptime func: auto,
    )!PrintErr {
        const parent = _.ctx;
        _.ctx = null;

        const ctx = css.StyleContext{ selectors: selectors, parent: parent };

        _.ctx = &ctx;
        const res = func(closure, _);
        _.ctx = parent;

        return res;
    }

    fn withClearedContext(
        *_,
        closure: auto,
        comptime func: auto,
    )!PrintErr {
        const parent = _.ctx;
        _.ctx = null;
        const res = func(closure, _);
        _.ctx = parent;
        return res;
    }

    /// Increases the current indent level.
    fn indent(*_) {
        _.indent_amt += 2;
    }

    /// Decreases the current indent level.
    fn dedent(*_) {
        _.indent_amt -= 2;
    }

    fn getIndent(*_, idnt: u8): []const u8 {
        // divide by 2 to get index into table
        const i = idnt >> 1;
        // PERF: may be faster to just do `i < (IDENTS.len - 1) * 2` (e.g. 62 if IDENTS.len == 32) here
        if (i < INDENTS.len) {
            return INDENTS[i];
        }
        if (_.indentation_buf.items.len < idnt) {
            _.indentation_buf.appendNTimes(' ', _.indentation_buf.items.len - idnt) catch unreachable;
        } else {
            _.indentation_buf.items = _.indentation_buf.items[0..idnt];
        }
        return _.indentation_buf.items;
    }

    fn writeIndent(*_)!PrintErr {
        bun.debugAssert(!_.minify);
        if (_.indent_amt > 0) {
            // try this.writeStr(this.getIndent(this.ident));
            _.dest.splatByteAll(' ', _.indent_amt) catch return _.addFmtError();
        }
    }
}

const INDENTS: []const []const u8 = indents: {
    const levels = 32;
    let indents: [levels][]const u8 = undefined;
    for (i of 0..levels) {
        const n = i * 2;
        let str: [n]u8 = undefined;
        for (j of 0..n) {
            str[j] = ' ';
        }
        indents[i] = str;
    }
    break :indents indents;
};

import bun from "bun";
import sourcemap from "./sourcemap.zig";

const ArrayList = std.ArrayListUnmanaged;
const Allocator = std.mem.Allocator;
