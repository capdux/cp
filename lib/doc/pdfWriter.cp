// Source: cp/lib/pdf/writer.cp (Zero-Dependency PDF 1.4 Multi-Page Document Writer)
// Pure CP PDF 1.4 multi-page generator supporting Base14 fonts, vector drawing,
// RGB colors, Info metadata, and 10-digit exact byte-offset xref tables.

const std = @import("std");

/// Errors returned by PDF generation and inspection.
pub err PdfError {
    InvalidPdf,
    UnsupportedVersion,
    MissingXRef,
    MissingEof,
    InvalidArgument,
    OutOfMemory,
}

/// Standard PDF Base14 built-in Type1 fonts (require no font embedding).
pub enum PdfFont: u8 {
    Helvetica = 0,
    HelveticaBold = 1,
    Courier = 2,
    TimesRoman = 3,

    /// Returns the PDF page resource identifier (`/F1` .. `/F4`).
    fn resourceName(): []const u8 {
        return switch (_) {
            .Helvetica => "/F1",
            .HelveticaBold => "/F2",
            .Courier => "/F3",
            .TimesRoman => "/F4",
        };
    }

    /// Returns the standard PDF BaseFont dictionary name.
    fn baseFontName(): []const u8 {
        return switch (_) {
            .Helvetica => "/Helvetica",
            .HelveticaBold => "/Helvetica-Bold",
            .Courier => "/Courier",
            .TimesRoman => "/Times-Roman",
        };
    }
}

/// Page dimensions in PDF points (1 pt = 1/72 inch).
pub struct PageSize {
    width: f64 = 595.28,
    height: f64 = 841.89,

    /// Standard ISO A4 portrait dimensions (595.28 x 841.89 pt).
    pub static fn a4(): PageSize {
        return PageSize{ width: 595.28, height: 841.89 };
    }

    /// Standard US Letter portrait dimensions (612 x 792 pt).
    pub static fn letter(): PageSize {
        return PageSize{ width: 612.0, height: 792.0 };
    }

    /// Creates a custom page size in PDF points.
    pub static fn custom(width: f64, height: f64): PageSize {
        return PageSize{ width: width, height: height };
    }
}

fn appendUsize(out: *std.ArrayList(u8), alloc: std.mem.Allocator, val: usize): void ! PdfError {
    let buf: [32]u8 = undefined;
    const s = std.fmt.bufPrint(&buf, "{d}", .{val}) catch return PdfError.InvalidArgument;
    out.appendSlice(alloc, s) catch return PdfError.OutOfMemory;
}

fn appendXrefOffset(out: *std.ArrayList(u8), alloc: std.mem.Allocator, offset: usize): void ! PdfError {
    let buf: [32]u8 = undefined;
    const s = std.fmt.bufPrint(&buf, "{d:0>10}", .{offset}) catch return PdfError.InvalidArgument;
    out.appendSlice(alloc, s) catch return PdfError.OutOfMemory;
}

fn appendPdfNum(out: *std.ArrayList(u8), alloc: std.mem.Allocator, val: f64): void ! PdfError {
    if (!std.math.isFinite(val)) {
        return PdfError.InvalidArgument;
    }
    let buf: [64]u8 = undefined;
    const raw = std.fmt.bufPrint(&buf, "{d:.4}", .{val}) catch return PdfError.InvalidArgument;
    let end: usize = raw.len;
    let has_dot = false;
    for (c of raw) {
        if (c == '.') {
            has_dot = true;
            break;
        }
    }
    if (has_dot) {
        while (end > 0 && raw[end - 1] == '0') {
            end -= 1;
        }
        if (end > 0 && raw[end - 1] == '.') {
            end -= 1;
        }
    }
    const trimmed = raw[0..end];
    if (std.mem.eql(u8, trimmed, "-0") || trimmed.len == 0) {
        out.append(alloc, '0') catch return PdfError.OutOfMemory;
    } else {
        out.appendSlice(alloc, trimmed) catch return PdfError.OutOfMemory;
    }
}

fn clampUnit(v: f64): f64 ! PdfError {
    if (!std.math.isFinite(v)) {
        return PdfError.InvalidArgument;
    }
    if (v < 0.0) return 0.0;
    if (v > 1.0) return 1.0;
    return v;
}

