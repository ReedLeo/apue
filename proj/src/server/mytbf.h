#ifndef _MYTBF_H__
#define _MYTBF_H__

/**
 * Token Bucket Flow contorl.
*/
#define MYTBF_MAX 1024

typedef void mytbf_t;

mytbf_t* mytbf_init(int cps, int burst);
int mytbf_fetch_token(mytbf_t*, int);
int mytbf_return_token(mytbf_t*, int);
int mytbf_destroy(mytbf_t*);

#endif