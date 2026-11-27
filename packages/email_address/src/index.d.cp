// Type definitions for CP

export class EmailAddress {
    local_part: string,
    domain: string,
    display_name: string,
    valid: bool,
}

export fn emIsAtext(b: u8): bool;
export fn emIsLabelChar(b: u8): bool;
export fn emTrim(s: string): string;
export fn emValidLocal(local: string): bool;
export fn emValidDomain(domain: string): bool;
export fn parseEmail(input: string): EmailAddress;
export fn isValidEmail(input: string): bool;
