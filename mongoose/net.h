// Copyright (c) 2023 Cesanta Software Limited
// All rights reserved
#pragma once

#include "mongoose.h"

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(HTTP_URL)
#define HTTP_URL "http://0.0.0.0:80"
#endif

#if !defined(HTTPS_URL)
#define HTTPS_URL "https://0.0.0.0:443"
#endif

#if !defined(TCPSRV_URL)
#define TCPSRV_URL "tcp://0.0.0.0:4001"
#endif

#define MAX_DEVICE_NAME 20

void mongoose_init(void);
void mongoose_poll(void);

#ifdef __cplusplus
}
#endif
