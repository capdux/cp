import css from "./css_parser.zig";
import css_values from "./values/values.zig";
pub const Error = css.Error;
const PrintErr = css.PrintErr;

type Str = []const u8;

pub struct CssModule {
    config: *const Config,
    sources: *const ArrayList(Str),
    hashes: ArrayList(Str),
    exports_by_source_index: ArrayList(CssModuleExports),
    references: *CssModuleReferences,

    static fn new(
        allocator: Allocator,
        config: *const Config,
        sources: *const ArrayList(Str),
        project_root: ([]const u8)?,
        references: *CssModuleReferences,
    ): CssModule {
        // TODO: this is BAAAAAAAAAAD we are going to remove it
        const StringList = ArrayList(bun.string);
        let hashes = bun.handleOom(StringList.initCapacity(allocator, sources.items.len));
        for (path of sources.items) {
            let alloced = false;
            let source = path;
            if (root := project_root) {
                if (bun.path.Platform.auto.isAbsolute(root)) {
                    alloced = true;
                    source = bun.handleOom(allocator.dupe(u8, bun.path.relative(root, path)));
                }
            }
            const h_val = hash(
                allocator,
                "{s}",
                .{source},
                config.pattern.segments.at(0).* == .hash,
            );
            if (alloced) allocator.free(source);
            hashes.appendAssumeCapacity(h_val);
        }
        let exports_by_source_index = bun.handleOom(ArrayList(CssModuleExports).initCapacity(allocator, sources.items.len));
        exports_by_source_index.appendNTimesAssumeCapacity(CssModuleExports{}, sources.items.len);
        return CssModule{
            config: config,
            sources: sources,
            references: references,
            hashes: hashes,
            exports_by_source_index: exports_by_source_index,
        };
    }

    fn deinit(*_) {
        * = _.exports_by_source_index;
        // TODO: deinit
    }

    fn getReference(*_, allocator: Allocator, name: []const u8, source_index: u32) {
        const gop = bun.handleOom(_.exports_by_source_index.items[source_index].getOrPut(allocator, name));
        if (gop.found_existing) {
            gop.value_ptr.is_referenced = true;
        } else {
            gop.value_ptr.* = CssModuleExport{
                name: _.config.pattern.writeToString(allocator, {}, _.hashes.items[source_index], _.sources.items[source_index], name),
                composes: {},
                is_referenced: true,
            };
        }
    }

    fn referenceDashed(
        *_,
        dest: *css.Printer,
        name: []const u8,
        from_: (*const css.css_properties.css_modules.Specifier)?,
        source_index: u32,
    ): PrintErr!([]const u8)? {
        let reference: CssModuleReference = undefined;
        let key: []const u8 = undefined;
        if (specifier := from_) {
            switch (specifier.*) {
                .global => return name[2..],
                .import_record_index => |import_record_index| {
                    const import_record = dest.importRecord(import_record_index)?;
                    reference = CssModuleReference{
                        dependency: {
                            name: name[2..],
                            specifier: import_record.path.text,
                        },
                    };
                    key = import_record.path.text;
                },
            }
        } else {
            // Local export. Mark as used.
            const gop = bun.handleOom(_.exports_by_source_index.items[source_index].getOrPut(dest.allocator, name));
            if (gop.found_existing) {
                gop.value_ptr.is_referenced = true;
            } else {
                const ByteList = ArrayList(u8);
                let res = ByteList{};
                bun.handleOom(res.appendSlice(dest.allocator, "--"));
                gop.value_ptr.* = CssModuleExport{
                    name: _.config.pattern.writeToString(
                        dest.allocator,
                        res,
                        _.hashes.items[source_index],
                        _.sources.items[source_index],
                        name[2..],
                    ),
                    composes: {},
                    is_referenced: true,
                };
            }
            return null;
        }

        const the_hash = hash(dest.allocator, "{s}_{s}_{s}", .{ _.hashes.items[source_index], name, key }, false);

        _.references.put(
            dest.allocator,
            bun.handleOom(std.fmt.allocPrint(dest.allocator, "--{s}", .{the_hash})),
            reference,
        ) catch |e| bun.handleOom(e);

        return the_hash;
    }

    static fn handleComposes(
        _: *CssModule,
        _: *css.Printer,
        selectors: *const css.selector.parser.SelectorList,
        _: *const css.css_properties.css_modules.Composes,
        _: u32,
    ): css.Maybe(void, css.PrinterErrorKind) {
        // const allocator = dest.allocator;
        for (*sel of selectors.v.slice()) {
            if (sel.len() == 1 && sel.components.items[0] == .@"class") {
                continue;
            }

            // The composes property can only be used within a simple class selector.
            return { err: css.PrinterErrorKind.invalid_composes_selector };
        }

        return .success;
    }

