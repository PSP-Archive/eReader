#ifndef _CONF_H_
#define _CONF_H_

#include "common/datatype.h"

typedef enum {
	conf_encode_gbk = 0,
	conf_encode_big5 = 1,
	conf_encode_sjis = 2,
} t_conf_encode;

typedef enum {
	conf_fit_none = 0,
	conf_fit_width = 1,
	conf_fit_height = 2,
	conf_fit_custom = 3
} t_conf_fit;

typedef enum {
	conf_rotate_0 = 0,
	conf_rotate_90 = 1,
	conf_rotate_180 = 2,
	conf_rotate_270 = 3,
} t_conf_rotate;

typedef struct {
	char path[256];
	dword forecolor;
	dword bgcolor;
	bool rowspace;
	bool infobar;
	bool rlastrow;
	bool autobm;
	bool vertread;
	t_conf_encode encode;
	t_conf_fit fit;
	bool imginfobar;
	bool scrollbar;
	dword scale;
	t_conf_rotate rotate;
	dword txtkey[20];
	dword imgkey[20];
	char shortpath[256];
	dword confver;
	bool bicubic;
} t_conf, * p_conf;

/* txt key:
	0 - bookmark;
	1 - prevpage;
	2 - nextpage;
	3 - prev100;
	4 - next100;
	5 - prev500;
	6 - next500;
	7 - firstpage;
	8 - lastpage;
	9 - prevbook;
	10 - nextbook;
	11 - exitbook;
	above 11 - reversed for future version

   image key:
	0 - imageprev;
	1 - imagenext;
	2 - imagescaletype;
	3 - imagescalesmaller;
	4 - imagescalelarger;
	5 - imagerotateleft;
	6 - imagerotateright;
	7 - imagebar;
	8 - imageinfo;
	9 - imageexit;
	above 9 - reversed for future version
*/

extern void conf_set_file(const char * filename);
extern void conf_get_keyname(dword key, char * res);
extern bool conf_load(p_conf conf);
extern bool conf_save(p_conf conf);
extern const char * conf_encodename(t_conf_encode encode);

#endif
