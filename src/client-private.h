#ifndef SRC_CLIENT_PRIVATE_H_
#define SRC_CLIENT_PRIVATE_H_

#include "client.h"  /* mc_client_t */
#include "common.h"  /* mc_frame_t */

int mc_client__handle_handshake(mc_client_t* client, mc_frame_t* frame);
int mc_client__client_limit(mc_client_t* client);
int mc_client__handle_frame(mc_client_t* client, mc_frame_t* frame);
int mc_client__send_kick(mc_client_t* client, const char* reason);

#endif  /* SRC_CLIENT_PRIVATE_H_ */
