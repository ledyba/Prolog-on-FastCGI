#include "pl_iconv.h"
#include <iconv.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

foreign_t pl_iconv_name(term_t atom_term,term_t string_code_term,term_t string_term){
	if(!PL_is_atomic(string_code_term)){
		fprintf(stderr,"[Prolog FastCGI][iconv_name/3] 2nd term term must be atomic.\n");
		PL_fail;
	}
	char* conv_from;
	char* conv_to;
	char* start_conv_to;
	char* conv_code_from;
	char* conv_code_to;
	size_t from_len;
	size_t to_len;
	size_t len;
	if(PL_is_variable(string_term) && PL_is_atom(atom_term)){
		// atom -> stringに変換
		conv_code_from = "UTF-8";
		PL_get_chars(string_code_term,&conv_code_to,CVT_ALL | REP_UTF8);

		PL_get_chars(atom_term,&conv_from,CVT_ALL | REP_UTF8);

		from_len = strlen(conv_from);
		to_len = from_len * 8;
		start_conv_to = conv_to = malloc(to_len);
		assert(conv_to);

		iconv_t _iconv = iconv_open(conv_code_to, conv_code_from);
		assert(_iconv > (iconv_t)0);
		len = iconv(_iconv,&conv_from, &from_len, &conv_to, &to_len);
		*conv_to = '\0';
		assert(len >= 0);
		int iconv_close_ret = iconv_close(_iconv);
		assert(iconv_close_ret == 0);

		if(!PL_unify_list_codes(string_term, start_conv_to)){
			fprintf(stderr,"[Prolog FastCGI][iconv_name/3][atom->string] failed to unify string.\n");
			PL_fail;
		}

		free(start_conv_to);
	}else if((PL_is_string(string_term) || PL_is_list(string_term)) && PL_is_variable(atom_term)){
		// string -> atom変換
		PL_get_chars(string_code_term,&conv_code_from,CVT_ALL | REP_UTF8);
		conv_code_to = "UTF-8";

		PL_get_chars(string_term,&conv_from,CVT_ALL);

		from_len = strlen(conv_from);
		to_len = from_len * 8;
		start_conv_to = conv_to = malloc(to_len);
		assert(conv_to);
		iconv_t _iconv = iconv_open(conv_code_to, conv_code_from);
		assert(_iconv > (iconv_t)0);
		len = iconv(_iconv,&conv_from, &from_len, &conv_to, &to_len);
		*conv_to = '\0';
		assert(len >= 0);
		int iconv_close_ret = iconv_close(_iconv);
		assert(iconv_close_ret == 0);

		if(!PL_unify_atom_chars(atom_term, start_conv_to)){
			fprintf(stderr,"[Prolog FastCGI][iconv_name/3][string->atom] failed to unify atom.\n");
			PL_fail;
		}
		free(start_conv_to);
	}else{
		fprintf(stderr,"[Prolog FastCGI][iconv_name/3] 1st term or 3rd term must be variable.\n");
		PL_fail;
	}
	PL_succeed;
}
int regist_iconv_libs(){
	if(
		PL_register_foreign("iconv_name",3,(void*)pl_iconv_name,0) &&
		1
	){
		return 0;
	}
	return -1;
}

