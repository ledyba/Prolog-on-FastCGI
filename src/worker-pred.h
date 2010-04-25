#ifndef _WORKER_PRED_H
#define _WORKER_PRED_H
#ifdef __cplusplus
extern "C"{
#endif

#include <SWI-Prolog.h>
#include <fcgiapp.h>
#include "predicates.h"

/*
 *　REQの時はすべての綱目が埋まっている。
 *　それ以外の時は、自分とそのリクエストが埋まっている。
 */
#define FCGI_PTR_VALID	(0x18c25afa)
#define FCGI_PTR_REQ	(0x45e7c762)
#define FCGI_PTR_IN		(0xe2551edc)
#define FCGI_PTR_OUT	(0xf091f111)
#define FCGI_PTR_ERR	(0x6c13ba51)

typedef struct FCGI_PTR FCGI_PTR;
#define FCGI_PTR_IS_VALID(x)	((x)->valid == FCGI_PTR_VALID)

#define FCGI_PTR_IS_REQ(x)	(FCGI_PTR_IS_VALID(x) && (x)->type == FCGI_PTR_REQ)
#define FCGI_PTR_IS_OUT(x)	(FCGI_PTR_IS_VALID(x) && (x)->type == FCGI_PTR_OUT)
#define FCGI_PTR_IS_ERR(x)	(FCGI_PTR_IS_VALID(x) && (x)->type == FCGI_PTR_ERR)
#define FCGI_PTR_IS_IN(x)	(FCGI_PTR_IS_VALID(x) && (x)->type == FCGI_PTR_IN)

#define FCGI_PTR_IS_OUTPUT(x)	(FCGI_PTR_IS_VALID(x) && (FCGI_PTR_IS_OUT(x) || FCGI_PTR_IS_ERR(x)))
#define FCGI_PTR_IS_INPUT(x)	(FCGI_PTR_IS_VALID(x) && FCGI_PTR_IS_IN(x))
#define FCGI_PTR_IS_STREAM(x)	(FCGI_PTR_IS_VALID(x) && (FCGI_PTR_IS_OUTPUT(x) || FCGI_PTR_IS_INPUT(x)))

struct FCGI_PTR{
	int valid;
	int type;
	FCGX_Request* req;
	union {
		struct {//REQの時は埋まっている
			FCGI_PTR* in;
			FCGI_PTR* out;
			FCGI_PTR* err;
		} req;
		struct {
			FCGX_Stream* stream;//IN/OUT/ERRの各ストリーム
			FCGI_PTR* req;
		} stream;
	}ptr;
};

foreign_t pl_worker(term_t term_fd,term_t term_debug_level);


#ifdef __cplusplus
}
#endif
#endif /* _WORKER_PRED_H */

