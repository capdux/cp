[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/finalhandler

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:finalhandler` ([https://github.com/pillarjs/finalhandler](https://github.com/pillarjs/finalhandler))

Clean-room MIT, zero-heap HTTP final response & error handler for CP — modeled on npm's `finalhandler` v2 (140M+/week).

## Dependencies
- `@cap/escape-html` (`1.0.0`) — XSS-safe HTML escaping for error messages
- `@cap/http-types` (`1.0.0`) — canonical HTTP status reason phrases
- `@cap/itoa` (`1.0.0`) — fast zero-heap status code & Content-Length formatting

## API
- `normalizeErrorStatus(code): u16`
- `statusReason(code): string`
- `formatErrorHtml(message, out): usize`
- `formatCannotMessage(method, url, out): usize`
- `buildFinalHttpResponse(status_code, message, is_head, out): FinalResponse`
