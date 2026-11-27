# credit-card-type@1.0.0 (Agent Quickref)

> Zero-allocation payment card BIN network detector, Luhn validator, formatter, and PCI-DSS masker for CP | License: MIT
> Import: `import { ... } from "credit-card-type";` | Full types: `src/index.d.cp`

## Types & Constants
- `export enum CardNetwork`

## Functions
- `export fn luhnValid(raw: string): bool;`
- `export fn inspectCard(raw: string): CardInfo;`
- `export fn networkName(n: CardNetwork): string;`
- `export fn formatCardNumber(raw: string, out: []u8): string;`
- `export fn maskCardPci(raw: string, out: []u8): string;`
