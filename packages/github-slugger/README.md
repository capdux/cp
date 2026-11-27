[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# github-slugger

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:github-slugger` ([https://github.com/Flet/github-slugger](https://github.com/Flet/github-slugger))（原库名 `github-slugger` → CP 包名 `github-slugger`）

Zero-allocation Markdown heading outline extractor, GitHub-compatible anchor slugger & automatic nested TOC generator for CP — clean-room MIT parity with npm `github-slugger` (35M/wk) and `markdown-toc`.

## Install

```bash
cap add github-slugger
```

## Features

- **GitHub Anchor Slugger**: `githubSlug(heading, out)` matches GitHub's heading anchor rules (lowercase ASCII, preserves CJK/UTF-8, strips punctuation, deduplicates repeated headings with `-1`, `-2`…).
- **Code Fence Safe**: ignores `#` comments inside ```` ``` ```` or `~~~` fenced code blocks.
- **Nested Markdown TOC Renderer**: `renderTocMarkdown(toc, min_level, max_level, out)`.

## Usage

```cp
import { extractToc, renderTocMarkdown } from "github-slugger";

const toc = extractToc("# Intro\n## Setup\n## Setup\n");
let out: [1024]u8 = undefined;
const md = renderTocMarkdown(toc, 1, 3, out[0..1024]);
```
