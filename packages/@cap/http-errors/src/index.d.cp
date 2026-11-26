// Type definitions for CP

export class HttpError {
    status: u16,
    code: string,
    title: string,
    detail: string,
    instance: string,
    expose: bool,
    fn withInstance(instance: string): HttpError;
    fn withExpose(expose: bool): HttpError;
    fn withCode(code: string): HttpError;
}

export fn statusTitle(status: u16): string;
export fn statusCodeName(status: u16): string;
export fn createHttpError(status: u16, detail: string): HttpError;
export fn badRequest(detail: string): HttpError;
export fn unauthorized(detail: string): HttpError;
export fn forbidden(detail: string): HttpError;
export fn notFound(detail: string): HttpError;
export fn methodNotAllowed(detail: string): HttpError;
export fn conflict(detail: string): HttpError;
export fn gone(detail: string): HttpError;
export fn payloadTooLarge(detail: string): HttpError;
export fn unsupportedMediaType(detail: string): HttpError;
export fn unprocessableEntity(detail: string): HttpError;
export fn tooManyRequests(detail: string): HttpError;
export fn internalServerError(detail: string): HttpError;
export fn notImplemented(detail: string): HttpError;
export fn badGateway(detail: string): HttpError;
export fn serviceUnavailable(detail: string): HttpError;
export fn gatewayTimeout(detail: string): HttpError;
export fn isClientError(err: HttpError): bool;
export fn isServerError(err: HttpError): bool;
export fn isRetryable(err: HttpError): bool;
export fn appendByte(out: []u8, pos: usize, c: u8): usize;
export fn appendStr(out: []u8, pos: usize, s: string): usize;
export fn hexDigitLower(v: u8): u8;
export fn appendJsonEscaped(out: []u8, start_pos: usize, s: string): usize;
export fn appendU16(out: []u8, pos: usize, val: u16): usize;
export fn toProblemJson(err: HttpError, include_hidden_detail: bool, out: []u8): string;
