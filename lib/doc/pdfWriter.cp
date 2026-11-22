// Source: cp/lib/pdf/writer.cp (Zero-Dependency PDF 1.4 Multi-Page Document Writer)
// Pure CP PDF 1.4 multi-page generator supporting Base14 fonts, vector drawing,
// RGB colors, Info metadata, and 10-digit exact byte-offset xref tables.

const std = @import("std");
import { deflate } from "cap:pack";

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

    /// Standard ISO A4 landscape dimensions (841.89 x 595.28 pt).
    pub static fn a4Landscape(): PageSize {
        return PageSize{ width: 841.89, height: 595.28 };
    }

    /// Standard US Letter landscape dimensions (792 x 612 pt).
    pub static fn letterLandscape(): PageSize {
        return PageSize{ width: 792.0, height: 612.0 };
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

// ============================================================================
// 2. Zero-Heap Production PDF 1.4-1.7 Layout, Vector, Table, Image XObject,
//    Outlines, UTF-16BE Metadata & FlateDecode Serializer Engine
// ============================================================================

/// Error set for zero-heap caller-slice PDF canvas & document serialization.
pub err PdfWriteError {
    BufferTooSmall,
    OutputSliceTooSmall,
    InvalidArgument,
    InvalidPageIndex,
    InvalidImage,
    CompressFailed,
}

/// Horizontal text alignment for wrapped paragraphs and table cells.
pub enum PdfTextAlign {
    left,
    center,
    right,
}

/// Supported PDF Image XObject color spaces.
pub enum PdfColorSpace {
    device_rgb,
    device_gray,

    fn channels(): usize {
        return match (_) {
            PdfColorSpace.device_rgb => 3,
            PdfColorSpace.device_gray => 1,
        };
    }

    fn pdfName(): []const u8 {
        return match (_) {
            PdfColorSpace.device_rgb => "/DeviceRGB",
            PdfColorSpace.device_gray => "/DeviceGray",
        };
    }
}

/// Normalized RGB color (`0.0` .. `1.0` per channel).
pub struct PdfRgb {
    r: f64 = 0.0;
    g: f64 = 0.0;
    b: f64 = 0.0;

    static fn init(r: f64, g: f64, b: f64): PdfRgb {
        return PdfRgb{ r: r, g: g, b: b };
    }

    static fn black(): PdfRgb {
        return PdfRgb{ r: 0.0, g: 0.0, b: 0.0 };
    }

    static fn white(): PdfRgb {
        return PdfRgb{ r: 1.0, g: 1.0, b: 1.0 };
    }

    static fn gray(v: f64): PdfRgb {
        return PdfRgb{ r: v, g: v, b: v };
    }
}

/// Page margin specification in PDF points (`72 pt = 1 inch`).
pub struct PdfMargins {
    top: f64 = 72.0;
    right: f64 = 72.0;
    bottom: f64 = 72.0;
    left: f64 = 72.0;
}

/// 2D point in PDF user-space points.
pub struct PdfPoint {
    x: f64 = 0.0;
    y: f64 = 0.0;
}

/// Visual styling parameters for structured table rendering.
pub struct PdfTableStyle {
    font: PdfFont = PdfFont.Helvetica;
    header_font: PdfFont = PdfFont.HelveticaBold;
    font_size: f64 = 10.0;
    row_height: f64 = 20.0;
    cell_padding_x: f64 = 6.0;
    border_width: f64 = 0.75;
    header_fill: PdfRgb = PdfRgb{ r: 0.90, g: 0.92, b: 0.96 };
    border_color: PdfRgb = PdfRgb{ r: 0.35, g: 0.35, b: 0.35 };
    text_color: PdfRgb = PdfRgb{ r: 0.0, g: 0.0, b: 0.0 };
}

/// Descriptor for an embedded 8-bit raster Image XObject (`/Subtype /Image`).
pub struct PdfImageSpec {
    width: u32 = 0;
    height: u32 = 0;
    color_space: PdfColorSpace = PdfColorSpace.device_rgb;
    pixels: []const u8 = [];
    compress: bool = false;
}

/// Document outline / bookmark entry pointing to a 0-based page index.
pub struct PdfBookmarkSpec {
    title: []const u8 = "";
    page_index: usize = 0;
}

/// Full document `/Info` metadata descriptor (ASCII or UTF-8 automatically encoded as UTF-16BE BOM when needed).
pub struct PdfMetadataSpec {
    title: []const u8 = "";
    author: []const u8 = "";
    subject: []const u8 = "";
    keywords: []const u8 = "";
    creator: []const u8 = "";
    producer: []const u8 = "cap:doc pdfWriter";
}

/// Single page input for `writePdfDocument`.
pub struct PdfPageSpec {
    size: PageSize = PageSize.a4();
    rotate: i32 = 0;
    content_stream: []const u8 = "";
}

// ---- Internal Fixed-Slice Formatting Helpers ----

struct SliceBuf {
    buf: []u8 = [],
    pos: usize = 0,

    static fn init(buf: []u8): SliceBuf {
        return SliceBuf{ buf: buf, pos: 0 };
    }

    fn putByte(*_, b: u8): void ! PdfWriteError {
        if (pos >= buf.len) return PdfWriteError.BufferTooSmall;
        buf[pos] = b;
        pos += 1;
    }

    fn putSlice(*_, s: []const u8): void ! PdfWriteError {
        if (pos + s.len > buf.len) return PdfWriteError.BufferTooSmall;
        let i: usize = 0;
        while (i < s.len) : (i += 1) {
            buf[pos + i] = s[i];
        }
        pos += s.len;
    }

    fn putUsize(*_, val: usize): void ! PdfWriteError {
        let tmp: [32]u8 = [0; 32];
        const s = std.fmt.bufPrint(tmp[0..], "{d}", .{val}) catch return PdfWriteError.InvalidArgument;
        putSlice(s)?;
    }

    fn putI32(*_, val: i32): void ! PdfWriteError {
        let tmp: [32]u8 = [0; 32];
        const s = std.fmt.bufPrint(tmp[0..], "{d}", .{val}) catch return PdfWriteError.InvalidArgument;
        putSlice(s)?;
    }

    fn putXref10(*_, off: usize): void ! PdfWriteError {
        let tmp: [32]u8 = [0; 32];
        const s = std.fmt.bufPrint(tmp[0..], "{d:0>10} 00000 n \n", .{off}) catch return PdfWriteError.InvalidArgument;
        putSlice(s)?;
    }

    fn putNum(*_, val: f64): void ! PdfWriteError {
        if (!std.math.isFinite(val)) return PdfWriteError.InvalidArgument;
        let tmp: [64]u8 = [0; 64];
        const raw = std.fmt.bufPrint(tmp[0..], "{d:.3}", .{val}) catch return PdfWriteError.InvalidArgument;
        let end: usize = raw.len;
        let has_dot: bool = false;
        let i: usize = 0;
        while (i < raw.len) : (i += 1) {
            if (raw[i] == '.') {
                has_dot = true;
                break;
            }
        }
        if (has_dot) {
            while (end > 0 && raw[end - 1] == '0') end -= 1;
            if (end > 0 && raw[end - 1] == '.') end -= 1;
        }
        const trimmed = raw[0..end];
        if (trimmed.len == 0 || std.mem.eql(u8, trimmed, "-0")) {
            putByte('0')?;
        } else {
            putSlice(trimmed)?;
        }
    }

    fn putUnitClamped(*_, v: f64): void ! PdfWriteError {
        if (!std.math.isFinite(v)) return PdfWriteError.InvalidArgument;
        let c: f64 = v;
        if (c < 0.0) c = 0.0;
        if (c > 1.0) c = 1.0;
        putNum(c)?;
    }

    fn putHexByte(*_, b: u8): void ! PdfWriteError {
        const hex = "0123456789ABCDEF";
        putByte(hex[(b >> 4) & 0x0F])?;
        putByte(hex[b & 0x0F])?;
    }

    fn putLiteralString(*_, s: []const u8): void ! PdfWriteError {
        putByte('(')?;
        let i: usize = 0;
        while (i < s.len) : (i += 1) {
            const c = s[i];
            if (c == '\\') {
                putSlice("\\\\")?;
            } else if (c == '(') {
                putSlice("\\(")?;
            } else if (c == ')') {
                putSlice("\\)")?;
            } else if (c == '\r') {
                putSlice("\\r")?;
            } else if (c == '\n') {
                putSlice("\\n")?;
            } else if (c == '\t') {
                putSlice("\\t")?;
            } else {
                putByte(c)?;
            }
        }
        putByte(')')?;
    }

    /// Writes `s` as an ASCII literal string if all bytes < 0x80, or as a UTF-16BE BOM hex string
    /// `<FEFF...>` when Unicode characters are present (round-trips cleanly with `decodePdfStringToUtf8`).
    fn putPdfTextString(*_, s: []const u8): void ! PdfWriteError {
        let is_ascii: bool = true;
        let i: usize = 0;
        while (i < s.len) : (i += 1) {
            if (s[i] >= 0x80) {
                is_ascii = false;
                break;
            }
        }
        if (is_ascii) {
            putLiteralString(s)?;
            return;
        }

        // Encode UTF-8 -> UTF-16BE with BOM `<FEFF...>`
        putSlice("<FEFF")?;
        i = 0;
        while (i < s.len) {
            const b0: u32 = s[i] as u32;
            let cp: u32 = 0xFFFD;
            if (b0 < 0x80) {
                cp = b0;
                i += 1;
            } else if ((b0 & 0xE0) == 0xC0 && i + 1 < s.len) {
                cp = ((b0 & 0x1F) << 6) | ((s[i + 1] as u32) & 0x3F);
                i += 2;
            } else if ((b0 & 0xF0) == 0xE0 && i + 2 < s.len) {
                cp = ((b0 & 0x0F) << 12) | (((s[i + 1] as u32) & 0x3F) << 6) | ((s[i + 2] as u32) & 0x3F);
                i += 3;
            } else if ((b0 & 0xF8) == 0xF0 && i + 3 < s.len) {
                cp = ((b0 & 0x07) << 18) | (((s[i + 1] as u32) & 0x3F) << 12)
                    | (((s[i + 2] as u32) & 0x3F) << 6) | ((s[i + 3] as u32) & 0x3F);
                i += 4;
            } else {
                i += 1;
            }

            if (cp <= 0xFFFF) {
                putHexByte(((cp >> 8) & 0xFF) as u8)?;
                putHexByte((cp & 0xFF) as u8)?;
            } else {
                const v: u32 = cp - 0x10000;
                const hi: u32 = (v >> 10) + 0xD800;
                const lo: u32 = (v & 0x3FF) + 0xDC00;
                putHexByte(((hi >> 8) & 0xFF) as u8)?;
                putHexByte((hi & 0xFF) as u8)?;
                putHexByte(((lo >> 8) & 0xFF) as u8)?;
                putHexByte((lo & 0xFF) as u8)?;
            }
        }
        putByte('>')?;
    }
}

fn approxCharWidth(font: PdfFont, size: f64): f64 {
    if (font == PdfFont.Courier) return size * 0.60;
    if (font == PdfFont.HelveticaBold) return size * 0.56;
    return size * 0.52;
}

/// Zero-heap caller-slice PDF page content stream builder supporting rich auto-wrapped text,
/// headers/footers, structured tables, vector paths (lines/rects/rounded rects/polygons/dashes),
/// and embedded Image XObject placement.
pub struct PdfPageCanvas {
    w: SliceBuf = SliceBuf{},

    static fn init(buf: []u8): PdfPageCanvas {
        return PdfPageCanvas{ w: SliceBuf.init(buf) };
    }

    fn reset(*_) {
        w.pos = 0;
    }

    fn bytes(): []const u8 {
        return w.buf[0..w.pos];
    }

    fn saveState(*_): void ! PdfWriteError {
        w.putSlice("q\n")?;
    }

    fn restoreState(*_): void ! PdfWriteError {
        w.putSlice("Q\n")?;
    }

    fn setFillRgb(*_, c: PdfRgb): void ! PdfWriteError {
        w.putUnitClamped(c.r)?;
        w.putByte(' ')?;
        w.putUnitClamped(c.g)?;
        w.putByte(' ')?;
        w.putUnitClamped(c.b)?;
        w.putSlice(" rg\n")?;
    }

    fn setStrokeRgb(*_, c: PdfRgb): void ! PdfWriteError {
        w.putUnitClamped(c.r)?;
        w.putByte(' ')?;
        w.putUnitClamped(c.g)?;
        w.putByte(' ')?;
        w.putUnitClamped(c.b)?;
        w.putSlice(" RG\n")?;
    }

    fn setLineWidth(*_, width: f64): void ! PdfWriteError {
        if (!std.math.isFinite(width) || width < 0.0) return PdfWriteError.InvalidArgument;
        w.putNum(width)?;
        w.putSlice(" w\n")?;
    }

    fn setDashPattern(*_, dash: f64, gap: f64): void ! PdfWriteError {
        if (!std.math.isFinite(dash) || !std.math.isFinite(gap) || dash < 0.0 || gap < 0.0) {
            return PdfWriteError.InvalidArgument;
        }
        w.putByte('[')?;
        w.putNum(dash)?;
        w.putByte(' ')?;
        w.putNum(gap)?;
        w.putSlice("] 0 d\n")?;
    }

    fn clearDashPattern(*_): void ! PdfWriteError {
        w.putSlice("[] 0 d\n")?;
    }

    fn drawLine(*_, x1: f64, y1: f64, x2: f64, y2: f64): void ! PdfWriteError {
        w.putNum(x1)?;
        w.putByte(' ')?;
        w.putNum(y1)?;
        w.putSlice(" m\n")?;
        w.putNum(x2)?;
        w.putByte(' ')?;
        w.putNum(y2)?;
        w.putSlice(" l\nS\n")?;
    }

    fn drawRect(*_, x: f64, y: f64, rw: f64, rh: f64): void ! PdfWriteError {
        w.putNum(x)?;
        w.putByte(' ')?;
        w.putNum(y)?;
        w.putByte(' ')?;
        w.putNum(rw)?;
        w.putByte(' ')?;
        w.putNum(rh)?;
        w.putSlice(" re\nS\n")?;
    }

    fn fillRect(*_, x: f64, y: f64, rw: f64, rh: f64): void ! PdfWriteError {
        w.putNum(x)?;
        w.putByte(' ')?;
        w.putNum(y)?;
        w.putByte(' ')?;
        w.putNum(rw)?;
        w.putByte(' ')?;
        w.putNum(rh)?;
        w.putSlice(" re\nf\n")?;
    }

    fn fillStrokeRect(*_, x: f64, y: f64, rw: f64, rh: f64): void ! PdfWriteError {
        w.putNum(x)?;
        w.putByte(' ')?;
        w.putNum(y)?;
        w.putByte(' ')?;
        w.putNum(rw)?;
        w.putByte(' ')?;
        w.putNum(rh)?;
        w.putSlice(" re\nB\n")?;
    }

    /// Draws an arbitrary-vertex polygon or polyline path.
    fn drawPolygon(*_, pts: []const PdfPoint, close_path: bool, fill: bool): void ! PdfWriteError {
        if (pts.len < 2) return PdfWriteError.InvalidArgument;
        w.putNum(pts[0].x)?;
        w.putByte(' ')?;
        w.putNum(pts[0].y)?;
        w.putSlice(" m\n")?;
        let i: usize = 1;
        while (i < pts.len) : (i += 1) {
            w.putNum(pts[i].x)?;
            w.putByte(' ')?;
            w.putNum(pts[i].y)?;
            w.putSlice(" l\n")?;
        }
        if (close_path) w.putSlice("h\n")?;
        if (fill) {
            w.putSlice("B\n")?;
        } else {
            w.putSlice("S\n")?;
        }
    }

    /// Draws a rounded rectangle using cubic Bézier corner arcs (`kappa = 0.55228475`).
    fn drawRoundedRect(*_, x: f64, y: f64, rw: f64, rh: f64, radius: f64, fill: bool): void ! PdfWriteError {
        if (rw <= 0.0 || rh <= 0.0 || radius < 0.0) return PdfWriteError.InvalidArgument;
        let r: f64 = radius;
        if (r > rw * 0.5) r = rw * 0.5;
        if (r > rh * 0.5) r = rh * 0.5;
        const k: f64 = r * 0.55228475;

        // Start bottom edge after bottom-left arc
        w.putNum(x + r)?;
        w.putByte(' ')?;
        w.putNum(y)?;
        w.putSlice(" m\n")?;

        // Bottom edge -> bottom-right arc
        w.putNum(x + rw - r)?;
        w.putByte(' ')?;
        w.putNum(y)?;
        w.putSlice(" l\n")?;
        w.putNum(x + rw - r + k)?;
        w.putByte(' ')?;
        w.putNum(y)?;
        w.putByte(' ')?;
        w.putNum(x + rw)?;
        w.putByte(' ')?;
        w.putNum(y + r - k)?;
        w.putByte(' ')?;
        w.putNum(x + rw)?;
        w.putByte(' ')?;
        w.putNum(y + r)?;
        w.putSlice(" c\n")?;

        // Right edge -> top-right arc
        w.putNum(x + rw)?;
        w.putByte(' ')?;
        w.putNum(y + rh - r)?;
        w.putSlice(" l\n")?;
        w.putNum(x + rw)?;
        w.putByte(' ')?;
        w.putNum(y + rh - r + k)?;
        w.putByte(' ')?;
        w.putNum(x + rw - r + k)?;
        w.putByte(' ')?;
        w.putNum(y + rh)?;
        w.putByte(' ')?;
        w.putNum(x + rw - r)?;
        w.putByte(' ')?;
        w.putNum(y + rh)?;
        w.putSlice(" c\n")?;

        // Top edge -> top-left arc
        w.putNum(x + r)?;
        w.putByte(' ')?;
        w.putNum(y + rh)?;
        w.putSlice(" l\n")?;
        w.putNum(x + r - k)?;
        w.putByte(' ')?;
        w.putNum(y + rh)?;
        w.putByte(' ')?;
        w.putNum(x)?;
        w.putByte(' ')?;
        w.putNum(y + rh - r + k)?;
        w.putByte(' ')?;
        w.putNum(x)?;
        w.putByte(' ')?;
        w.putNum(y + rh - r)?;
        w.putSlice(" c\n")?;

        // Left edge -> bottom-left arc
        w.putNum(x)?;
        w.putByte(' ')?;
        w.putNum(y + r)?;
        w.putSlice(" l\n")?;
        w.putNum(x)?;
        w.putByte(' ')?;
        w.putNum(y + r - k)?;
        w.putByte(' ')?;
        w.putNum(x + r - k)?;
        w.putByte(' ')?;
        w.putNum(y)?;
        w.putByte(' ')?;
        w.putNum(x + r)?;
        w.putByte(' ')?;
        w.putNum(y)?;
        w.putSlice(" c\nh\n")?;

        if (fill) {
            w.putSlice("B\n")?;
        } else {
            w.putSlice("S\n")?;
        }
    }

    fn drawText(*_, x: f64, y: f64, font: PdfFont, size: f64, text: []const u8): void ! PdfWriteError {
        if (!std.math.isFinite(size) || size <= 0.0) return PdfWriteError.InvalidArgument;
        w.putSlice("BT\n")?;
        w.putSlice(font.resourceName())?;
        w.putByte(' ')?;
        w.putNum(size)?;
        w.putSlice(" Tf\n")?;
        w.putNum(x)?;
        w.putByte(' ')?;
        w.putNum(y)?;
        w.putSlice(" Td\n")?;
        w.putLiteralString(text)?;
        w.putSlice(" Tj\nET\n")?;
    }

    /// Word-wraps `text` into lines fitting `max_width` with `align` (`left` / `center` / `right`)
    /// and custom `line_height`. Returns the `y` coordinate below the final rendered line.
    fn drawWrappedText(
        *_,
        x: f64,
        top_y: f64,
        max_width: f64,
        font: PdfFont,
        size: f64,
        line_height: f64,
        text_align: PdfTextAlign,
        text: []const u8,
    ): f64 ! PdfWriteError {
        if (max_width <= 0.0 || size <= 0.0 || line_height <= 0.0) return PdfWriteError.InvalidArgument;
        const cw: f64 = approxCharWidth(font, size);
        let max_chars: usize = @as(usize, @intFromFloat(max_width / cw));
        if (max_chars == 0) max_chars = 1;

        let cur_y: f64 = top_y;
        let pos: usize = 0;
        while (pos < text.len) {
            // Find next explicit newline or wrap boundary
            let line_end: usize = pos;
            let last_space: usize = pos;
            let found_space: bool = false;
            while (line_end < text.len && text[line_end] != '\n' && (line_end - pos) < max_chars) {
                if (text[line_end] == ' ') {
                    last_space = line_end;
                    found_space = true;
                }
                line_end += 1;
            }

            let next_pos: usize = line_end;
            if (line_end < text.len && text[line_end] == '\n') {
                next_pos = line_end + 1;
            } else if (line_end < text.len && found_space && last_space > pos) {
                line_end = last_space;
                next_pos = last_space + 1;
            }

            const line = text[pos..line_end];
            const line_w: f64 = (line.len as f64) * cw;
            let draw_x: f64 = x;
            if (text_align == PdfTextAlign.center && line_w < max_width) {
                draw_x = x + (max_width - line_w) * 0.5;
            } else if (text_align == PdfTextAlign.right && line_w < max_width) {
                draw_x = x + (max_width - line_w);
            }
            if (line.len > 0) {
                drawText(draw_x, cur_y, font, size, line)?;
            }
            cur_y -= line_height;
            pos = next_pos;
        }
        return cur_y;
    }

    /// Renders a page header and numbered footer (`"<footer_prefix> <page_num>/<total_pages>"`).
    fn drawHeaderFooter(
        *_,
        page_size: PageSize,
        margins: PdfMargins,
        header_text: []const u8,
        footer_prefix: []const u8,
        page_num: usize,
        total_pages: usize,
        font: PdfFont,
        size: f64,
    ): void ! PdfWriteError {
        if (header_text.len > 0) {
            const hy: f64 = page_size.height - (margins.top * 0.55);
            drawText(margins.left, hy, font, size, header_text)?;
        }
        let fbuf: [128]u8 = [0; 128];
        let fb = SliceBuf.init(fbuf[0..]);
        if (footer_prefix.len > 0) {
            fb.putSlice(footer_prefix)?;
            fb.putByte(' ')?;
        }
        fb.putUsize(page_num)?;
        fb.putByte('/')?;
        fb.putUsize(total_pages)?;
        const fy: f64 = margins.bottom * 0.45;
        drawText(margins.left, fy, font, size, fb.buf[0..fb.pos])?;
    }

    /// Renders a structured table with header background fill, grid borders, and padded cells.
    /// `rows` is a flat `[num_rows * col_widths.len][]const u8` slice. Returns the `y` cursor below the table.
    fn drawTable(
        *_,
        x: f64,
        top_y: f64,
        col_widths: []const f64,
        headers: []const []const u8,
        rows: []const []const u8,
        num_rows: usize,
        style: PdfTableStyle,
    ): f64 ! PdfWriteError {
        const ncols: usize = col_widths.len;
        if (ncols == 0 || headers.len < ncols || rows.len < num_rows * ncols) {
            return PdfWriteError.InvalidArgument;
        }

        let total_w: f64 = 0.0;
        let ci: usize = 0;
        while (ci < ncols) : (ci += 1) {
            total_w += col_widths[ci];
        }

        setLineWidth(style.border_width)?;
        setStrokeRgb(style.border_color)?;

        let cur_y: f64 = top_y;
        // 1. Header row background fill + border + bold text
        const hdr_bottom: f64 = cur_y - style.row_height;
        setFillRgb(style.header_fill)?;
        fillStrokeRect(x, hdr_bottom, total_w, style.row_height)?;

        setFillRgb(style.text_color)?;
        let cx: f64 = x;
        ci = 0;
        while (ci < ncols) : (ci += 1) {
            if (ci > 0) drawLine(cx, cur_y, cx, hdr_bottom)?;
            const ty: f64 = hdr_bottom + (style.row_height - style.font_size) * 0.4;
            drawText(cx + style.cell_padding_x, ty, style.header_font, style.font_size, headers[ci])?;
            cx += col_widths[ci];
        }
        cur_y = hdr_bottom;

        // 2. Data rows
        let r: usize = 0;
        while (r < num_rows) : (r += 1) {
            const row_bottom: f64 = cur_y - style.row_height;
            setStrokeRgb(style.border_color)?;
            drawRect(x, row_bottom, total_w, style.row_height)?;

            setFillRgb(style.text_color)?;
            cx = x;
            ci = 0;
            while (ci < ncols) : (ci += 1) {
                if (ci > 0) drawLine(cx, cur_y, cx, row_bottom)?;
                const ty: f64 = row_bottom + (style.row_height - style.font_size) * 0.4;
                drawText(cx + style.cell_padding_x, ty, style.font, style.font_size, rows[r * ncols + ci])?;
                cx += col_widths[ci];
            }
            cur_y = row_bottom;
        }

        return cur_y;
    }

    /// Places embedded Image XObject `img_index` (0-based -> `/Im1`, `/Im2`, ...) at `(x, y)` sized `(iw, ih)`.
    fn drawImageXObject(*_, img_index: usize, x: f64, y: f64, iw: f64, ih: f64): void ! PdfWriteError {
        if (iw <= 0.0 || ih <= 0.0) return PdfWriteError.InvalidArgument;
        saveState()?;
        w.putNum(iw)?;
        w.putSlice(" 0 0 ")?;
        w.putNum(ih)?;
        w.putByte(' ')?;
        w.putNum(x)?;
        w.putByte(' ')?;
        w.putNum(y)?;
        w.putSlice(" cm\n/Im")?;
        w.putUsize(img_index + 1)?;
        w.putSlice(" Do\n")?;
        restoreState()?;
    }
}

/// Computes the exact minimum `xref_offsets_work` slice length required by `writePdfDocument`.
pub fn pdfWriterXrefWorkLen(
    num_pages: usize,
    num_images: usize,
    num_bookmarks: usize,
    has_metadata: bool,
): usize {
    const base: usize = 7; // 0(free) + 1(Catalog) + 2(Pages) + 3..6(Base14 Fonts)
    const bm_objs: usize = if (num_bookmarks > 0) (num_bookmarks + 1) else 0;
    const info_objs: usize = if (has_metadata) 1 else 0;
    return base + num_images + num_pages * 2 + bm_objs + info_objs;
}

/// Serializes a complete PDF 1.5 document (multi-page, optional `/FlateDecode` stream compression,
/// embedded Image XObjects, `/Outlines` bookmarks, and UTF-16BE-capable `/Info` metadata) into
/// caller-provided `out_buf` with zero dynamic heap allocation for PDF structure/XRef assembly.
pub fn writePdfDocument(
    pages: []const PdfPageSpec,
    images: []const PdfImageSpec,
    bookmarks: []const PdfBookmarkSpec,
    meta: PdfMetadataSpec,
    compress_streams: bool,
    xref_offsets_work: []usize,
    out_buf: []u8,
): usize ! PdfWriteError {
    const has_meta: bool = meta.title.len > 0 || meta.author.len > 0 || meta.subject.len > 0
        || meta.keywords.len > 0 || meta.creator.len > 0 || meta.producer.len > 0;
    const need_xref_len: usize = pdfWriterXrefWorkLen(pages.len, images.len, bookmarks.len, has_meta);
    if (xref_offsets_work.len < need_xref_len) {
        return PdfWriteError.OutputSliceTooSmall;
    }

    // Validate images & bookmarks upfront
    let ii: usize = 0;
    while (ii < images.len) : (ii += 1) {
        const img = images[ii];
        const expected: usize = @as(usize, img.width) * @as(usize, img.height) * img.color_space.channels();
        if (img.width == 0 || img.height == 0 || img.pixels.len != expected) {
            return PdfWriteError.InvalidImage;
        }
    }
    let bi: usize = 0;
    while (bi < bookmarks.len) : (bi += 1) {
        if (bookmarks[bi].page_index >= pages.len) {
            return PdfWriteError.InvalidPageIndex;
        }
    }

    // Object ID layout:
    //   1: Catalog
    //   2: Pages
    //   3..6: Base14 Fonts (/F1../F4)
    //   img_base .. img_base + images.len - 1: Image XObjects
    //   page_base .. page_base + 2*pages.len - 1: Page + Content Stream pairs
    //   outlines_root (+ bookmarks.len items)
    //   optional info_obj
    const img_base: usize = 7;
    const page_base: usize = img_base + images.len;
    const outlines_root: usize = page_base + pages.len * 2;
    const bm_objs: usize = if (bookmarks.len > 0) (bookmarks.len + 1) else 0;
    const info_obj: usize = outlines_root + bm_objs;
    const total_objs: usize = need_xref_len - 1;

    let w = SliceBuf.init(out_buf);
    xref_offsets_work[0] = 0;

    // Header (PDF 1.5 with binary marker)
    w.putSlice("%PDF-1.5\n%\xE2\xE3\xCF\xD3\n")?;

    // 1 0 obj: Catalog
    xref_offsets_work[1] = w.pos;
    w.putSlice("1 0 obj\n<< /Type /Catalog /Pages 2 0 R")?;
    if (bookmarks.len > 0) {
        w.putSlice(" /Outlines ")?;
        w.putUsize(outlines_root)?;
        w.putSlice(" 0 R")?;
    }
    w.putSlice(" >>\nendobj\n")?;

    // 2 0 obj: Pages
    xref_offsets_work[2] = w.pos;
    w.putSlice("2 0 obj\n<< /Type /Pages /Kids [")?;
    let pi: usize = 0;
    while (pi < pages.len) : (pi += 1) {
        if (pi > 0) w.putByte(' ')?;
        w.putUsize(page_base + pi * 2)?;
        w.putSlice(" 0 R")?;
    }
    w.putSlice("] /Count ")?;
    w.putUsize(pages.len)?;
    w.putSlice(" >>\nendobj\n")?;

    // 3..6: Base14 Fonts
    const fonts: [4]PdfFont = [PdfFont.Helvetica, PdfFont.HelveticaBold, PdfFont.Courier, PdfFont.TimesRoman];
    let fi: usize = 0;
    while (fi < 4) : (fi += 1) {
        const oid = 3 + fi;
        xref_offsets_work[oid] = w.pos;
        w.putUsize(oid)?;
        w.putSlice(" 0 obj\n<< /Type /Font /Subtype /Type1 /BaseFont ")?;
        w.putSlice(fonts[fi].baseFontName())?;
        w.putSlice(" >>\nendobj\n")?;
    }

    // Image XObjects
    ii = 0;
    while (ii < images.len) : (ii += 1) {
        const img = images[ii];
        const oid = img_base + ii;
        xref_offsets_work[oid] = w.pos;
        w.putUsize(oid)?;
        w.putSlice(" 0 obj\n<< /Type /XObject /Subtype /Image /Width ")?;
        w.putUsize(@as(usize, img.width))?;
        w.putSlice(" /Height ")?;
        w.putUsize(@as(usize, img.height))?;
        w.putSlice(" /ColorSpace ")?;
        w.putSlice(img.color_space.pdfName())?;
        w.putSlice(" /BitsPerComponent 8")?;

        if (img.compress || compress_streams) {
            const comp = deflate(img.pixels, 6);
            if (comp.len == 0 && img.pixels.len > 0) return PdfWriteError.CompressFailed;
            w.putSlice(" /Filter /FlateDecode /Length ")?;
            w.putUsize(comp.len)?;
            w.putSlice(" >>\nstream\n")?;
            w.putSlice(comp)?;
            w.putSlice("\nendstream\nendobj\n")?;
        } else {
            w.putSlice(" /Length ")?;
            w.putUsize(img.pixels.len)?;
            w.putSlice(" >>\nstream\n")?;
            w.putSlice(img.pixels)?;
            w.putSlice("\nendstream\nendobj\n")?;
        }
    }

    // Pages + Content Streams
    pi = 0;
    while (pi < pages.len) : (pi += 1) {
        const p = pages[pi];
        const page_oid = page_base + pi * 2;
        const content_oid = page_oid + 1;

        xref_offsets_work[page_oid] = w.pos;
        w.putUsize(page_oid)?;
        w.putSlice(" 0 obj\n<< /Type /Page /Parent 2 0 R /MediaBox [0 0 ")?;
        w.putNum(p.size.width)?;
        w.putByte(' ')?;
        w.putNum(p.size.height)?;
        w.putByte(']')?;
        if (p.rotate != 0) {
            w.putSlice(" /Rotate ")?;
            w.putI32(p.rotate)?;
        }
        w.putSlice(" /Resources << /Font << /F1 3 0 R /F2 4 0 R /F3 5 0 R /F4 6 0 R >>")?;
        if (images.len > 0) {
            w.putSlice(" /XObject <<")?;
            let xi: usize = 0;
            while (xi < images.len) : (xi += 1) {
                w.putSlice(" /Im")?;
                w.putUsize(xi + 1)?;
                w.putByte(' ')?;
                w.putUsize(img_base + xi)?;
                w.putSlice(" 0 R")?;
            }
            w.putSlice(" >>")?;
        }
        w.putSlice(" >> /Contents ")?;
        w.putUsize(content_oid)?;
        w.putSlice(" 0 R >>\nendobj\n")?;

        // Content Stream (raw or FlateDecode)
        xref_offsets_work[content_oid] = w.pos;
        w.putUsize(content_oid)?;
        if (compress_streams && p.content_stream.len > 0) {
            const comp = deflate(p.content_stream, 6);
            if (comp.len == 0) return PdfWriteError.CompressFailed;
            w.putSlice(" 0 obj\n<< /Filter /FlateDecode /Length ")?;
            w.putUsize(comp.len)?;
            w.putSlice(" >>\nstream\n")?;
            w.putSlice(comp)?;
            w.putSlice("\nendstream\nendobj\n")?;
        } else {
            w.putSlice(" 0 obj\n<< /Length ")?;
            w.putUsize(p.content_stream.len)?;
            w.putSlice(" >>\nstream\n")?;
            w.putSlice(p.content_stream)?;
            w.putSlice("\nendstream\nendobj\n")?;
        }
    }

    // Optional `/Outlines` Bookmark Chain
    if (bookmarks.len > 0) {
        const first_bm = outlines_root + 1;
        const last_bm = outlines_root + bookmarks.len;
        xref_offsets_work[outlines_root] = w.pos;
        w.putUsize(outlines_root)?;
        w.putSlice(" 0 obj\n<< /Type /Outlines /First ")?;
        w.putUsize(first_bm)?;
        w.putSlice(" 0 R /Last ")?;
        w.putUsize(last_bm)?;
        w.putSlice(" 0 R /Count ")?;
        w.putUsize(bookmarks.len)?;
        w.putSlice(" >>\nendobj\n")?;

        bi = 0;
        while (bi < bookmarks.len) : (bi += 1) {
            const oid = first_bm + bi;
            const bm = bookmarks[bi];
            const target_page_oid = page_base + bm.page_index * 2;
            xref_offsets_work[oid] = w.pos;
            w.putUsize(oid)?;
            w.putSlice(" 0 obj\n<< /Title ")?;
            w.putPdfTextString(bm.title)?;
            w.putSlice(" /Parent ")?;
            w.putUsize(outlines_root)?;
            w.putSlice(" 0 R /Dest [")?;
            w.putUsize(target_page_oid)?;
            w.putSlice(" 0 R /Fit]")?;
            if (bi > 0) {
                w.putSlice(" /Prev ")?;
                w.putUsize(oid - 1)?;
                w.putSlice(" 0 R")?;
            }
            if (bi + 1 < bookmarks.len) {
                w.putSlice(" /Next ")?;
                w.putUsize(oid + 1)?;
                w.putSlice(" 0 R")?;
            }
            w.putSlice(" >>\nendobj\n")?;
        }
    }

    // Optional `/Info` Metadata Dictionary (UTF-16BE capable)
    if (has_meta) {
        xref_offsets_work[info_obj] = w.pos;
        w.putUsize(info_obj)?;
        w.putSlice(" 0 obj\n<<")?;
        if (meta.title.len > 0) {
            w.putSlice(" /Title ")?;
            w.putPdfTextString(meta.title)?;
        }
        if (meta.author.len > 0) {
            w.putSlice(" /Author ")?;
            w.putPdfTextString(meta.author)?;
        }
        if (meta.subject.len > 0) {
            w.putSlice(" /Subject ")?;
            w.putPdfTextString(meta.subject)?;
        }
        if (meta.keywords.len > 0) {
            w.putSlice(" /Keywords ")?;
            w.putPdfTextString(meta.keywords)?;
        }
        if (meta.creator.len > 0) {
            w.putSlice(" /Creator ")?;
            w.putPdfTextString(meta.creator)?;
        }
        if (meta.producer.len > 0) {
            w.putSlice(" /Producer ")?;
            w.putPdfTextString(meta.producer)?;
        }
        w.putSlice(" >>\nendobj\n")?;
    }

    // Cross-Reference Table
    const xref_off = w.pos;
    w.putSlice("xref\n0 ")?;
    w.putUsize(total_objs + 1)?;
    w.putSlice("\n0000000000 65535 f \n")?;
    let oi: usize = 1;
    while (oi <= total_objs) : (oi += 1) {
        w.putXref10(xref_offsets_work[oi])?;
    }

    // Trailer
    w.putSlice("trailer\n<< /Size ")?;
    w.putUsize(total_objs + 1)?;
    w.putSlice(" /Root 1 0 R")?;
    if (has_meta) {
        w.putSlice(" /Info ")?;
        w.putUsize(info_obj)?;
        w.putSlice(" 0 R")?;
    }
    w.putSlice(" >>\nstartxref\n")?;
    w.putUsize(xref_off)?;
    w.putSlice("\n%%EOF\n")?;

    return w.pos;
}
