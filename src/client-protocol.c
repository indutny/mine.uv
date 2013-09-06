#include "client.h"
#include "client-private.h"
#include "utils/common.h"  /* mc_frame_t */


int mc_client__handle_frame(mc_client_t* client, mc_frame_t* frame) {
  mc_client_destroy(client, "No protocol yet");
  return 0;
}
