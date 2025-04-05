#ifndef S8_HANDLER_H
#define S8_HANDLER_H

#include "ogs-gtp.h"
#include "context.h"

#ifdef __cplusplus
extern "C" {
#endif

int smf_s8_handle_create_session_request(ogs_gtp_xact_t *xact,
    smf_sess_t *sess, ogs_gtp_create_session_request_t *req);
int smf_s8_handle_create_session_response(ogs_gtp_xact_t *xact,
    smf_sess_t *sess, ogs_gtp_create_session_response_t *rsp);
int smf_s8_handle_modify_bearer_request(ogs_gtp_xact_t *xact,
    smf_sess_t *sess, ogs_gtp_modify_bearer_request_t *req);
int smf_s8_handle_modify_bearer_response(ogs_gtp_xact_t *xact,
    smf_sess_t *sess, ogs_gtp_modify_bearer_response_t *rsp);
int smf_s8_handle_delete_session_request(ogs_gtp_xact_t *xact,
    smf_sess_t *sess, ogs_gtp_delete_session_request_t *req);
int smf_s8_handle_delete_session_response(ogs_gtp_xact_t *xact,
    smf_sess_t *sess, ogs_gtp_delete_session_response_t *rsp);
int smf_s8_handle_bearer_resource_command(ogs_gtp_xact_t *xact,
    smf_sess_t *sess, ogs_gtp_bearer_resource_command_t *cmd);
int smf_s8_handle_bearer_resource_failure_indication(ogs_gtp_xact_t *xact,
    smf_sess_t *sess, ogs_gtp_bearer_resource_failure_indication_t *ind);

#ifdef __cplusplus
}
#endif

#endif /* S8_HANDLER_H */ 