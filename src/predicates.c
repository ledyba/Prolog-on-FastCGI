#include "predicates.h"
#include <stdio.h>

/**
 *　すべてで共通に使うユーティリティ
 */

void* get_ptr(term_t term){
	void* pointer = 0;
	int type;
	switch(type = PL_term_type(term)){
		case PL_POINTER:
		case PL_INTEGER:
			PL_get_pointer(term,&pointer);
			break;
		default:
			fprintf(stderr,"[Prolog FastCGI] Invalid req type:%d\n",type);
	}
	return pointer;
}

/**
 *　環境系
 */

foreign_t pl_getenv(term_t req_term,term_t name_term,term_t val_term){
	FCGI_PTR* req;
	char* name;
	char* val;
	if(!(req = (FCGI_PTR*)get_ptr(req_term)) || !FCGI_PTR_IS_VALID(req)){PL_fail;}
	if(PL_term_type(name_term) != PL_ATOM || PL_term_type(val_term) != PL_VARIABLE){
		PL_fail;
	}
	if(!PL_get_atom_chars(name_term, &name)){
		PL_fail;
	}
	val = FCGX_GetParam(name,req->req->envp);
	if(val){
		PL_unify_atom_chars(val_term,val);
	}else{
		PL_unify_nil(val_term);
	}
	PL_succeed;
}

foreign_t pl_fset_exit_status(term_t stream_term,term_t code_term){
	int c;
	FCGI_PTR* stream;
	if(!(stream = (FCGI_PTR*)get_ptr(stream_term)) || !(FCGI_PTR_IS_STREAM(stream))){PL_fail;}
	if(PL_term_type(code_term) != PL_INTEGER){PL_fail;}
	PL_get_integer(code_term,&c);
	FCGX_SetExitStatus(c,stream->ptr.stream.stream);
	PL_succeed;
}

/**
 *　ストリーム取得
 */
foreign_t pl_get_in_stream(term_t req_term,term_t val_term){
	FCGI_PTR* req;
	if(!(req = (FCGI_PTR*)get_ptr(req_term)) || !FCGI_PTR_IS_REQ(req)){PL_fail;}
	if(PL_term_type(val_term) == PL_VARIABLE && PL_unify_pointer(val_term, req->ptr.req.in)){
		PL_succeed;
	}
	PL_fail;
}
foreign_t pl_get_out_stream(term_t req_term,term_t val_term){
	FCGI_PTR* req;
	if(!(req = (FCGI_PTR*)get_ptr(req_term)) || !FCGI_PTR_IS_REQ(req)){PL_fail;}
	if(PL_term_type(val_term) == PL_VARIABLE && PL_unify_pointer(val_term, req->ptr.req.out)){
		PL_succeed;
	}
	PL_fail;
}
foreign_t pl_get_err_stream(term_t req_term,term_t val_term){
	FCGI_PTR* req;
	if(!(req = (FCGI_PTR*)get_ptr(req_term)) || !FCGI_PTR_IS_REQ(req)){PL_fail;}
	if(PL_term_type(val_term) == PL_VARIABLE && PL_unify_pointer(val_term, req->ptr.req.err)){
		PL_succeed;
	}
	PL_fail;
}
/**
 *　以下出力系API
 */
int _pl_write(FCGX_Stream* stream,term_t term){
	int type = PL_term_type(term);
	char *str;
	switch(type){
		case PL_VARIABLE:
			FCGX_PutS("(variable)", stream);
			break;
		case PL_ATOM:
		case PL_STRING:
		case PL_INTEGER:
		case PL_FLOAT:
			PL_get_chars(term, &str, CVT_WRITE | REP_UTF8);//UTF8にしないとなんかバケる。要注意。
			FCGX_PutS(str, stream);
			break;
		case PL_TERM:
			{
				atom_t name;
				int arity;
				term_t a = PL_new_term_ref();
				PL_get_name_arity(term, &name, &arity);
				if(PL_is_list(term)){
					int n;
					for(n=1; n<=arity; n++){
						PL_get_arg(n, term, a);
						if(n < arity || PL_is_compound(a)){
							_pl_write(stream,a);
						}
					}
				}else{
					FCGX_PutS(PL_atom_chars(name), stream);
					FCGX_PutS("(", stream);
					int n;
					for(n=1; n<=arity; n++){
						PL_get_arg(n, term, a);
						if(n > 1){FCGX_PutS(",", stream);}
						_pl_write(stream,a);
					}
					FCGX_PutS(")", stream);
				}
			}
	 		break;
	    default:
			return -1;
	}
	return 0;
}

/*
 *　改行なしで書き込む
 */

foreign_t pl_fprint(term_t stream_term,term_t term){
	FCGI_PTR* stream;
	if(!(stream = (FCGI_PTR*)get_ptr(stream_term)) || !(FCGI_PTR_IS_OUTPUT(stream))){PL_fail;}
	_pl_write(stream->ptr.stream.stream,term);
	PL_succeed;
}
/*
 *　改行ありで書き込む
 */

foreign_t pl_fputs(term_t stream_term,term_t term){
	FCGI_PTR* stream;
	if(!(stream = (FCGI_PTR*)get_ptr(stream_term)) || !(FCGI_PTR_IS_OUTPUT(stream))){PL_fail;}
	_pl_write(stream->ptr.stream.stream,term);
	FCGX_PutS("\n",stream->ptr.stream.stream);
	PL_succeed;
}

/*
 *　一文字書き込む
 */

