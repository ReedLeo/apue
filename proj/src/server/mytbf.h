#ifndef _MYTBF_H__
#define _MYTBF_H__

/**
 * Token Bucket Flow contorl.
*/
#define MYTBF_MAX 1024

typedef void mytbf_t;

mytbf_t* mytbf_init(const size_t cir, const size_t cbs);
ssize_t mytbf_fetch_token(mytbf_t*, const size_t);
ssize_t mytbf_return_token(mytbf_t*, const size_t);
ssize_t mytbf_destroy(mytbf_t*);

#endif