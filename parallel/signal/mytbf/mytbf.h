#ifndef MYTBF_H__
#define MYTBF_H__

#define MYTBF_MAX 1024

typedef void mytbf_t;

mytbf_t* mytbf_init(int cps, int burst);
int mytbf_fetch_token(mytbf_t* p_tbf, int request);
int mytbf_return_token(mytbf_t* p_tbf, int token_num);
int mytbf_destory(mytbf_t* p_tbf);

#endif