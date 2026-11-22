// Source: cp/lib/webauthn/index.cp (cap:webauthn & cap.webauthn FIDO2 / WebAuthn Level 3 server verification standard library)
export {
    WebAuthnError,
    AuthFlags,
    AttestedCredData,
    AuthenticatorData,
    ClientData,
    ExpectedClientData,
    RegistrationResult,
    AssertionResult,
} from "./types.cp";

export { parseAuthenticatorData } from "./authData.cp";

export {
    encodeChallengeB64u,
    constantTimeEql,
    parseClientDataJson,
    verifyClientData,
} from "./clientData.cp";

export {
    computeRpIdHash,
    computeClientDataHash,
    verifyRegistrationNone,
    verifyAssertion,
} from "./verify.cp";
