#include <string.h>
#include "srtp.h"

int cp_srtp_init(void) {
    return (int)srtp_init();
}

int cp_srtp_shutdown(void) {
    return (int)srtp_shutdown();
}

const char *cp_srtp_version(void) {
    return srtp_get_version_string();
}

void *cp_srtp_create_aes128_sha1(const unsigned char *key30, int inbound) {
    if (!key30) return NULL;
    srtp_policy_t policy;
    memset(&policy, 0, sizeof(policy));
    srtp_crypto_policy_set_rtp_default(&policy.rtp);
    srtp_crypto_policy_set_rtcp_default(&policy.rtcp);
    policy.ssrc.type = inbound ? ssrc_any_inbound : ssrc_any_outbound;
    policy.ssrc.value = 0;
    policy.key = (unsigned char *)key30;
    policy.next = NULL;

    srtp_t session = NULL;
    if (srtp_create(&session, &policy) != srtp_err_status_ok) {
        return NULL;
    }
    return (void *)session;
}

int cp_srtp_protect(void *session, void *buf, int *len_inout) {
    if (!session || !buf || !len_inout) return (int)srtp_err_status_bad_param;
    return (int)srtp_protect((srtp_t)session, buf, len_inout);
}

int cp_srtp_unprotect(void *session, void *buf, int *len_inout) {
    if (!session || !buf || !len_inout) return (int)srtp_err_status_bad_param;
    return (int)srtp_unprotect((srtp_t)session, buf, len_inout);
}

void cp_srtp_destroy(void *session) {
    if (session) {
        srtp_dealloc((srtp_t)session);
    }
}