fn appendPdfLiteralString(out: *std.ArrayList(u8), alloc: std.mem.Allocator, s: []const u8): void ! PdfError {
    out.append(alloc, '(') catch return PdfError.OutOfMemory;
    let i: usize = 0;
    let flush_start: usize = 0;
    while (i < s.len) : (i += 1) {
        const c = s[i];
        if (c == '\\' || c == '(' || c == ')' || c == '\r' || c == '\n' || c == '\t') {
            if (i > flush_start) {
                out.appendSlice(alloc, s[flush_start..i]) catch return PdfError.OutOfMemory;
            }
            switch (c) {
                '\\' => out.appendSlice(alloc, "\\\\") catch return PdfError.OutOfMemory,
                '(' => out.appendSlice(alloc, "\\(") catch return PdfError.OutOfMemory,
                ')' => out.appendSlice(alloc, "\\)") catch return PdfError.OutOfMemory,
                '\r' => out.appendSlice(alloc, "\\r") catch return PdfError.OutOfMemory,
                '\n' => out.appendSlice(alloc, "\\n") catch return PdfError.OutOfMemory,
                '\t' => out.appendSlice(alloc, "\\t") catch return PdfError.OutOfMemory,
                else => {},
            }
            flush_start = i + 1;
        }
    }
    if (i > flush_start) {
        out.appendSlice(alloc, s[flush_start..i]) catch return PdfError.OutOfMemory;
    }
    out.append(alloc, ')') catch return PdfError.OutOfMemory;
}

/// Mutable page canvas recording PDF 1.4 text and vector graphics operators.
pub struct PdfPage {
    allocator: std.mem.Allocator = undefined,
    size: PageSize = PageSize.a4(),
    content: std.ArrayList(u8) = .empty,

    fn init(alloc: std.mem.Allocator, size: PageSize): PdfPage {
        return PdfPage{
            allocator: alloc,
            size: size,
            content: .empty,
        };
    }

    fn deinit(*_) {
        _.content.deinit(_.allocator);
    }

    /// Returns a read-only slice of the raw PDF content stream operators for this page.
    pub fn contentSlice(): []const u8 {
        return _.content.items;
    }

    /// Draws UTF-8/ASCII text at `(x, y)` in PDF points using the given Base14 `font` and `size`.
    /// Properly escapes `\`, `(`, and `)` inside PDF string literals.
    pub fn drawText(*_, x: f64, y: f64, font: PdfFont, size: f64, text: []const u8): void ! PdfError {
        if (!std.math.isFinite(size) || size <= 0.0) {
            return PdfError.InvalidArgument;
        }
        _.content.appendSlice(_.allocator, "BT\n") catch return PdfError.OutOfMemory;
        _.content.appendSlice(_.allocator, font.resourceName()) catch return PdfError.OutOfMemory;
        _.content.append(_.allocator, ' ') catch return PdfError.OutOfMemory;
        appendPdfNum(&_.content, _.allocator, size)?;
        _.content.appendSlice(_.allocator, " Tf\n") catch return PdfError.OutOfMemory;
        appendPdfNum(&_.content, _.allocator, x)?;
        _.content.append(_.allocator, ' ') catch return PdfError.OutOfMemory;
        appendPdfNum(&_.content, _.allocator, y)?;
        _.content.appendSlice(_.allocator, " Td\n") catch return PdfError.OutOfMemory;
        appendPdfLiteralString(&_.content, _.allocator, text)?;
        _.content.appendSlice(_.allocator, " Tj\nET\n") catch return PdfError.OutOfMemory;
    }

    /// Sets the non-stroking (fill) RGB color (`0.0` .. `1.0` per channel).
    pub fn setFillRgb(*_, r: f64, g: f64, b: f64): void ! PdfError {
        const cr = clampUnit(r)?;
        const cg = clampUnit(g)?;
        const cb = clampUnit(b)?;
        appendPdfNum(&_.content, _.allocator, cr)?;
        _.content.append(_.allocator, ' ') catch return PdfError.OutOfMemory;
        appendPdfNum(&_.content, _.allocator, cg)?;
        _.content.append(_.allocator, ' ') catch return PdfError.OutOfMemory;
        appendPdfNum(&_.content, _.allocator, cb)?;
        _.content.appendSlice(_.allocator, " rg\n") catch return PdfError.OutOfMemory;
    }

    /// Sets the stroking RGB color (`0.0` .. `1.0` per channel).
    pub fn setStrokeRgb(*_, r: f64, g: f64, b: f64): void ! PdfError {
        const cr = clampUnit(r)?;
        const cg = clampUnit(g)?;
        const cb = clampUnit(b)?;
        appendPdfNum(&_.content, _.allocator, cr)?;
        _.content.append(_.allocator, ' ') catch return PdfError.OutOfMemory;
        appendPdfNum(&_.content, _.allocator, cg)?;
        _.content.append(_.allocator, ' ') catch return PdfError.OutOfMemory;
        appendPdfNum(&_.content, _.allocator, cb)?;
        _.content.appendSlice(_.allocator, " RG\n") catch return PdfError.OutOfMemory;
    }

