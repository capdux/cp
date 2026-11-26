// @cap/card-bin — Zero-allocation payment card BIN detector, Luhn validator, formatter & PCI-DSS masker for CP
// Clean-room MIT implementation inspired by npm `credit-card-type` (4M/wk) and `card-validator` (3M/wk).

const MAX_CARD_DIGITS: usize = 32;

pub enum CardNetwork {
    Visa,
    Mastercard,
    Amex,
    UnionPay,
    Jcb,
    Discover,
    DinersClub,
    Maestro,
    Unknown,
}

pub struct CardInfo {
    pub network: CardNetwork,
    pub digit_count: usize,
    pub valid_length: bool,
    pub luhn_ok: bool,
    pub is_valid: bool,
    pub cvv_len: u8,
}

struct ExtractResult {
    count: usize,
    ok: bool,
}

fn extractDigits(raw: string, out: *[MAX_CARD_DIGITS]u8) -> ExtractResult {
    let count: usize = 0;
    let i: usize = 0;
    while (i < raw.len) {
        const c = raw[i];
        if (c == ' ' || c == '-') {
            i += 1;
            continue;
        }
        if (c >= '0' && c <= '9') {
            if (count >= MAX_CARD_DIGITS) {
                return ExtractResult{ count: 0 as usize, ok: false };
            }
            out[count] = c;
            count += 1;
            i += 1;
            continue;
        }
        return ExtractResult{ count: 0 as usize, ok: false };
    }
    return ExtractResult{ count: count, ok: true };
}

fn prefixNum(digits: *[MAX_CARD_DIGITS]u8, count: usize, len: usize) -> u32 {
    if (count < len) {
        return 0 as u32;
    }
    let val: u32 = 0;
    let i: usize = 0;
    while (i < len) {
        val = val * (10 as u32) + ((digits[i] - '0') as u32);
        i += 1;
    }
    return val;
}

fn detectNetwork(digits: *[MAX_CARD_DIGITS]u8, count: usize) -> CardNetwork {
    if (count == 0) {
        return CardNetwork.Unknown;
    }
    const p1 = prefixNum(digits, count, 1 as usize);
    const p2 = prefixNum(digits, count, 2 as usize);
    const p3 = prefixNum(digits, count, 3 as usize);
    const p4 = prefixNum(digits, count, 4 as usize);

    // Visa: 4
    if (p1 == 4) {
        return CardNetwork.Visa;
    }
    // Mastercard: 51-55, 2221-2720
    if ((count >= 2 && p2 >= 51 && p2 <= 55) || (count >= 4 && p4 >= 2221 && p4 <= 2720)) {
        return CardNetwork.Mastercard;
    }
    // Amex: 34, 37
    if (count >= 2 && (p2 == 34 || p2 == 37)) {
        return CardNetwork.Amex;
    }
    // UnionPay: 62
    if (count >= 2 && p2 == 62) {
        return CardNetwork.UnionPay;
    }
    // JCB: 3528-3589
    if (count >= 4 && p4 >= 3528 && p4 <= 3589) {
        return CardNetwork.Jcb;
    }
    // Discover: 6011, 644-649, 65
    if ((count >= 4 && p4 == 6011) || (count >= 3 && p3 >= 644 && p3 <= 649) || (count >= 2 && p2 == 65)) {
        return CardNetwork.Discover;
    }
    // DinersClub: 300-305, 36, 38-39
    if ((count >= 3 && p3 >= 300 && p3 <= 305) || (count >= 2 && (p2 == 36 || p2 == 38 || p2 == 39))) {
        return CardNetwork.DinersClub;
    }
    // Maestro: 50, 56-58, 6304, 67
    if ((count >= 2 && (p2 == 50 || (p2 >= 56 && p2 <= 58) || p2 == 67)) || (count >= 4 && p4 == 6304)) {
        return CardNetwork.Maestro;
    }
    return CardNetwork.Unknown;
}

fn checkValidLength(net: CardNetwork, len: usize) -> bool {
    switch (net) {
        CardNetwork.Visa => {
            return len == 13 || len == 16 || len == 19;
        },
        CardNetwork.Mastercard => {
            return len == 16;
        },
        CardNetwork.Amex => {
            return len == 15;
        },
        CardNetwork.UnionPay => {
            return len >= 16 && len <= 19;
        },
        CardNetwork.Jcb => {
            return len >= 16 && len <= 19;
        },
        CardNetwork.Discover => {
            return len >= 16 && len <= 19;
        },
        CardNetwork.DinersClub => {
            return len == 14 || len == 16;
        },
        CardNetwork.Maestro => {
            return len >= 12 && len <= 19;
        },
        CardNetwork.Unknown => {
            return false;
        },
    }
}

