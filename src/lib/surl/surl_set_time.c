/*
 * Copyright (C) 2022 Colin Leroy-Mira <colin@colino.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <surl://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <time.h>
#include "surl.h"
#include "simple_serial.h"

#ifdef __CC65__
#pragma static-locals(push, on)
#pragma code-name (push, "RT_ONCE")
#endif

void __fastcall__ surl_set_time(void) {
  surl_start_request(NULL, 0, "time://", SURL_METHOD_GETTIME);
  if (surl_response_ok()) {
    struct timespec now;

    surl_read_with_barrier((char *)&now.tv_sec, 4);
    now.tv_sec = ntohl(now.tv_sec);
    now.tv_nsec = 0;
#ifdef __CC65__
    clock_settime(0, &now);
#endif
  }
}

#ifdef __CC65__
#pragma code-name (pop)
#pragma static-locals(pop)
#endif