    /// Sets the stroke line width in PDF points (`w >= 0`).
    pub fn setLineWidth(*_, w: f64): void ! PdfError {
        if (!std.math.isFinite(w) || w < 0.0) {
            return PdfError.InvalidArgument;
        }
        appendPdfNum(&_.content, _.allocator, w)?;
        _.content.appendSlice(_.allocator, " w\n") catch return PdfError.OutOfMemory;
    }

    /// Strokes a straight line segment from `(x1, y1)` to `(x2, y2)`.
    pub fn drawLine(*_, x1: f64, y1: f64, x2: f64, y2: f64): void ! PdfError {
        appendPdfNum(&_.content, _.allocator, x1)?;
        _.content.append(_.allocator, ' ') catch return PdfError.OutOfMemory;
        appendPdfNum(&_.content, _.allocator, y1)?;
        _.content.appendSlice(_.allocator, " m\n") catch return PdfError.OutOfMemory;
        appendPdfNum(&_.content, _.allocator, x2)?;
        _.content.append(_.allocator, ' ') catch return PdfError.OutOfMemory;
        appendPdfNum(&_.content, _.allocator, y2)?;
        _.content.appendSlice(_.allocator, " l\nS\n") catch return PdfError.OutOfMemory;
    }

    /// Strokes an axis-aligned rectangle at `(x, y)` with dimensions `(w, h)`.
    pub fn drawRect(*_, x: f64, y: f64, w: f64, h: f64): void ! PdfError {
        appendPdfNum(&_.content, _.allocator, x)?;
        _.content.append(_.allocator, ' ') catch return PdfError.OutOfMemory;
        appendPdfNum(&_.content, _.allocator, y)?;
        _.content.append(_.allocator, ' ') catch return PdfError.OutOfMemory;
        appendPdfNum(&_.content, _.allocator, w)?;
        _.content.append(_.allocator, ' ') catch return PdfError.OutOfMemory;
        appendPdfNum(&_.content, _.allocator, h)?;
        _.content.appendSlice(_.allocator, " re\nS\n") catch return PdfError.OutOfMemory;
    }

    /// Fills an axis-aligned rectangle at `(x, y)` with dimensions `(w, h)`.
    pub fn fillRect(*_, x: f64, y: f64, w: f64, h: f64): void ! PdfError {
        appendPdfNum(&_.content, _.allocator, x)?;
        _.content.append(_.allocator, ' ') catch return PdfError.OutOfMemory;
        appendPdfNum(&_.content, _.allocator, y)?;
        _.content.append(_.allocator, ' ') catch return PdfError.OutOfMemory;
        appendPdfNum(&_.content, _.allocator, w)?;
        _.content.append(_.allocator, ' ') catch return PdfError.OutOfMemory;
        appendPdfNum(&_.content, _.allocator, h)?;
        _.content.appendSlice(_.allocator, " re\nf\n") catch return PdfError.OutOfMemory;
    }
}

/// Multi-page PDF 1.4 document builder.
pub struct PdfDocument {
    allocator: std.mem.Allocator = undefined,
    pages: std.ArrayList(*PdfPage) = .empty,
    title: ([]u8)? = null,
    author: ([]u8)? = null,
    creator: ([]u8)? = null,

    /// Creates a new empty PDF 1.4 document.
    pub static fn init(alloc: std.mem.Allocator): PdfDocument {
        return PdfDocument{
            allocator: alloc,
            pages: .empty,
            title: null,
            author: null,
            creator: null,
        };
    }

    /// Releases all pages and metadata owned by the document.
    pub fn deinit(*_) {
        for (p of _.pages.items) {
            p.deinit();
            _.allocator.destroy(p);
        }
        _.pages.deinit(_.allocator);
        if (t := _.title) {
            _.allocator.free(t);
            _.title = null;
        }
        if (a := _.author) {
            _.allocator.free(a);
            _.author = null;
        }
        if (c := _.creator) {
            _.allocator.free(c);
            _.creator = null;
        }
    }

    /// Sets the document `/Title` metadata field.
    pub fn setTitle(*_, t: []const u8): void ! PdfError {
        const duped = _.allocator.dupe(u8, t) catch return PdfError.OutOfMemory;
        if (prev := _.title) {
            _.allocator.free(prev);
        }
        _.title = duped;
    }

