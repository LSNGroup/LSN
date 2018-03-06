#pragma once

#include "Shiyong.h"
#include "Guaji.h"


void fake_rtp_recv_fn(void *ctx, int payload_type, unsigned long rtptimestamp, unsigned char *data, int len);
