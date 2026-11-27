// Type definitions for CP

import {  } from "std";
export class ValidationIssue {
    path: string,
    message: string,
    code: string,
}

export class FormatErrorOptions {
    prefix: string,
    prefix_sep: string,
    issue_sep: string,
    max_issues: usize,
    include_path: bool,
}

export class MessageBuilderOptions {
    prefix: string,
    prefix_separator: string,
    issue_separator: string,
    union_separator: string,
    max_issues_in_message: usize,
    include_path: bool,
    include_code: bool,
}

export class ValidationError {
    name: string,
    message_text: string,
    details_count: usize,
    total_issues: usize,
    truncated: bool,
    fn message(): string;
    fn toString(): string;
    fn copyMessage(out: []u8): usize;
    fn issueCount(): usize;
    fn detailsCount(): usize;
    fn isTruncated(): bool;
    fn isValidationError(): bool;
}

export fn defaultFormatErrorOptions(): FormatErrorOptions;
export fn defaultMessageBuilderOptions(): MessageBuilderOptions;
export fn zverWriteStr(out: []u8, pos: usize, s: string): usize;
export fn zverWriteByte(out: []u8, pos: usize, ch: u8): usize;
export fn zverIsIdentChar(ch: u8): bool;
export fn zverIsDigitsOnly(s: string): bool;
export fn zverIsSimpleIdentifier(s: string): bool;
export fn formatIssuePath(segments: &[]string, out: []u8): usize;
export fn formatValidationIssue(issue: ValidationIssue, include_path: bool, include_code: bool, out: []u8): usize;
export fn formatValidationIssues(issues: &[]ValidationIssue, opts: FormatErrorOptions, out: []u8): usize;
export fn createMessage(issues: &[]ValidationIssue, opts: MessageBuilderOptions, out: []u8): usize;
export fn fromZodErrorInto(issues: &[]ValidationIssue, opts: MessageBuilderOptions, out: []u8): ValidationError;
export fn fromZodError(issues: &[]ValidationIssue, opts: MessageBuilderOptions): ValidationError;
export fn fromError(issues: &[]ValidationIssue, opts: MessageBuilderOptions): ValidationError;
export fn fromErrorMessage(msg: string, opts: MessageBuilderOptions): ValidationError;
export fn isValidationError(error_name: string): bool;
export fn isValidationErrorLike(error_name: string, has_details: bool): bool;
