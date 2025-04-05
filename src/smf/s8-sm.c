#include "s8-handler.h"
#include "gtp-path.h"
#include "pfcp-path.h"
#include "context.h"

void smf_s8_state_initial(ogs_fsm_t *s, smf_event_t *e)
{
    smf_sm_debug(e);

    ogs_assert(s);

    OGS_FSM_TRAN(s, &smf_s8_state_operational);
}

void smf_s8_state_final(ogs_fsm_t *s, smf_event_t *e)
{
    smf_sm_debug(e);

    ogs_assert(s);
}

void smf_s8_state_wait_create_session_response(ogs_fsm_t *s, smf_event_t *e)
{
    smf_sess_t *sess = NULL;
    ogs_gtp_xact_t *xact = NULL;

    smf_sm_debug(e);

    ogs_assert(s);
    sess = e->sess;
    ogs_assert(sess);
    xact = e->gtp_xact;
    ogs_assert(xact);

    switch (e->id) {
        case SMF_EVT_S8_MESSAGE:
            switch (e->gtp_message.h.type) {
                case OGS_GTP_CREATE_SESSION_RESPONSE_TYPE:
                    smf_s8_handle_create_session_response(xact, sess,
                        &e->gtp_message.create_session_response);
                    OGS_FSM_TRAN(s, &smf_s8_state_operational);
                    break;
                default:
                    ogs_error("Unexpected message type: %d",
                        e->gtp_message.h.type);
                    OGS_FSM_TRAN(s, &smf_s8_state_exception);
                    break;
            }
            break;
        case SMF_EVT_S8_TIMER:
            ogs_error("S8 Create Session Response timeout");
            OGS_FSM_TRAN(s, &smf_s8_state_exception);
            break;
        default:
            ogs_error("Unexpected event: %d", e->id);
            OGS_FSM_TRAN(s, &smf_s8_state_exception);
            break;
    }
}

void smf_s8_state_wait_modify_bearer_response(ogs_fsm_t *s, smf_event_t *e)
{
    smf_sess_t *sess = NULL;
    ogs_gtp_xact_t *xact = NULL;

    smf_sm_debug(e);

    ogs_assert(s);
    sess = e->sess;
    ogs_assert(sess);
    xact = e->gtp_xact;
    ogs_assert(xact);

    switch (e->id) {
        case SMF_EVT_S8_MESSAGE:
            switch (e->gtp_message.h.type) {
                case OGS_GTP_MODIFY_BEARER_RESPONSE_TYPE:
                    smf_s8_handle_modify_bearer_response(xact, sess,
                        &e->gtp_message.modify_bearer_response);
                    OGS_FSM_TRAN(s, &smf_s8_state_operational);
                    break;
                default:
                    ogs_error("Unexpected message type: %d",
                        e->gtp_message.h.type);
                    OGS_FSM_TRAN(s, &smf_s8_state_exception);
                    break;
            }
            break;
        case SMF_EVT_S8_TIMER:
            ogs_error("S8 Modify Bearer Response timeout");
            OGS_FSM_TRAN(s, &smf_s8_state_exception);
            break;
        default:
            ogs_error("Unexpected event: %d", e->id);
            OGS_FSM_TRAN(s, &smf_s8_state_exception);
            break;
    }
}

void smf_s8_state_wait_delete_session_response(ogs_fsm_t *s, smf_event_t *e)
{
    smf_sess_t *sess = NULL;
    ogs_gtp_xact_t *xact = NULL;

    smf_sm_debug(e);

    ogs_assert(s);
    sess = e->sess;
    ogs_assert(sess);
    xact = e->gtp_xact;
    ogs_assert(xact);

    switch (e->id) {
        case SMF_EVT_S8_MESSAGE:
            switch (e->gtp_message.h.type) {
                case OGS_GTP_DELETE_SESSION_RESPONSE_TYPE:
                    smf_s8_handle_delete_session_response(xact, sess,
                        &e->gtp_message.delete_session_response);
                    OGS_FSM_TRAN(s, &smf_s8_state_final);
                    break;
                default:
                    ogs_error("Unexpected message type: %d",
                        e->gtp_message.h.type);
                    OGS_FSM_TRAN(s, &smf_s8_state_exception);
                    break;
            }
            break;
        case SMF_EVT_S8_TIMER:
            ogs_error("S8 Delete Session Response timeout");
            OGS_FSM_TRAN(s, &smf_s8_state_exception);
            break;
        default:
            ogs_error("Unexpected event: %d", e->id);
            OGS_FSM_TRAN(s, &smf_s8_state_exception);
            break;
    }
}

void smf_s8_state_operational(ogs_fsm_t *s, smf_event_t *e)
{
    smf_sess_t *sess = NULL;
    ogs_gtp_xact_t *xact = NULL;

    smf_sm_debug(e);

    ogs_assert(s);
    sess = e->sess;
    ogs_assert(sess);
    xact = e->gtp_xact;
    ogs_assert(xact);

    switch (e->id) {
        case SMF_EVT_S8_MESSAGE:
            switch (e->gtp_message.h.type) {
                case OGS_GTP_CREATE_SESSION_REQUEST_TYPE:
                    smf_s8_handle_create_session_request(xact, sess,
                        &e->gtp_message.create_session_request);
                    OGS_FSM_TRAN(s, &smf_s8_state_wait_create_session_response);
                    break;
                case OGS_GTP_MODIFY_BEARER_REQUEST_TYPE:
                    smf_s8_handle_modify_bearer_request(xact, sess,
                        &e->gtp_message.modify_bearer_request);
                    OGS_FSM_TRAN(s, &smf_s8_state_wait_modify_bearer_response);
                    break;
                case OGS_GTP_DELETE_SESSION_REQUEST_TYPE:
                    smf_s8_handle_delete_session_request(xact, sess,
                        &e->gtp_message.delete_session_request);
                    OGS_FSM_TRAN(s, &smf_s8_state_wait_delete_session_response);
                    break;
                case OGS_GTP_BEARER_RESOURCE_COMMAND_TYPE:
                    smf_s8_handle_bearer_resource_command(xact, sess,
                        &e->gtp_message.bearer_resource_command);
                    break;
                case OGS_GTP_BEARER_RESOURCE_FAILURE_INDICATION_TYPE:
                    smf_s8_handle_bearer_resource_failure_indication(xact, sess,
                        &e->gtp_message.bearer_resource_failure_indication);
                    break;
                default:
                    ogs_error("Unexpected message type: %d",
                        e->gtp_message.h.type);
                    OGS_FSM_TRAN(s, &smf_s8_state_exception);
                    break;
            }
            break;
        default:
            ogs_error("Unexpected event: %d", e->id);
            OGS_FSM_TRAN(s, &smf_s8_state_exception);
            break;
    }
}

void smf_s8_state_exception(ogs_fsm_t *s, smf_event_t *e)
{
    smf_sm_debug(e);

    ogs_assert(s);

    switch (e->id) {
        case SMF_EVT_S8_MESSAGE:
            ogs_error("Unexpected message type: %d", e->gtp_message.h.type);
            break;
        default:
            ogs_error("Unexpected event: %d", e->id);
            break;
    }
} 