    /// Sets the document `/Author` metadata field.
    pub fn setAuthor(*_, a: []const u8): void ! PdfError {
        const duped = _.allocator.dupe(u8, a) catch return PdfError.OutOfMemory;
        if (prev := _.author) {
            _.allocator.free(prev);
        }
        _.author = duped;
    }

    /// Sets the document `/Creator` metadata field.
    pub fn setCreator(*_, c: []const u8): void ! PdfError {
        const duped = _.allocator.dupe(u8, c) catch return PdfError.OutOfMemory;
        if (prev := _.creator) {
            _.allocator.free(prev);
        }
        _.creator = duped;
    }

    /// Appends a new page with `size` and returns a stable mutable handle to it.
    pub fn addPage(*_, size: PageSize): (*PdfPage) ! PdfError {
        if (!std.math.isFinite(size.width) || !std.math.isFinite(size.height) || size.width <= 0.0 || size.height <= 0.0) {
            return PdfError.InvalidArgument;
        }
        let page_ptr = _.allocator.create(PdfPage) catch return PdfError.OutOfMemory;
        unsafe {
            page_ptr.* = PdfPage.init(_.allocator, size);
        }
        errdefer {
            page_ptr.deinit();
            _.allocator.destroy(page_ptr);
        }
        _.pages.append(_.allocator, page_ptr) catch return PdfError.OutOfMemory;
        return page_ptr;
    }

    /// Returns the current number of pages added to the document.
    pub fn pageCount(): usize {
        return _.pages.items.len;
    }

