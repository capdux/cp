// Source: Absorbed from /data/dev/bun/src/md/root.cp | Synced: 2026-11-20T11:08:48Z
// lib/md/engine.cp
// High-Level Entry Points for the Ported Bun MD4C Engine (HTML, ANSI & Custom Callback Renderer)

const std = @import("std");
import { AnsiRenderer, AnsiTheme } from "./ansiRenderer.cp";
import { processDoc } from "./blocks.cp";
import { skipUtf8Bom } from "./helpers.cp";
import { HtmlRenderer } from "./htmlRenderer.cp";
import { processInlineContent } from "./inlines.cp";
import { MdFlags, Md4cParserState, RenderOptions, Renderer } from "./types.cp";

pub struct MdEngineOptions {
    tables: bool = true,
    strikethrough: bool = true,
    tasklists: bool = true,
    permissive_autolinks: bool = false,
    permissive_url_autolinks: bool = false,
    permissive_www_autolinks: bool = false,
    permissive_email_autolinks: bool = false,
    hard_soft_breaks: bool = false,
    wiki_links: bool = false,
    underline: bool = false,
    latex_math: bool = false,
    collapse_whitespace: bool = false,
    permissive_atx_headers: bool = false,
    no_indented_code_blocks: bool = false,
    no_html_blocks: bool = false,
    no_html_spans: bool = false,
    tag_filter: bool = false,
    heading_ids: bool = false,
    autolink_headings: bool = false,
    sanitize_html: bool = false,

    pub static fn commonmark(): MdEngineOptions {
        return MdEngineOptions{
            tables: false,
            strikethrough: false,
            tasklists: false,
        };
    }

    pub static fn github(): MdEngineOptions {
        return MdEngineOptions{
            tables: true,
            strikethrough: true,
            tasklists: true,
            permissive_autolinks: true,
            permissive_www_autolinks: true,
            permissive_email_autolinks: true,
            tag_filter: true,
            heading_ids: true,
        };
    }

    pub static fn terminal(): MdEngineOptions {
        return MdEngineOptions{
            tables: true,
            strikethrough: true,
            tasklists: true,
            permissive_url_autolinks: true,
            permissive_www_autolinks: true,
            permissive_email_autolinks: true,
            wiki_links: true,
            underline: true,
            latex_math: true,
        };
    }

    pub fn toFlags(): MdFlags {
        return MdFlags{
            tables: _.tables,
            strikethrough: _.strikethrough,
            tasklists: _.tasklists,
            permissive_url_autolinks: _.permissive_url_autolinks || _.permissive_autolinks,
            permissive_www_autolinks: _.permissive_www_autolinks || _.permissive_autolinks,
            permissive_email_autolinks: _.permissive_email_autolinks || _.permissive_autolinks,
            hard_soft_breaks: _.hard_soft_breaks,
            wiki_links: _.wiki_links,
            underline: _.underline,
            latex_math: _.latex_math,
            collapse_whitespace: _.collapse_whitespace,
            permissive_atx_headers: _.permissive_atx_headers,
            no_indented_code_blocks: _.no_indented_code_blocks,
            no_html_blocks: _.no_html_blocks,
            no_html_spans: _.no_html_spans,
        };
    }

    pub fn toRenderOptions(): RenderOptions {
        return RenderOptions{
            tag_filter: _.tag_filter,
            heading_ids: _.heading_ids,
            autolink_headings: _.autolink_headings,
            sanitize_html: _.sanitize_html,
        };
    }
}

pub fn renderToHtml(text: []const u8, allocator: std.mem.Allocator): ([]u8)! {
    return renderToHtmlWithOptions(text, allocator, MdEngineOptions{});
}

pub fn renderToHtmlWithOptions(text: []const u8, allocator: std.mem.Allocator, options: MdEngineOptions): ([]u8)! {
    const input = skipUtf8Bom(text);
    let html_renderer = HtmlRenderer.init(allocator, input, options.toRenderOptions());
    defer html_renderer.deinit();

    let p = Md4cParserState.init(allocator, input, options.toFlags(), html_renderer.renderer());
    zig { p.process_inline_ptr = @intFromPtr(&processInlineContent); }
    defer p.deinit();

    processDoc(&p)?;
    return html_renderer.toOwnedSlice();
}

pub fn renderWithRenderer(
    text: []const u8,
    allocator: std.mem.Allocator,
    options: MdEngineOptions,
    rend: Renderer,
): ! {
    const input = skipUtf8Bom(text);
    let p = Md4cParserState.init(allocator, input, options.toFlags(), rend);
    zig { p.process_inline_ptr = @intFromPtr(&processInlineContent); }
    defer p.deinit();
    processDoc(&p)?;
}

pub fn renderToAnsi(
    text: []const u8,
    allocator: std.mem.Allocator,
    options: MdEngineOptions,
    theme: AnsiTheme,
): ([]u8)! {
    const input = skipUtf8Bom(text);
    let rend = AnsiRenderer.init(allocator, input, theme);
    defer rend.deinit();

    renderWithRenderer(input, allocator, options, rend.renderer())?;
    return rend.toOwnedSlice();
}
