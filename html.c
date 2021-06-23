#include <stdio.h>
#include <string.h>
#include "html.h"

static char * html_skip_spaces(char * string)
{
	char * res = string;
	while(*res == '\r' || *res == '\n' || *res == ' ' || *res == '\t')
		res ++;
	return res;
}

static char * html_find_next_tag(char * string, char ** tag, bool * istag)
{
	char * res = string;
	bool inquote = false;
	char quote = ' ';
	while(*res != 0 && *res != '<' && *res != '&')
	{
		if(*res == ' ' || *res == '\t' || *res == '\r' || *res == '\n')
		{
			if(res > string && *(res - 1) == ' ')
				*res = 0x1F;
			else
				*res = ' ';
		}
		res ++;
	}
	if(*res == 0)
		return NULL;
	if(*res == '<')
	{
		*res++ = 0;
		res = html_skip_spaces(res);
		*tag = res;
		while(inquote || (*res != 0 && *res != '>'))
		{
			if(*res == '"' || *res == '\'')
			{
				if(!inquote)
				{
					inquote = true;
					quote = *res;
				}
				else if(quote == *res)
					inquote = false;
			}
			if(!inquote && (*res == ' ' || *res == '\t' || *res == '\r' || *res == '\n'))
				*res = 0;
			res ++;
		}
		if(*res == 0)
			return NULL;
		*res++ = 0;
		res = html_skip_spaces(res);
		*istag = true;
	}
	else
	{
		*res++ = 0;
		*tag = res;
		while(*res != 0 && *res != ';')
			res ++;
		if(*res == 0)
			return NULL;
		*res++ = 0;
		*istag = false;
	}
	return res;
}

static char * html_skip_close_tag(char * string, char * tag)
{
	char * res = string, *sstart;
	int len = strlen(tag) + 1;
	char ttag[len + 1];
	sprintf(ttag, "/%s", tag);
	do {
		while(*res != 0 && *res != '<') res ++;
		if(*res == 0)
			return NULL;
		sstart = res;
		res ++;
		res = html_skip_spaces(res);
		if(strncmp(ttag, res, len) == 0)
			res += len;
		else
			continue;
		res = html_skip_spaces(res);
		if(*res == 0)
			return NULL;
	} while(*res != '>');
	*sstart = *res++ = 0;
	return res;
}

extern dword html_to_text(char * string)
{
	char * nstr, * ntag, * cstr = string, * str = string;
	bool istag;
	while(str != NULL && (nstr = html_find_next_tag(str, &ntag, &istag)) != NULL)
	{
		if(str != cstr)
		{
			int len = (int)ntag - (int)str - 1;
			if(len > 0)
			{
				strncpy(cstr, str, len);
				cstr += len;
			}
		}
		if(istag)
		{
			if(strcasecmp(ntag, "head") == 0 || strcasecmp(ntag, "style") == 0 || strcasecmp(ntag, "title") == 0)
			{
				char * pstr = html_skip_close_tag(nstr, ntag);
				if(pstr != NULL)
					str = pstr;
				else
					str = nstr;
			}
			else if(strcasecmp(ntag, "script") == 0)
			{
				char * kstr = nstr;
				str = html_skip_close_tag(nstr, ntag);
				while((kstr = strstr(kstr, "document.write(")) != NULL)
				{
					kstr += 15;
					while(*kstr == ' ' || *kstr == '\t')
						kstr ++;
					if(*kstr == '\'' || *kstr == '"')
					{
						char qt = *kstr;
						char * dstr;
						kstr ++;
						dstr = kstr;
						while(*dstr != 0 && (*dstr != qt || *(dstr - 1) == '\\'))
							dstr ++;
						if(*dstr == qt)
						{
							strncpy(cstr, kstr, dstr - kstr);
							cstr[dstr - kstr] = 0;
							cstr += html_to_text(cstr);
						}
						kstr = dstr;
					}
				}
			}
			else
			{
				if(strcasecmp(ntag, "br") == 0 || strcasecmp(ntag, "li") == 0 || strcasecmp(ntag, "p") == 0 || strcasecmp(ntag, "/p") == 0 || strcasecmp(ntag, "tr") == 0 || strcasecmp(ntag, "/table") == 0 || strcasecmp(ntag, "div") == 0 || strcasecmp(ntag, "/div") == 0)
					*cstr ++ = '\n';
				else if(strcasecmp(ntag, "th") == 0 || strcasecmp(ntag, "td") == 0)
					*cstr ++ = ' ';
				str = nstr;
			}
		}
		else
		{
			if(strcasecmp(ntag, "nbsp") == 0)
				*cstr ++ = ' ';
			else if(strcasecmp(ntag, "gt") == 0)
				*cstr ++ = '>';
			else if(strcasecmp(ntag, "lt") == 0)
				*cstr ++ = '<';
			else if(strcasecmp(ntag, "amp") == 0)
				*cstr ++ = '&';
			else if(strcasecmp(ntag, "quote") == 0)
				*cstr ++ = '\'';
			else if(strcasecmp(ntag, "copy") == 0)
			{
				*cstr ++ = '(';
				*cstr ++ = 'C';
				*cstr ++ = ')';
			}
			str = nstr;
		}
	}
	strcpy(cstr, str);
	cstr += strlen(str);
	return cstr - string;
}