    /// Serializes the document into a complete, self-contained PDF 1.4 byte buffer.
    /// Caller owns the returned slice and must free it with `alloc.free(...)`.
    pub fn build(alloc: std.mem.Allocator): ([]u8) ! PdfError {
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(alloc);

        const has_info = _.title != null || _.author != null || _.creator != null;
        const page_count: usize = _.pages.items.len;
        // Objects:
        // 1: Catalog
        // 2: Pages
        // 3..6: Base14 fonts (/F1../F4)
        // 7..(7 + 2*page_count - 1): Page + Content stream pairs
        // optional info_obj_id = 7 + 2*page_count
        const base_page_obj: usize = 7;
        const info_obj_id: usize = base_page_obj + page_count * usize(2);
        const info_extra: usize = if (has_info) usize(1) else usize(0);
        const total_objs: usize = usize(6) + page_count * usize(2) + info_extra;

        let offsets: std.ArrayList(usize) = .empty;
        defer offsets.deinit(alloc);
        offsets.resize(alloc, total_objs + 1) catch return PdfError.OutOfMemory;
        offsets.items[0] = 0;

        // PDF 1.4 Header
        out.appendSlice(alloc, "%PDF-1.4\n") catch return PdfError.OutOfMemory;

        // 1 0 obj: Catalog
        offsets.items[1] = out.items.len;
        out.appendSlice(alloc, "1 0 obj\n<< /Type /Catalog /Pages 2 0 R >>\nendobj\n") catch return PdfError.OutOfMemory;

        // 2 0 obj: Pages
        offsets.items[2] = out.items.len;
        out.appendSlice(alloc, "2 0 obj\n<< /Type /Pages /Kids [") catch return PdfError.OutOfMemory;
        let pi: usize = 0;
        while (pi < page_count) : (pi += 1) {
            if (pi > 0) {
                out.append(alloc, ' ') catch return PdfError.OutOfMemory;
            }
            appendUsize(&out, alloc, base_page_obj + pi * 2)?;
            out.appendSlice(alloc, " 0 R") catch return PdfError.OutOfMemory;
        }
        out.appendSlice(alloc, "] /Count ") catch return PdfError.OutOfMemory;
        appendUsize(&out, alloc, page_count)?;
        out.appendSlice(alloc, " >>\nendobj\n") catch return PdfError.OutOfMemory;

        // 3..6: Base14 Fonts
        const fonts: [4]PdfFont = [PdfFont.Helvetica, PdfFont.HelveticaBold, PdfFont.Courier, PdfFont.TimesRoman];
        let fi: usize = 0;
        while (fi < 4) : (fi += 1) {
            const obj_id = 3 + fi;
            offsets.items[obj_id] = out.items.len;
            appendUsize(&out, alloc, obj_id)?;
            out.appendSlice(alloc, " 0 obj\n<< /Type /Font /Subtype /Type1 /BaseFont ") catch return PdfError.OutOfMemory;
            out.appendSlice(alloc, fonts[fi].baseFontName()) catch return PdfError.OutOfMemory;
            out.appendSlice(alloc, " >>\nendobj\n") catch return PdfError.OutOfMemory;
        }

        // Pages & Content Streams
        pi = 0;
        while (pi < page_count) : (pi += 1) {
            const p = _.pages.items[pi];
            const page_obj_id = base_page_obj + pi * 2;
            const content_obj_id = page_obj_id + 1;

            offsets.items[page_obj_id] = out.items.len;
            appendUsize(&out, alloc, page_obj_id)?;
            out.appendSlice(alloc, " 0 obj\n<< /Type /Page /Parent 2 0 R /MediaBox [0 0 ") catch return PdfError.OutOfMemory;
            appendPdfNum(&out, alloc, p.size.width)?;
            out.append(alloc, ' ') catch return PdfError.OutOfMemory;
            appendPdfNum(&out, alloc, p.size.height)?;
            out.appendSlice(alloc, "] /Resources << /Font << /F1 3 0 R /F2 4 0 R /F3 5 0 R /F4 6 0 R >> >> /Contents ") catch return PdfError.OutOfMemory;
            appendUsize(&out, alloc, content_obj_id)?;
            out.appendSlice(alloc, " 0 R >>\nendobj\n") catch return PdfError.OutOfMemory;

            const stream_bytes = p.content.items;
            offsets.items[content_obj_id] = out.items.len;
            appendUsize(&out, alloc, content_obj_id)?;
            out.appendSlice(alloc, " 0 obj\n<< /Length ") catch return PdfError.OutOfMemory;
            appendUsize(&out, alloc, stream_bytes.len)?;
            out.appendSlice(alloc, " >>\nstream\n") catch return PdfError.OutOfMemory;
            out.appendSlice(alloc, stream_bytes) catch return PdfError.OutOfMemory;
            out.appendSlice(alloc, "\nendstream\nendobj\n") catch return PdfError.OutOfMemory;
        }

        // Optional Info Dictionary
        if (has_info) {
            offsets.items[info_obj_id] = out.items.len;
            appendUsize(&out, alloc, info_obj_id)?;
            out.appendSlice(alloc, " 0 obj\n<<") catch return PdfError.OutOfMemory;
            if (t := _.title) {
                out.appendSlice(alloc, " /Title ") catch return PdfError.OutOfMemory;
                appendPdfLiteralString(&out, alloc, t)?;
            }
            if (a := _.author) {
                out.appendSlice(alloc, " /Author ") catch return PdfError.OutOfMemory;
                appendPdfLiteralString(&out, alloc, a)?;
            }
            if (c := _.creator) {
                out.appendSlice(alloc, " /Creator ") catch return PdfError.OutOfMemory;
                appendPdfLiteralString(&out, alloc, c)?;
            }
            out.appendSlice(alloc, " >>\nendobj\n") catch return PdfError.OutOfMemory;
        }

        // Cross-Reference Table (20-byte entries)
        const xref_offset = out.items.len;
        out.appendSlice(alloc, "xref\n0 ") catch return PdfError.OutOfMemory;
        appendUsize(&out, alloc, total_objs + 1)?;
        out.appendSlice(alloc, "\n0000000000 65535 f \n") catch return PdfError.OutOfMemory;
        let oi: usize = 1;
        while (oi <= total_objs) : (oi += 1) {
            appendXrefOffset(&out, alloc, offsets.items[oi])?;
            out.appendSlice(alloc, " 00000 n \n") catch return PdfError.OutOfMemory;
        }

        // Trailer
        out.appendSlice(alloc, "trailer\n<< /Size ") catch return PdfError.OutOfMemory;
        appendUsize(&out, alloc, total_objs + 1)?;
        out.appendSlice(alloc, " /Root 1 0 R") catch return PdfError.OutOfMemory;
        if (has_info) {
            out.appendSlice(alloc, " /Info ") catch return PdfError.OutOfMemory;
            appendUsize(&out, alloc, info_obj_id)?;
            out.appendSlice(alloc, " 0 R") catch return PdfError.OutOfMemory;
        }
        out.appendSlice(alloc, " >>\nstartxref\n") catch return PdfError.OutOfMemory;
        appendUsize(&out, alloc, xref_offset)?;
        out.appendSlice(alloc, "\n%%EOF\n") catch return PdfError.OutOfMemory;

        return out.toOwnedSlice(alloc) catch return PdfError.OutOfMemory;
    }
}
