// Type definitions for CP

export enum CardNetwork {
    Visa,
    Mastercard,
    Amex,
    UnionPay,
    Jcb,
    Discover,
    DinersClub,
    Maestro,
    Unknown
}

export class CardInfo {
    network: CardNetwork,
    digit_count: usize,
    valid_length: bool,
    luhn_ok: bool,
    is_valid: bool,
    cvv_len: u8,
}

export class ExtractResult {
    count: usize,
    ok: bool,
}

export fn extractDigits(raw: string, out: &mut [MAX_CARD_DIGITS]u8): ExtractResult;
export fn prefixNum(digits: &mut [MAX_CARD_DIGITS]u8, count: usize, len: usize): u32;
export fn detectNetwork(digits: &mut [MAX_CARD_DIGITS]u8, count: usize): CardNetwork;
export fn checkValidLength(net: CardNetwork, len: usize): bool;
export fn networkCvvLen(net: CardNetwork): u8;
export fn checkLuhnDigits(digits: &mut [MAX_CARD_DIGITS]u8, count: usize): bool;
export fn luhnValid(raw: string): bool;
export fn inspectCard(raw: string): CardInfo;
export fn networkName(n: CardNetwork): string;
export fn formatCardNumber(raw: string, out: []u8): string;
export fn maskCardPci(raw: string, out: []u8): string;