    fn addDashed(*_, allocator: Allocator, local: []const u8, source_index: u32) {
        const gop = bun.handleOom(_.exports_by_source_index.items[source_index].getOrPut(allocator, local));
        if (!gop.found_existing) {
            gop.value_ptr.* = CssModuleExport{
                // todo_stuff.depth
                name: _.config.pattern.writeToStringWithPrefix(
                    allocator,
                    "--",
                    _.hashes.items[source_index],
                    _.sources.items[source_index],
                    local[2..],
                ),
                composes: {},
                is_referenced: false,
            };
        }
    }

    fn addLocal(*_, allocator: Allocator, exported: []const u8, local: []const u8, source_index: u32) {
        const gop = bun.handleOom(_.exports_by_source_index.items[source_index].getOrPut(allocator, exported));
        if (!gop.found_existing) {
            gop.value_ptr.* = CssModuleExport{
                // todo_stuff.depth
                name: _.config.pattern.writeToString(
                    allocator,
                    {},
                    _.hashes.items[source_index],
                    _.sources.items[source_index],
                    local,
                ),
                composes: {},
                is_referenced: false,
            };
        }
    }
}

/// Configuration for CSS modules.
pub struct Config {
    /// The name pattern to use when renaming class names and other identifiers.
    /// Default is `[hash]_[local]`.
    pattern: Pattern = {},

    /// Whether to rename dashed identifiers, e.g. custom properties.
    dashed_idents: bool = false,

    /// Whether to scope animation names.
    /// Default is `true`.
    animation: bool = true,

    /// Whether to scope grid names.
    /// Default is `true`.
    grid: bool = true,

    /// Whether to scope custom identifiers
    /// Default is `true`.
    custom_idents: bool = true,
}

const SegmentList = css.SmallList(Segment, 3);
const default_segments: [3]Segment = [ .local, Segment{ literal: "_" }, .hash ];

const PrefixClosure = struct {
    res: ArrayList(u8),
    allocator: Allocator,
    prefix: []const u8,

    fn writefn(*_, slice: []const u8, replace_dots: bool) {
        bun.handleOom(_.res.appendSlice(_.allocator, _.prefix));
        if (replace_dots) {
            const start = _.res.items.len;
            bun.handleOom(_.res.appendSlice(_.allocator, slice));
            const end = _.res.items.len;
            for (*c of _.res.items[start..end]) {
                if (c.* == '.') {
                    c.* = '-';
                }
            }
            return;
        }
        bun.handleOom(_.res.appendSlice(_.allocator, slice));
    }
};

const ToStringClosure = struct {
    res: *ArrayList(u8),
    allocator: Allocator,

    fn writefn(*_, slice: []const u8, replace_dots: bool) {
        if (replace_dots) {
            const start = _.res.items.len;
            bun.handleOom(_.res.appendSlice(_.allocator, slice));
            const end = _.res.items.len;
            for (*c of _.res.items[start..end]) {
                if (c.* == '.') {
                    c.* = '-';
                }
            }
            return;
        }
        bun.handleOom(_.res.appendSlice(_.allocator, slice));
        return;
    }
};

/// A CSS modules class name pattern.
pub struct Pattern {
    /// The list of segments in the pattern.
    segments: SegmentList = SegmentList.initInlined(&default_segments),

    /// Write the substituted pattern to a destination.
    fn write(
        hash_: []const u8,
        path: []const u8,
        local: []const u8,
        closure: auto,
        comptime writefn: *const fn (@TypeOf(closure), []const u8, replace_dots: bool) void,
    ) {
        for (*segment of _.segments.slice()) {
            switch (segment.*) {
                .literal => |s| {
                    writefn(closure, s, false);
                },
                .name => {
                    const stem = std.fs.path.stem(path);
                    if (_ := std.mem.indexOf(u8, stem, ".")) {
                        writefn(closure, stem, true);
                    } else {
                        writefn(closure, stem, false);
                    }
                },
                .local => {
                    writefn(closure, local, false);
                },
                .hash => {
                    writefn(closure, hash_, false);
                },
            }
        }
    }

