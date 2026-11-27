// Type definitions for CP

import { isAlnum, trim } from "cap:ascii";
import { writeStr } from "cap:fmt";
export class MailAddress {
    valid: bool,
    display_name: string,
    address: string,
    local: string,
    domain: string,
}

export class AddressList {
    items: [MAX_ADDRESSES]MailAddress,
    count: usize,
    fn get(idx: usize): MailAddress?;
}

export fn invalidMail(): MailAddress;
export fn stripEdgeComments(raw: string): string;
export fn isValidLocal(local: string): bool;
export fn isValidDomain(domain: string): bool;
export fn makeAddrFromSpec(displayName: string, addrSpec: string): MailAddress;
export fn unwrapDisplayName(rawName: string): string;
export fn parseMailbox(raw: string): MailAddress;
export fn parseAddressList(raw: string): AddressList;
export fn appendByte(out: []u8, pos: usize, c: u8): usize;
export fn needsQuoting(name: string): bool;
export fn formatMailbox(m: MailAddress, out: []u8): string;
