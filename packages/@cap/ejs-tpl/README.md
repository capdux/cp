# @cap/ejs-tpl

Clean-room MIT, zero-heap Embedded Template Engine for CP — modeled on npm's `ejs` v3.

## Dependencies
- `@cap/escape-html` (`1.0.0`) — reused for XSS-safe `<%= ... %>` HTML escaping.

## Supported Tags
- `<%= key %>` — HTML-escaped value interpolation
- `<%- key %>` — raw unescaped output
- `<%# comment %>` — comment (omitted from output)
- `<%%` — literal `<%`
- `<% if key %>...<% else %>...<% end %>` — conditional branching
- `<% unless key %>...<% end %>` — inverted conditional
- `<% repeat key %>...<% end %>` — counted block repetition
- `<%_ ... _%>` / `-%>` — whitespace & newline slurping
