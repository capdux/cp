# @cap/path-to-regexp

Clean-room MIT, zero-heap Express/Router URL path template matching, parameter extraction, and reverse route compilation engine for CP — modeled on npm `path-to-regexp` v8.

## Dependencies
- `@cap/unicase` (`1.0.0`) — reused for optional case-insensitive static segment matching (`eqAscii`).

## API

### Structs
- `RouteParam { name: string, value: string }`
  - `RouteParam.new(name, value): RouteParam`
- `RouteMatch { matched: bool, params: [16]RouteParam, param_count: usize }`
  - `RouteMatch.noMatch(): RouteMatch`
  - `getParam(name: string): string?` — retrieves an extracted parameter value by name.

### Functions
- `matchPath(pattern: string, path: string, case_sensitive: bool): RouteMatch`
  - Supports static segments (`/users/profile`), named parameters (`/users/:id/posts/:postId`), optional parameters (`/files/:name?`), wildcard rest segments (`/assets/*filepath`, `/assets/*filepath?`), trailing-slash tolerance, and case-sensitive or ASCII case-insensitive matching.
- `compilePath(pattern: string, params: []const RouteParam, out: []u8): usize`
  - Reverse-compiles a route pattern into `out` by substituting `:name` and `*wildcard` slots from `params`. Skips omitted optional parameters and returns `0` if any required parameter is missing.
- `countRouteParams(pattern: string): usize`
  - Counts dynamic parameter and wildcard slots in a route pattern.