foreign_t pl_fputc(term_t stream_term,term_t term){
	int c;
	FCGI_PTR* stream;
	if(!(stream = (FCGI_PTR*)get_ptr(stream_term)) || !(FCGI_PTR_IS_OUTPUT(stream))){PL_fail;}
	if(PL_term_type(term) != PL_INTEGER){PL_fail;}
	PL_get_integer(term,&c);
	FCGX_PutChar(c,stream->ptr.stream.stream);
	PL_succeed;
}

/*
 *　ストリームをフラッシュする。
 */

foreign_t pl_fflush(term_t stream_term){
	FCGI_PTR* stream;
	if(!(stream = (FCGI_PTR*)get_ptr(stream_term)) || !(FCGI_PTR_IS_OUTPUT(stream))){PL_fail;}
	FCGX_FFlush(stream->ptr.stream.stream);
	PL_succeed;
}

foreign_t pl_fclose(term_t stream_term){
	FCGI_PTR* stream;
	if(!(stream = (FCGI_PTR*)get_ptr(stream_term)) || !(FCGI_PTR_IS_STREAM(stream))){PL_fail;}
	if(FCGX_FClose(stream->ptr.stream.stream) < 0){
		fprintf(stderr,"[FastCGI Prolog] an error occurd in closing a stream.");
		PL_fail;
	}
	PL_succeed;
}

/**
 *
 */
/*
 *　改行なしで書き込む
 */

foreign_t pl_print(term_t req_term,term_t term){
	FCGI_PTR* req;
	if(!(req = (FCGI_PTR*)get_ptr(req_term)) || !FCGI_PTR_IS_VALID(req)){PL_fail;}
	_pl_write(req->req->out,term);
	PL_succeed;
}
/*
 *　改行ありで書き込む
 */

foreign_t pl_puts(term_t req_term,term_t term){
	FCGI_PTR* req;
	if(!(req = (FCGI_PTR*)get_ptr(req_term)) || !FCGI_PTR_IS_VALID(req)){PL_fail;}
	_pl_write(req->req->out,term);
	FCGX_PutS("\n",req->req->out);
	PL_succeed;
}

/*
 *　一文字書き込む
 */

foreign_t pl_putc(term_t req_term,term_t term){
	FCGI_PTR* req;
	int c;
	if(!(req = (FCGI_PTR*)get_ptr(req_term)) || !FCGI_PTR_IS_VALID(req)){PL_fail;}
	if(PL_term_type(term) != PL_INTEGER){PL_fail;}
	PL_get_integer(term,&c);
	FCGX_PutChar(c,req->req->out);
	PL_succeed;
}

/*
 *　ストリームをフラッシュする。
 */

foreign_t pl_flush(term_t req_term){
	FCGI_PTR* req;
	if(!(req = (FCGI_PTR*)get_ptr(req_term)) || !FCGI_PTR_IS_VALID(req)){PL_fail;}
	FCGX_FFlush(req->req->out);
	PL_succeed;
}

foreign_t pl_fget_error(term_t stream_term,term_t term){
	FCGI_PTR* stream;
	if(!(stream = (FCGI_PTR*)get_ptr(stream_term)) || !FCGI_PTR_IS_STREAM(stream)){PL_fail;}
	if(PL_term_type(term) != PL_VARIABLE){PL_fail;}
	PL_unify_integer(term,FCGX_GetError(stream->ptr.stream.stream));
	PL_succeed;
}

foreign_t pl_fclear_error(term_t stream_term){
	FCGI_PTR* stream;
	if(!(stream = (FCGI_PTR*)get_ptr(stream_term)) || !FCGI_PTR_IS_STREAM(stream)){PL_fail;}
	FCGX_ClearError(stream->ptr.stream.stream);
	PL_succeed;
}

/**
 *　起動第４段階：各種述語（written in C）を登録
 */
int regist_predicates(){
	if(
		/* スレッド起動用述語 */
		PL_register_foreign("fcgi_worker",2,(void*)pl_worker,0) &&
		/* 環境変数取得 */
		PL_register_foreign("fcgi_getenv",3,(void*)pl_getenv,0) &&
		PL_register_foreign("fcgi_fget_error",2,(void*)pl_fget_error,0) &&
		PL_register_foreign("fcgi_fclear_error",1,(void*)pl_fclear_error,0) &&
		PL_register_foreign("fcgi_fset_exit_status",1,(void*)pl_fset_exit_status,0) &&
		PL_register_foreign("fcgi_get_in_stream",2,(void*)pl_get_in_stream,0) &&
		PL_register_foreign("fcgi_get_err_stream",2,(void*)pl_get_err_stream,0) &&
		PL_register_foreign("fcgi_get_out_stream",2,(void*)pl_get_out_stream,0) &&
		/* 出力系述語 */
		PL_register_foreign("fcgi_putc",2,(void*)pl_putc,0) &&
		PL_register_foreign("fcgi_puts",2,(void*)pl_puts,0) &&
		PL_register_foreign("fcgi_print",2,(void*)pl_print,0) &&
		PL_register_foreign("fcgi_flush",1,(void*)pl_flush,0) &&

		PL_register_foreign("fcgi_fputc",2,(void*)pl_fputc,0) &&
		PL_register_foreign("fcgi_fputs",2,(void*)pl_fputs,0) &&
		PL_register_foreign("fcgi_fprint",2,(void*)pl_fprint,0) &&
		PL_register_foreign("fcgi_fflush",1,(void*)pl_fflush,0) &&
		PL_register_foreign("fcgi_fclose",1,(void*)pl_fclose,0) &&
		/* あったら便利な追加ライブラリ */
		regist_ext_libs() == 0
	){
		return 0;
	}else{
		return -1;
	}
}

