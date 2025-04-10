/* Gx Interface, 3GPP TS 29.212 section 4
 * Copyright (C) 2019-2024 by Sukchan Lee <acetcom@gmail.com>
 * Copyright (C) 2022 by sysmocom - s.f.m.c. GmbH <info@sysmocom.de>
 *
 * This file is part of Open5GS.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "context.h"
#include "gtp-path.h"
#include "pfcp-path.h"
#include "fd-path.h"
#include "gx-handler.h"
#include "binding.h"
#include <stdlib.h>
#include <string.h>

/* Returns ER_DIAMETER_SUCCESS on success, Diameter error code on failue. */

uint32_t smf_gx_handle_cca_initial_request(
        smf_sess_t *sess, ogs_diam_gx_message_t *gx_message,
        ogs_gtp_xact_t *gtp_xact)
{
    int i;
    smf_bearer_t *bearer = NULL;
    ogs_pfcp_pdr_t *dl_pdr = NULL;
    ogs_pfcp_pdr_t *ul_pdr = NULL;
    ogs_pfcp_far_t *dl_far = NULL;
    ogs_pfcp_far_t *up2cp_far = NULL;
    ogs_pfcp_pdr_t *cp2up_pdr = NULL;
    ogs_pfcp_pdr_t *up2cp_pdr = NULL;


    bool is_ims_apn = false;

    ogs_assert(sess);
    ogs_assert(gx_message);
    ogs_assert(gtp_xact);

    ogs_info("[PGW] Create Session Response");
    ogs_info("    SGW_S5C_TEID[0x%x] PGW_S5C_TEID[0x%x]",
            sess->sgw_s5c_teid, sess->smf_n4_teid);

    /* Check if this is IMS APN */
    if (sess->session.name &&
        (ogs_strcasecmp(sess->session.name, "ims") == 0)) {
        is_ims_apn = true;
    }

    /* For non-IMS APNs, verify rating groups */
    if (!is_ims_apn) {
        if (gx_message->result_code != ER_DIAMETER_SUCCESS) {
            ogs_error("Gx CCA-I failed with result code %d", gx_message->result_code);
            return gx_message->err ? *gx_message->err :
                                   ER_DIAMETER_AUTHENTICATION_REJECTED;
        }


        /* First pass: Collect unique rating groups and their rules */
        for (i = 0; i < gx_message->session_data.num_of_pcc_rule; i++) {
            ogs_pcc_rule_t *pcc_rule = &gx_message->session_data.pcc_rule[i];

            if (pcc_rule->rating_group) {
                ogs_pfcp_pdr_t *rg_pdr = ogs_pfcp_pdr_add(&sess->pfcp);
        		ogs_assert(rg_pdr);
				rg_pdr->id = pcc_rule->rating_group;
				rg_pdr->src_if = OGS_PFCP_INTERFACE_ACCESS; // or CORE, depending on direction
		        //ogs_pfcp_paa_to_ue_ip_addr(&sess->paa, &rg_pdr->ue_ip_addr, &rg_pdr->ue_ip_addr_len);
		        for (int j = 0; j < pcc_rule->num_of_flow; j++) {
                  ogs_flow_t *flow = &pcc_rule->flow[j];
		          rg_pdr->flow[rg_pdr->num_of_flow].fd = j + 1;
		          rg_pdr->flow[rg_pdr->num_of_flow].description = flow->description;

		          rg_pdr->num_of_flow++;
                }
				// Create a new FAR for this PDR
        		ogs_pfcp_far_t *rg_far = ogs_pfcp_far_add(&sess->pfcp);
        		ogs_assert(rg_far);
        		rg_far->apply_action = OGS_PFCP_APPLY_ACTION_FORW;
        		ogs_pfcp_pdr_associate_far(rg_pdr, rg_far);

        		// Create a new QER for this PDR
        		ogs_pfcp_qer_t *rg_qer = ogs_pfcp_qer_add(&sess->pfcp);
        		ogs_assert(rg_qer);
        		// Configure QER parameters here
        		ogs_pfcp_pdr_associate_qer(rg_pdr, rg_qer);

        		// Create a new URR for this PDR
        		ogs_pfcp_urr_t *rg_urr = ogs_pfcp_urr_add(&sess->pfcp);
				// Add RG URR messure types
				rg_urr->meas_method = OGS_PFCP_MEASUREMENT_METHOD_VOLUME |
                              OGS_PFCP_MEASUREMENT_METHOD_DURATION;



            	/* Configure volume thresholds based on PCC rules */
            	rg_urr->vol_threshold.flags = OGS_PFCP_VOLUME_MEASUREMENT_TYPE;
            	rg_urr->vol_threshold.total_volume = 100000;  /* Will be set by PCC rules */
            	rg_urr->vol_threshold.downlink_volume = 100000;  /* Will be set by PCC rules */
            	rg_urr->vol_threshold.uplink_volume = 100000;  /* Will be set by PCC rules */

            	/* Configure time threshold */
            	rg_urr->time_threshold = 3600;  /* 1 hour in seconds  3600 for now 60 */

            	/* Set reporting triggers */
            	rg_urr->rep_triggers.volume_threshold = 1;
            	rg_urr->rep_triggers.time_threshold = 1;


        		ogs_assert(rg_urr);
        		// Configure URR parameters here
        		ogs_pfcp_pdr_associate_urr(rg_pdr, rg_urr);

        		ogs_info("Configured PDR, FAR, QER, and URR for PCC Rule[%s]", pcc_rule->name);


            }
        }

    }

    /* Store PCC rules */
    sess->policy.num_of_pcc_rule = gx_message->session_data.num_of_pcc_rule;
    for (i = 0; i < gx_message->session_data.num_of_pcc_rule; i++) {
        OGS_STORE_PCC_RULE(&sess->policy.pcc_rule[i],
                &gx_message->session_data.pcc_rule[i]);

        /* Debug log for PCC rule details */
        ogs_debug("PCC Rule[%d] Details:", i);
        ogs_debug("  - Name: %s", gx_message->session_data.pcc_rule[i].name);
        ogs_debug("  - Rating Group: %d", gx_message->session_data.pcc_rule[i].rating_group);
        ogs_debug("  - Flow Status: %d", gx_message->session_data.pcc_rule[i].flow_status);

        /* Log flow information */
        if (gx_message->session_data.pcc_rule[i].num_of_flow > OGS_MAX_NUM_OF_FLOW) {
            ogs_error("Too many flows in PCC rule");
            return ER_DIAMETER_UNABLE_TO_COMPLY;
        }

        if (gx_message->session_data.pcc_rule[i].num_of_flow) {
            int j;
            for (j = 0; j < gx_message->session_data.pcc_rule[i].num_of_flow; j++) {
                ogs_info("  - Flow[%d]: %s (dir:%d)",
                    j,
                    gx_message->session_data.pcc_rule[i].flow[j].description,
                    gx_message->session_data.pcc_rule[i].flow[j].direction);
            }
        }
    }

    /* Get default bearer */
    bearer = smf_default_bearer_in_sess(sess);
    ogs_assert(bearer);

    /* Create CP/UP data forwarding */
    smf_sess_create_cp_up_data_forwarding(sess);


    /* Initialize PDRs and FARs from bearer */
    dl_pdr = bearer->dl_pdr;
    ul_pdr = bearer->ul_pdr;
    dl_far = bearer->dl_far;
    up2cp_far = sess->up2cp_far;

    ogs_assert(dl_pdr);
    ogs_assert(ul_pdr);

    //veirfy
    cp2up_pdr = sess->cp2up_pdr;
    ogs_assert(cp2up_pdr);
    up2cp_pdr = sess->up2cp_pdr;
    ogs_assert(up2cp_pdr);
    //end veirfy
    ogs_assert(up2cp_far);


    /* Configure FARs */
    if (dl_far) {
        dl_far->apply_action = OGS_PFCP_APPLY_ACTION_FORW;
        ogs_info("    Configured DL FAR with FORWARD action");
    }

    /// Veirfy
    /* Set Outer Header Creation to the Default DL FAR */
    ogs_assert(OGS_OK ==
        ogs_pfcp_ip_to_outer_header_creation(
            &bearer->sgw_s5u_ip,
            &dl_far->outer_header_creation,
            &dl_far->outer_header_creation_len));
    dl_far->outer_header_creation.teid = bearer->sgw_s5u_teid;


    /* Set UE IP Address to the Default DL PDR */
    ogs_assert(OGS_OK ==
        ogs_pfcp_paa_to_ue_ip_addr(&sess->paa,
            &dl_pdr->ue_ip_addr, &dl_pdr->ue_ip_addr_len));
    dl_pdr->ue_ip_addr.sd = OGS_PFCP_UE_IP_DST;

    ogs_assert(OGS_OK ==
        ogs_pfcp_paa_to_ue_ip_addr(&sess->paa,
            &ul_pdr->ue_ip_addr, &ul_pdr->ue_ip_addr_len));

    /* Set UE-to-CP Flow-Description and Outer-Header-Creation */
    up2cp_pdr->flow[up2cp_pdr->num_of_flow].fd = 1;
    up2cp_pdr->flow[up2cp_pdr->num_of_flow].description =
        (char *)"permit out 58 from ff02::2/128 to assigned";
    up2cp_pdr->num_of_flow++;

    ogs_assert(OGS_OK ==
        ogs_pfcp_ip_to_outer_header_creation(
            &ogs_gtp_self()->gtpu_ip,
            &up2cp_far->outer_header_creation,
            &up2cp_far->outer_header_creation_len));
    up2cp_far->outer_header_creation.teid = sess->index;

    /* Set F-TEID */
    ogs_assert(sess->pfcp_node);
    if (sess->pfcp_node->up_function_features.ftup) {
        ul_pdr->f_teid.ipv4 = 1;
        ul_pdr->f_teid.ipv6 = 1;
        ul_pdr->f_teid.ch = 1;
        ul_pdr->f_teid.chid = 1;
        ul_pdr->f_teid.choose_id = OGS_PFCP_DEFAULT_CHOOSE_ID;
        ul_pdr->f_teid_len = 2;

        cp2up_pdr->f_teid.ipv4 = 1;
        cp2up_pdr->f_teid.ipv6 = 1;
        cp2up_pdr->f_teid.ch = 1;
        cp2up_pdr->f_teid_len = 1;

        up2cp_pdr->f_teid.ipv4 = 1;
        up2cp_pdr->f_teid.ipv6 = 1;
        up2cp_pdr->f_teid.ch = 1;
        up2cp_pdr->f_teid.chid = 1;
        up2cp_pdr->f_teid.choose_id = OGS_PFCP_DEFAULT_CHOOSE_ID;
        up2cp_pdr->f_teid_len = 2;
    } else {
        ogs_gtpu_resource_t *resource = NULL;
        resource = ogs_pfcp_find_gtpu_resource(
                &sess->pfcp_node->gtpu_resource_list,
                sess->session.name, ul_pdr->src_if);
        if (resource) {
            ogs_user_plane_ip_resource_info_to_sockaddr(&resource->info,
                &bearer->pgw_s5u_addr, &bearer->pgw_s5u_addr6);
            if (resource->info.teidri)
                bearer->pgw_s5u_teid = OGS_PFCP_GTPU_INDEX_TO_TEID(
                        ul_pdr->teid, resource->info.teidri,
                        resource->info.teid_range);
            else
                bearer->pgw_s5u_teid = ul_pdr->teid;
        } else {
            ogs_assert(sess->pfcp_node->addr_list);
            if (sess->pfcp_node->addr_list->ogs_sa_family == AF_INET)
                ogs_assert(OGS_OK ==
                    ogs_copyaddrinfo(
                        &bearer->pgw_s5u_addr, sess->pfcp_node->addr_list));
            else if (sess->pfcp_node->addr_list->ogs_sa_family == AF_INET6)
                ogs_assert(OGS_OK ==
                    ogs_copyaddrinfo(
                        &bearer->pgw_s5u_addr6, sess->pfcp_node->addr_list));
            else
                ogs_assert_if_reached();

            bearer->pgw_s5u_teid = ul_pdr->teid;
        }

        ogs_assert(OGS_OK ==
            ogs_pfcp_sockaddr_to_f_teid(
                bearer->pgw_s5u_addr, bearer->pgw_s5u_addr6,
                &ul_pdr->f_teid, &ul_pdr->f_teid_len));
        ul_pdr->f_teid.teid = bearer->pgw_s5u_teid;

        ogs_assert(OGS_OK ==
            ogs_pfcp_sockaddr_to_f_teid(
                bearer->pgw_s5u_addr, bearer->pgw_s5u_addr6,
                &cp2up_pdr->f_teid, &cp2up_pdr->f_teid_len));
        cp2up_pdr->f_teid.teid = cp2up_pdr->teid;

        ogs_assert(OGS_OK ==
            ogs_pfcp_sockaddr_to_f_teid(
                bearer->pgw_s5u_addr, bearer->pgw_s5u_addr6,
                &up2cp_pdr->f_teid, &up2cp_pdr->f_teid_len));
        up2cp_pdr->f_teid.teid = bearer->pgw_s5u_teid;
    }

    /// Verify

    if (up2cp_far) {
        up2cp_far->apply_action = OGS_PFCP_APPLY_ACTION_FORW;
        ogs_info("    Configured UP2CP FAR with FORWARD action");
    }
    /// Set Default URR
    ogs_pfcp_urr_t *urr = ogs_pfcp_urr_add(&sess->pfcp);

    urr->meas_method = OGS_PFCP_MEASUREMENT_METHOD_VOLUME |
                  OGS_PFCP_MEASUREMENT_METHOD_DURATION;

    /* Configure volume thresholds based on PCC rules */
    urr->vol_threshold.flags = OGS_PFCP_VOLUME_MEASUREMENT_TYPE;
    urr->vol_threshold.total_volume = 100000;  /* Will be set by PCC rules */
    urr->vol_threshold.downlink_volume = 100000;  /* Will be set by PCC rules */
    urr->vol_threshold.uplink_volume = 100000;  /* Will be set by PCC rules */

    /* Configure Default time threshold */
    urr->time_threshold = 3600;  /* 1 hour in seconds  3600 for now 60 */

    /* Set Default reporting triggers */
    urr->rep_triggers.volume_threshold = 1;
    urr->rep_triggers.time_threshold = 1;

    ogs_assert(urr);
    // Configure URR parameters here

    /// Defaults
    if (ul_pdr) {
        ul_pdr->flow[ul_pdr->num_of_flow].description = malloc(strlen("permit out 6 from any to any") + 1);
        //ul_pdr->flow[ul_pdr->num_of_flow].fd = "2";
        if (ul_pdr->flow[ul_pdr->num_of_flow].description) {
            strcpy(ul_pdr->flow[ul_pdr->num_of_flow].description, "permit out 6 from any to any");
        }

        ul_pdr->num_of_flow++;

        ul_pdr->flow[ul_pdr->num_of_flow].description = malloc(strlen("permit out 17 from any to any") + 1);
        //ul_pdr->flow[ul_pdr->num_of_flow].fd = "2";
        if (ul_pdr->flow[ul_pdr->num_of_flow].description) {
            strcpy(ul_pdr->flow[ul_pdr->num_of_flow].description, "permit out 17 from any to any");
        }

        ul_pdr->num_of_flow++;

        ogs_pfcp_pdr_associate_urr(ul_pdr, urr);
        ogs_info("      - Associated with Default UL PDR");

    }
    if (dl_pdr) {
        dl_pdr->flow[dl_pdr->num_of_flow].description = malloc(strlen("permit out 6 from any to any") + 1);
        //ul_pdr->flow[ul_pdr->num_of_flow].fd = "2";

        if (dl_pdr->flow[dl_pdr->num_of_flow].description) {
            strcpy(dl_pdr->flow[dl_pdr->num_of_flow].description, "permit out 6 from any to any");
        }
        dl_pdr->num_of_flow++;

        dl_pdr->flow[dl_pdr->num_of_flow].description = malloc(strlen("permit out 17 from any to any") + 1);
        //ul_pdr->flow[ul_pdr->num_of_flow].fd = "2";

        if (dl_pdr->flow[dl_pdr->num_of_flow].description) {
            strcpy(dl_pdr->flow[dl_pdr->num_of_flow].description, "permit out 17 from any to any");
        }
        dl_pdr->num_of_flow++;

        ogs_pfcp_pdr_associate_urr(dl_pdr, urr);
        ogs_pfcp_pdr_associate_far(dl_pdr, dl_far);
        ogs_info("      - Associated with Default DL PDR");
    }


    /* Handle APN-AMBR updates */
    sess->gtp.create_session_response_apn_ambr = false;
    if ((gx_message->session_data.session.ambr.uplink &&
            (sess->session.ambr.uplink / 1000) !=
                (gx_message->session_data.session.ambr.uplink / 1000)) ||
        (gx_message->session_data.session.ambr.downlink &&
            (sess->session.ambr.downlink / 1000) !=
                (gx_message->session_data.session.ambr.downlink / 1000))) {

        sess->session.ambr.downlink =
            gx_message->session_data.session.ambr.downlink;
        sess->session.ambr.uplink =
            gx_message->session_data.session.ambr.uplink;

        sess->gtp.create_session_response_apn_ambr = true;
    }

    /* Handle Bearer QoS updates */
    sess->gtp.create_session_response_bearer_qos = false;
    if ((gx_message->session_data.session.qos.index &&
        sess->session.qos.index !=
            gx_message->session_data.session.qos.index) ||
        (gx_message->session_data.session.qos.arp.priority_level &&
        sess->session.qos.arp.priority_level !=
            gx_message->session_data.session.qos.arp.priority_level) ||
        sess->session.qos.arp.pre_emption_capability !=
            gx_message->session_data.session.qos.arp.pre_emption_capability ||
        sess->session.qos.arp.pre_emption_vulnerability !=
            gx_message->session_data.session.qos.arp.pre_emption_vulnerability) {

        sess->session.qos.index = gx_message->session_data.session.qos.index;
        sess->session.qos.arp.priority_level =
            gx_message->session_data.session.qos.arp.priority_level;
        sess->session.qos.arp.pre_emption_capability =
            gx_message->session_data.session.qos.arp.pre_emption_capability;
        sess->session.qos.arp.pre_emption_vulnerability =
            gx_message->session_data.session.qos.arp.pre_emption_vulnerability;

        sess->gtp.create_session_response_bearer_qos = true;
    }

    /* Setup QER if AMBR is present */
    if (sess->session.ambr.downlink || sess->session.ambr.uplink) {
        ogs_pfcp_qer_t *qer = ogs_pfcp_qer_add(&sess->pfcp);
        ogs_assert(qer);

        qer->mbr.downlink = sess->session.ambr.downlink;
        qer->mbr.uplink = sess->session.ambr.uplink;

        if (qer) {
            ogs_pfcp_pdr_associate_qer(dl_pdr, qer);
            ogs_pfcp_pdr_associate_qer(ul_pdr, qer);
        }
    }

    return ER_DIAMETER_SUCCESS;
}

uint32_t smf_gx_handle_cca_termination_request(
        smf_sess_t *sess, ogs_diam_gx_message_t *gx_message,
        ogs_gtp_xact_t *gtp_xact)
{
    ogs_assert(sess);
    ogs_assert(gx_message);

    ogs_debug("[SMF] Delete Session Response");
    ogs_debug("    SGW_S5C_TEID[0x%x] SMF_N4_TEID[0x%x]",
            sess->sgw_s5c_teid, sess->smf_n4_teid);

    return ER_DIAMETER_SUCCESS;
}

void smf_gx_handle_re_auth_request(
        smf_sess_t *sess, ogs_diam_gx_message_t *gx_message)
{
    int i;

    sess->policy.num_of_pcc_rule = gx_message->session_data.num_of_pcc_rule;
    for (i = 0; i < gx_message->session_data.num_of_pcc_rule; i++)
        OGS_STORE_PCC_RULE(&sess->policy.pcc_rule[i],
                &gx_message->session_data.pcc_rule[i]);

    smf_bearer_binding(sess);
}
