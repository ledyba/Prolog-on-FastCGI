#include "worker-pred.h"
#include <stdio.h>
foreign_t pl_worker(term_t term_fd,term_t term_debug_level){
	int fd;
	int debug_level;
	predicate_t fcgid_handler = PL_predicate("fcgid_handler", 1, NULL);
	predicate_t make_pred = PL_predicate("make", 0, NULL);
	switch(PL_term_type(term_fd)){
		case PL_INTEGER:
			if(!PL_get_integer(term_fd, &fd)){
				PL_fail;
			}
			break;
		default://数字以外は許さない
			PL_fail;
	}
	switch(PL_term_type(term_debug_level)){
		case PL_INTEGER:
			if(!PL_get_integer(term_debug_level, &debug_level)){
				PL_fail;
			}
			break;
		default://数字以外は許さない
			PL_fail;
	}
	FCGX_Request req;
	if(FCGX_InitRequest(&req,fd,0)){
		fprintf(stderr,"[FastCGI-Prolog] failed to FCGX_InitRequest.\n");
		PL_fail;
	}
	while(1){
		if(FCGX_Accept_r(&req) < 0){
			fprintf(stderr,"[FastCGI-Prolog] failed to FCGX_Accept_r.\n");
			continue;
		}
		//ポインタの設定
		FCGI_PTR req_ptr;
		FCGI_PTR in_ptr;
		FCGI_PTR out_ptr;
		FCGI_PTR err_ptr;

		req_ptr.valid = FCGI_PTR_VALID;
		req_ptr.type = FCGI_PTR_REQ;
		req_ptr.req = &req;
		req_ptr.ptr.req.in = &in_ptr;
		req_ptr.ptr.req.out = &out_ptr;
		req_ptr.ptr.req.err = &err_ptr;

		in_ptr.valid = FCGI_PTR_VALID;
		in_ptr.type = FCGI_PTR_IN;
		in_ptr.req = &req;
		in_ptr.ptr.stream.stream = req.in;
		in_ptr.ptr.stream.req = &req_ptr;

		out_ptr.valid = FCGI_PTR_VALID;
		out_ptr.type = FCGI_PTR_OUT;
		out_ptr.req = &req;
		out_ptr.ptr.stream.stream = req.out;
		out_ptr.ptr.stream.req = &req_ptr;

		err_ptr.valid = FCGI_PTR_VALID;
		err_ptr.type = FCGI_PTR_ERR;
		err_ptr.req = &req;
		err_ptr.ptr.stream.stream = req.err;
		err_ptr.ptr.stream.req = &req_ptr;

		// debugモードは毎回スクリプトの変更をチェック
		if(debug_level > 0){
			qid_t make_qid = PL_open_query(NULL, PL_Q_NORMAL, make_pred, 0);
			while(PL_next_solution(make_qid) == TRUE){}
			PL_close_query(make_qid);
		}
		// Prolog側の述語の呼び出し
		term_t handler_term = PL_new_term_ref();
		PL_put_pointer(handler_term,&req_ptr);
		qid_t handler_qid = PL_open_query(NULL, PL_Q_NORMAL, fcgid_handler, handler_term);
		while(PL_next_solution(handler_qid) == TRUE){}
		PL_close_query(handler_qid);
		FCGX_FFlush(req.out);

		//最後におわり
		FCGX_Finish_r(&req);
	}
	FCGX_Free(&req,1);
	PL_succeed;
}

