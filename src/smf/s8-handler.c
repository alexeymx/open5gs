#include "s8-handler.h"
#include "gtp-path.h"
#include "pfcp-path.h"
#include "context.h"

int smf_s8_handle_create_session_request(ogs_gtp_xact_t *xact,
    smf_sess_t *sess, ogs_gtp_create_session_request_t *req)
{
    int rv;
    ogs_gtp_message_t gtp_message;
    ogs_gtp_create_session_response_t *rsp = NULL;

    ogs_assert(xact);
    ogs_assert(sess);
    ogs_assert(req);

    /* Create PFCP session */
    rv = smf_pfcp_send_session_establishment_request(sess);
    if (rv != OGS_OK) {
        ogs_error("smf_pfcp_send_session_establishment_request() failed");
        return OGS_ERROR;
    }

    /* Build response */
    memset(&gtp_message, 0, sizeof(ogs_gtp_message_t));
    rsp = &gtp_message.create_session_response;
    rsp->cause.presence = 1;
    rsp->cause.u8 = OGS_GTP_CAUSE_REQUEST_ACCEPTED;

    /* Send response */
    rv = ogs_gtp_xact_commit(xact, &gtp_message);
    ogs_assert(rv == OGS_OK);

    return OGS_OK;
}

int smf_s8_handle_create_session_response(ogs_gtp_xact_t *xact,
    smf_sess_t *sess, ogs_gtp_create_session_response_t *rsp)
{
    ogs_assert(xact);
    ogs_assert(sess);
    ogs_assert(rsp);

    /* Handle response from peer SMF */
    if (rsp->cause.presence && rsp->cause.u8 == OGS_GTP_CAUSE_REQUEST_ACCEPTED) {
        /* Session established successfully */
        ogs_info("S8 Create Session Response accepted");
    } else {
        ogs_error("S8 Create Session Response rejected");
        return OGS_ERROR;
    }

    return OGS_OK;
}

int smf_s8_handle_modify_bearer_request(ogs_gtp_xact_t *xact,
    smf_sess_t *sess, ogs_gtp_modify_bearer_request_t *req)
{
    int rv;
    ogs_gtp_message_t gtp_message;
    ogs_gtp_modify_bearer_response_t *rsp = NULL;

    ogs_assert(xact);
    ogs_assert(sess);
    ogs_assert(req);

    /* Update PFCP session if needed */
    if (req->bearer_contexts_to_be_modified.presence) {
        rv = smf_pfcp_send_session_modification_request(sess);
        if (rv != OGS_OK) {
            ogs_error("smf_pfcp_send_session_modification_request() failed");
            return OGS_ERROR;
        }
    }

    /* Build response */
    memset(&gtp_message, 0, sizeof(ogs_gtp_message_t));
    rsp = &gtp_message.modify_bearer_response;
    rsp->cause.presence = 1;
    rsp->cause.u8 = OGS_GTP_CAUSE_REQUEST_ACCEPTED;

    /* Send response */
    rv = ogs_gtp_xact_commit(xact, &gtp_message);
    ogs_assert(rv == OGS_OK);

    return OGS_OK;
}

int smf_s8_handle_modify_bearer_response(ogs_gtp_xact_t *xact,
    smf_sess_t *sess, ogs_gtp_modify_bearer_response_t *rsp)
{
    ogs_assert(xact);
    ogs_assert(sess);
    ogs_assert(rsp);

    /* Handle response from peer SMF */
    if (rsp->cause.presence && rsp->cause.u8 == OGS_GTP_CAUSE_REQUEST_ACCEPTED) {
        /* Bearer modification successful */
        ogs_info("S8 Modify Bearer Response accepted");
    } else {
        ogs_error("S8 Modify Bearer Response rejected");
        return OGS_ERROR;
    }

    return OGS_OK;
}

int smf_s8_handle_delete_session_request(ogs_gtp_xact_t *xact,
    smf_sess_t *sess, ogs_gtp_delete_session_request_t *req)
{
    int rv;
    ogs_gtp_message_t gtp_message;
    ogs_gtp_delete_session_response_t *rsp = NULL;

    ogs_assert(xact);
    ogs_assert(sess);
    ogs_assert(req);

    /* Delete PFCP session */
    rv = smf_pfcp_send_session_deletion_request(sess);
    if (rv != OGS_OK) {
        ogs_error("smf_pfcp_send_session_deletion_request() failed");
        return OGS_ERROR;
    }

    /* Build response */
    memset(&gtp_message, 0, sizeof(ogs_gtp_message_t));
    rsp = &gtp_message.delete_session_response;
    rsp->cause.presence = 1;
    rsp->cause.u8 = OGS_GTP_CAUSE_REQUEST_ACCEPTED;

    /* Send response */
    rv = ogs_gtp_xact_commit(xact, &gtp_message);
    ogs_assert(rv == OGS_OK);

    return OGS_OK;
}

int smf_s8_handle_delete_session_response(ogs_gtp_xact_t *xact,
    smf_sess_t *sess, ogs_gtp_delete_session_response_t *rsp)
{
    ogs_assert(xact);
    ogs_assert(sess);
    ogs_assert(rsp);

    /* Handle response from peer SMF */
    if (rsp->cause.presence && rsp->cause.u8 == OGS_GTP_CAUSE_REQUEST_ACCEPTED) {
        /* Session deleted successfully */
        ogs_info("S8 Delete Session Response accepted");
    } else {
        ogs_error("S8 Delete Session Response rejected");
        return OGS_ERROR;
    }

    return OGS_OK;
}

int smf_s8_handle_bearer_resource_command(ogs_gtp_xact_t *xact,
    smf_sess_t *sess, ogs_gtp_bearer_resource_command_t *cmd)
{
    int rv;
    ogs_gtp_message_t gtp_message;
    ogs_gtp_bearer_resource_failure_indication_t *ind = NULL;

    ogs_assert(xact);
    ogs_assert(sess);
    ogs_assert(cmd);

    /* Process bearer resource command */
    if (cmd->linked_eps_bearer_id.presence) {
        /* Update bearer resources */
        rv = smf_pfcp_send_session_modification_request(sess);
        if (rv != OGS_OK) {
            ogs_error("smf_pfcp_send_session_modification_request() failed");
            return OGS_ERROR;
        }
    }

    /* Build failure indication if needed */
    memset(&gtp_message, 0, sizeof(ogs_gtp_message_t));
    ind = &gtp_message.bearer_resource_failure_indication;
    ind->cause.presence = 1;
    ind->cause.u8 = OGS_GTP_CAUSE_REQUEST_ACCEPTED;

    /* Send response */
    rv = ogs_gtp_xact_commit(xact, &gtp_message);
    ogs_assert(rv == OGS_OK);

    return OGS_OK;
}

int smf_s8_handle_bearer_resource_failure_indication(ogs_gtp_xact_t *xact,
    smf_sess_t *sess, ogs_gtp_bearer_resource_failure_indication_t *ind)
{
    ogs_assert(xact);
    ogs_assert(sess);
    ogs_assert(ind);

    /* Handle failure indication from peer SMF */
    if (ind->cause.presence) {
        ogs_error("S8 Bearer Resource Failure Indication received: cause=%d",
            ind->cause.u8);
    }

    return OGS_OK;
} 