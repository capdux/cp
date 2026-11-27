[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# ejs

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:ejs` ([https://github.com/mde/ejs](https://github.com/mde/ejs))（原库名 `ejs` → CP 包名 `ejs`）

Clean-room MIT, zero-heap Embedded Template Engine for CP — modeled on npm's `ejs` v3.

## Dependencies
- `escape-html` (`1.0.0`) — reused for XSS-safe `<%= ... %>` HTML escaping.

## Supported Tags
- `<%= key %>` — HTML-escaped value interpolation
- `<%- key %>` — raw unescaped output
- `<%# comment %>` — comment (omitted from output)
- `<%%` — literal `<%`
- `<% if key %>...<% else %>...<% end %>` — conditional branching
- `<% unless key %>...<% end %>` — inverted conditional
- `<% repeat key %>...<% end %>` — counted block repetition
- `<%_ ... _%>` / `-%>` — whitespace & newline slurping
