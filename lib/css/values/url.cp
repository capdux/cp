import css from "../css_parser.zig";
const Result = css.Result;
const Printer = css.Printer;
const PrintErr = css.PrintErr;
const UrlDependency = css.dependencies.UrlDependency;

/// A CSS [url()](https://www.w3.org/TR/css-values-4/#urls) value and its source location.
pub struct Url {
    /// The url string.
    import_record_idx: u32,
    /// The location where the `url()` was seen in the CSS source file.
    loc: css.dependencies.Location,

    const This = @This();

    static fn parse(input: *css.Parser): Result(Url) {
        const start_pos = input.position();
        const loc = input.currentSourceLocation();
        const url = switch (input.expectUrl()) {
            .result => |vv| vv,
            .err => |e| return { err: e },
        };
        const import_record_idx = switch (input.addImportRecord(url, start_pos, .url)) {
            .result => |idx| idx,
            .err => |e| return { err: e },
        };
        return { result: Url{ .import_record_idx = import_record_idx, loc: css.dependencies.Location.fromSourceLocation(loc) } };
    }

    /// Returns whether the URL is absolute, and not relative.
    fn isAbsolute(import_records: *const bun.BabyList(bun.ImportRecord)): bool {
        const url = import_records.at(_.import_record_idx).path.pretty;

        // Quick checks. If the url starts with '.', it is relative.
        if (bun.strings.startsWithChar(url, '.')) {
            return false;
        }

        // If the url starts with '/' it is absolute.
        if (bun.strings.startsWithChar(url, '/')) {
            return true;
        }

        // If the url starts with '#' we have a fragment URL.
        // These are resolved relative to the document rather than the CSS file.
        // https://drafts.csswg.org/css-values-4/#local-urls
        if (bun.strings.startsWithChar(url, '#')) {
            return true;
        }

        // Otherwise, we might have a scheme. These must start with an ascii alpha character.
        // https://url.spec.whatwg.org/#scheme-start-state
        if (url.len == 0 || !std.ascii.isAlphabetic(url[0])) {
            return false;
        }

        // https://url.spec.whatwg.org/#scheme-state
        for (c of url) {
            switch (c) {
                'a'...'z', 'A'...'Z', '0'...'9', '+', '-', '.' => {},
                ':' => return true,
                else => break,
            }
        }

        return false;
    }

    fn toCss(
        dest: *Printer,
    ): PrintErr!void {
        const dep: UrlDependency? = if (dest.dependencies != null)
            UrlDependency.new(dest.allocator, _, dest.filename(), try dest.getImportRecords())
        else
            null;

        // If adding dependencies, always write url() with quotes so that the placeholder can
        // be replaced without escaping more easily. Quotes may be removed later during minification.
        if (d := dep) {
            dest.writeStr("url(")?;
            css.serializer.serializeString(d.placeholder, dest) catch return dest.addFmtError();
            dest.writeChar(')')?;

            if (*dependencies := dest.dependencies) {
                bun.handleOom(dependencies.append(dest.allocator, css.Dependency{ .url = d }));
            }

            return;
        }

        const import_record = dest.importRecord(_.import_record_idx)?;
        const url = dest.getImportRecordUrl(_.import_record_idx)?;

        if (dest.minify && !import_record.flags.is_internal) {
            let buf = std.Io.Writer.Allocating.init(dest.allocator);
            defer buf.deinit();
            // PERF(alloc) we could use stack fallback here?
            css.Token.toCssGeneric(&css.Token{ .unquoted_url = url }, &buf.writer) catch return dest.addFmtError();

            // If the unquoted url is longer than it would be quoted (e.g. `url("...")`)
            // then serialize as a string and choose the shorter version.
            if (buf.written().len > url.len + 7) {
                let buf2 = std.Io.Writer.Allocating.init(dest.allocator);
                defer buf2.deinit();
                // PERF(alloc) we could use stack fallback here?
                css.serializer.serializeString(url, &buf2.writer) catch return dest.addFmtError();
                if (buf2.written().len + 5 < buf.written().len) {
                    dest.writeStr("url(")?;
                    dest.writeStr(buf2.written())?;
                    return dest.writeChar(')');
                }
            }

            dest.writeStr(buf.written())?;
        } else {
            dest.writeStr("url(")?;
            css.serializer.serializeString(url, dest) catch return dest.addFmtError();
            dest.writeChar(')')?;
        }
    }

    fn deepClone(allocator: std.mem.Allocator): @This() {
        return css.implementDeepClone(@This(), _, allocator);
    }

    // TODO: dedupe import records??
    // This might not fucking work
    fn eql(other: *const Url): bool {
        return _.import_record_idx == other.import_record_idx;
    }

    // TODO: dedupe import records??
    // This might not fucking work
    fn hash(hasher: *std.hash.Wyhash) {
        return css.implementHash(@This(), _, hasher);
    }
}

import bun from "bun";
