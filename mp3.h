#ifndef _MP3_H_
#define _MP3_H_

#include "common/datatype.h"

extern bool mp3_init();
extern void mp3_end();
extern void mp3_start();
extern void mp3_pause();
extern void mp3_resume();
extern void mp3_stop();
extern void mp3_powerdown();
extern void mp3_powerup();
extern void mp3_list(const char *path);
extern void mp3_prev();
extern void mp3_next();
extern void mp3_restart();
extern bool mp3_paused();
extern char * mp3_get_tag();
extern bool mp3_get_info(int * bitrate, int * sample, int * curlength, int * totallength);

#endif
