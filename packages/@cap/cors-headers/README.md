# @cap/cors-headers

Clean-room MIT, zero-heap Fetch/W3C CORS policy evaluator and response header builder for CP — modeled on npm's `cors`.

## Dependencies
- `@cap/unicase` (`1.0.0`) — case-insensitive HTTP method/header comparison
- `@cap/wildmatch` (`1.0.0`) — wildcard origin matching (e.g., `https://*.example.com`)

## API
- `CorsOptions` (`permissive`, `restrictive`, `addOrigin`)
- `isOriginAllowed(opts, origin)` & `isMethodAllowed(allowed_methods, method)`
- `evaluateCors(opts, method, origin, ac_req_method, ac_req_headers): CorsDecision`
- `formatCorsHeaders(decision, out): usize`