    fn writeToStringWithPrefix(
        allocator: Allocator,
        comptime prefix: []const u8,
        hash_: []const u8,
        path: []const u8,
        local: []const u8,
    ): []const u8 {
        * = prefix;
        let closure = PrefixClosure{ res: {}, allocator: allocator, prefix: prefix };
        _.write(
            hash_,
            path,
            local,
            &closure,
            PrefixClosure.writefn,
        );
        return closure.res.items;
    }

    fn writeToString(
        allocator: Allocator,
        res_: ArrayList(u8),
        hash_: []const u8,
        path: []const u8,
        local: []const u8,
    ): []const u8 {
        let res = res_;
        let closure = ToStringClosure{ res: &res, allocator: allocator };
        _.write(
            hash_,
            path,
            local,
            &closure,
            ToStringClosure.writefn,
        );

        return res.items;
    }
}

/// A segment in a CSS modules class name pattern.
///
/// See [Pattern](Pattern).
pub const Segment = union(enum) {
    /// A literal string segment.
    literal: []const u8,

    /// The base file name.
    name,

    /// The original class name.
    local,

    /// A hash of the file name.
    hash,
};

/// A map of exported names to values.
pub const CssModuleExports = std.StringArrayHashMapUnmanaged(CssModuleExport);

/// A map of placeholders to references.
pub const CssModuleReferences = std.StringArrayHashMapUnmanaged(CssModuleReference);

/// An exported value from a CSS module.
pub struct CssModuleExport {
    /// The local (compiled) name for this export.
    name: []const u8,
    /// Other names that are composed by this export.
    composes: ArrayList(CssModuleReference),
    /// Whether the export is referenced in this file.
    is_referenced: bool,
}

/// A referenced name within a CSS module, e.g. via the `composes` property.
///
/// See [CssModuleExport](CssModuleExport).
pub const CssModuleReference = union(enum) {
    /// A local reference.
    local: struct {
        /// The local (compiled) name for the reference.
        name: []const u8,
    },
    /// A global reference.
    global: struct {
        /// The referenced global name.
        name: []const u8,
    },
    /// A reference to an export in a different file.
    dependency: struct {
        /// The name to reference within the dependency.
        name: []const u8,
        /// The dependency specifier for the referenced file.
        ///
        /// import record idx
        specifier: []const u8,
    },

    pub fn eql(this: *const @This(), other: *const @This()): bool {
        if (@intFromEnum(this.*) != @intFromEnum(other.*)) return false;

        return switch (this.*) {
            .local => |v| bun.strings.eql(v.name, other.local.name),
            .global => |v| bun.strings.eql(v.name, other.global.name),
            .dependency => |v| bun.strings.eql(v.name, other.dependency.name) and bun.strings.eql(v.specifier, other.dependency.specifier),
        };
    }
};

// TODO: replace with bun's hash
pub fn hash(allocator: Allocator, comptime fmt: []const u8, args: auto, at_start: bool): []const u8 {
    const count = std.fmt.count(fmt, args);
    let stack_fallback = std.heap.stackFallback(128, allocator);
    const fmt_alloc = if (count <= 128) stack_fallback.get() else allocator;
    let hasher = bun.Wyhash11.init(0);
    let fmt_str = bun.handleOom(std.fmt.allocPrint(fmt_alloc, fmt, args));
    hasher.update(fmt_str);

    const h: u32 = @truncate(hasher.final());
    let h_bytes: [4]u8 = undefined;
    std.mem.writeInt(u32, &h_bytes, h, .little);

    const encode_len = bun.base64.simdutfEncodeLenUrlSafe(h_bytes[0..].len);

    let slice_to_write = if (encode_len <= 128 - @as(usize, @intFromBool(at_start)))
        bun.handleOom(allocator.alloc(u8, encode_len + @as(usize, @intFromBool(at_start))))
    else
        fmt_str[0..];

    const base64_encoded_hash_len = bun.base64.simdutfEncodeUrlSafe(slice_to_write, &h_bytes);

    const base64_encoded_hash = slice_to_write[0..base64_encoded_hash_len];

    if (at_start && base64_encoded_hash.len > 0 && base64_encoded_hash[0] >= '0' && base64_encoded_hash[0] <= '9') {
        std.mem.copyBackwards(u8, slice_to_write[1..][0..base64_encoded_hash_len], base64_encoded_hash);
        slice_to_write[0] = '_';
        return slice_to_write[0 .. base64_encoded_hash_len + 1];
    }

    return base64_encoded_hash;
}

import bun from "bun";

const ArrayList = std.ArrayListUnmanaged;
const Allocator = std.mem.Allocator;