fn networkCvvLen(net: CardNetwork) -> u8 {
    switch (net) {
        CardNetwork.Amex => {
            return 4 as u8;
        },
        CardNetwork.Visa => {
            return 3 as u8;
        },
        CardNetwork.Mastercard => {
            return 3 as u8;
        },
        CardNetwork.UnionPay => {
            return 3 as u8;
        },
        CardNetwork.Jcb => {
            return 3 as u8;
        },
        CardNetwork.Discover => {
            return 3 as u8;
        },
        CardNetwork.DinersClub => {
            return 3 as u8;
        },
        CardNetwork.Maestro => {
            return 3 as u8;
        },
        CardNetwork.Unknown => {
            return 3 as u8;
        },
    }
}

fn checkLuhnDigits(digits: *[MAX_CARD_DIGITS]u8, count: usize) -> bool {
    if (count < 2) {
        return false;
    }
    let sum: u32 = 0;
    let doubleIt = false;
    let idx: usize = count;
    while (idx > 0) {
        idx -= 1;
        let d: u32 = (digits[idx] - '0') as u32;
        if (doubleIt) {
            d = d * (2 as u32);
            if (d > 9) {
                d -= 9 as u32;
            }
        }
        sum += d;
        doubleIt = !doubleIt;
    }
    return (sum % (10 as u32)) == (0 as u32);
}

pub fn luhnValid(raw: string) -> bool {
    let digits: [MAX_CARD_DIGITS]u8 = undefined;
    const ex = extractDigits(raw, &digits);
    if (!ex.ok || ex.count < 2) {
        return false;
    }
    return checkLuhnDigits(&digits, ex.count);
}

pub fn inspectCard(raw: string) -> CardInfo {
    let digits: [MAX_CARD_DIGITS]u8 = undefined;
    const ex = extractDigits(raw, &digits);
    if (!ex.ok || ex.count == 0) {
        return CardInfo{
            network: CardNetwork.Unknown,
            digit_count: 0 as usize,
            valid_length: false,
            luhn_ok: false,
            is_valid: false,
            cvv_len: 3 as u8,
        };
    }
    const net = detectNetwork(&digits, ex.count);
    const vlen = checkValidLength(net, ex.count);
    const luhn = checkLuhnDigits(&digits, ex.count);
    const cvv = networkCvvLen(net);
    const valid = (net != CardNetwork.Unknown) && vlen && luhn;
    return CardInfo{
        network: net,
        digit_count: ex.count,
        valid_length: vlen,
        luhn_ok: luhn,
        is_valid: valid,
        cvv_len: cvv,
    };
}

pub fn networkName(n: CardNetwork) -> string {
    switch (n) {
        CardNetwork.Visa => { return "Visa"; },
        CardNetwork.Mastercard => { return "Mastercard"; },
        CardNetwork.Amex => { return "American Express"; },
        CardNetwork.UnionPay => { return "UnionPay"; },
        CardNetwork.Jcb => { return "JCB"; },
        CardNetwork.Discover => { return "Discover"; },
        CardNetwork.DinersClub => { return "Diners Club"; },
        CardNetwork.Maestro => { return "Maestro"; },
        CardNetwork.Unknown => { return "Unknown"; },
    }
}

pub fn formatCardNumber(raw: string, out: []u8) -> string {
    let digits: [MAX_CARD_DIGITS]u8 = undefined;
    const ex = extractDigits(raw, &digits);
    if (!ex.ok || ex.count == 0) {
        return out[0..0];
    }
    const net = detectNetwork(&digits, ex.count);
    let pos: usize = 0;
    let i: usize = 0;
    while (i < ex.count && pos < out.len) {
        const needSpace = if (net == CardNetwork.Amex) {
            (i == 4 || i == 10)
        } else {
            (i > 0 && (i % (4 as usize)) == (0 as usize))
        };
        if (needSpace) {
            if (pos >= out.len) {
                break;
            }
            out[pos] = ' ';
            pos += 1;
            if (pos >= out.len) {
                break;
            }
        }
        out[pos] = digits[i];
        pos += 1;
        i += 1;
    }
    return out[0..pos];
}

pub fn maskCardPci(raw: string, out: []u8) -> string {
    let digits: [MAX_CARD_DIGITS]u8 = undefined;
    const ex = extractDigits(raw, &digits);
    if (!ex.ok || ex.count == 0) {
        return out[0..0];
    }
    const count = ex.count;
    let pos: usize = 0;
    let i: usize = 0;
    while (i < count && pos < out.len) {
        let keep = false;
        if (count > 10) {
            keep = (i < 6) || (i >= count - 4);
        } else if (count > 4) {
            keep = i >= count - 4;
        } else {
            keep = true;
        }
        out[pos] = if (keep) { digits[i] } else { '*' };
        pos += 1;
        i += 1;
    }
    return out[0..pos];
}

export {
    CardNetwork,
    CardInfo,
    luhnValid,
    inspectCard,
    networkName,
    formatCardNumber,
    maskCardPci,
};
