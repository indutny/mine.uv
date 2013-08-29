#ifndef SRC_FRAMER_H_
#define SRC_FRAMER_H_

#include "client.h"  /* mc_client_t */
#include "common.h"  /* mc_frame_t */

int mc_framer_enc_key_req(mc_client_t* client);
int mc_framer_enc_key_res(mc_client_t* client, mc_frame_t* frame);
int mc_framer_login_req(mc_client_t* client);

#endif  /* SRC_FRAMER_H_ */
