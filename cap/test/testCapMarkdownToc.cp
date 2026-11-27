// cap/test/testCapMarkdownToc.cp
// Comprehensive unit tests for @cap/markdown-toc

import { test, expect_eq } from "cap:test";
import {
    githubSlug,
    extractToc,
    renderTocMarkdown,
} from "../../packages/github-slugger/src/index.cp";

fn main() {
    test("1. @cap/markdown-toc — GitHub anchor slug generation (ASCII & CJK)", () => {
        let buf: [128]u8 = undefined;
        expect_eq(githubSlug("Hello World!", buf[0..128]), "hello-world");
        expect_eq(githubSlug("API v2.0: Quick Start (Beta)", buf[0..128]), "api-v20-quick-start-beta");
        expect_eq(githubSlug("快速开始 Guide", buf[0..128]), "快速开始-guide");
    });

    test("2. @cap/markdown-toc — Heading extraction & duplicate slug deduplication", () => {
        const doc = "# Capdux\n## Install\n### Usage\n## Install ###\n";
        const toc = extractToc(doc);
        expect_eq(toc.count, 4 as usize);
        expect_eq(toc.levelAt(0 as usize), 1 as u8);
        expect_eq(toc.textAt(0 as usize), "Capdux");
        expect_eq(toc.slugAt(0 as usize), "capdux");
        expect_eq(toc.slugAt(1 as usize), "install");
        expect_eq(toc.slugAt(2 as usize), "usage");
        expect_eq(toc.textAt(3 as usize), "Install");
        expect_eq(toc.slugAt(3 as usize), "install-1");
        expect_eq(toc.lineAt(3 as usize), 4 as usize);
    });

    test("3. @cap/markdown-toc — Ignores # comments inside fenced code blocks", () => {
        const doc = "# Real Heading\n```bash\n# Not a heading\n## Also ignored\n```\n## Second Real\n";
        const toc = extractToc(doc);
        expect_eq(toc.count, 2 as usize);
        expect_eq(toc.textAt(0 as usize), "Real Heading");
        expect_eq(toc.textAt(1 as usize), "Second Real");
    });

    test("4. @cap/markdown-toc — Nested Markdown TOC rendering with level filter", () => {
        const doc = "# Title\n## Overview\n### Architecture\n#### Deep Detail\n## Usage\n";
        const toc = extractToc(doc);
        let out: [512]u8 = undefined;
        const rendered = renderTocMarkdown(toc, 2 as u8, 3 as u8, out[0..512]);
        expect_eq(rendered, "- [Overview](#overview)\n  - [Architecture](#architecture)\n- [Usage](#usage)");
    });
}
