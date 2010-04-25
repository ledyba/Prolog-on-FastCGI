#include "fcgi_util.h"
#include <iconv.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

/*
 *　方針：めんどくさいので、全部UTF-32に変換した上でかたっぱしから変換。うへへ。
 */
foreign_t pl_escape_html(term_t normal_term,term_t escaped_term){
	if(PL_is_variable(normal_term) && PL_is_atomic(escaped_term)){
		// escaped -> normal
#ifdef LITTLE_ENDIAN
		iconv_t _iconv = iconv_open("UTF-8", "UTF-32LE");
#else
		iconv_t _iconv = iconv_open("UTF-8", "UTF-32BE");
#endif
		assert(_iconv > (iconv_t)0);


		int iconv_close_ret = iconv_close(_iconv);
		assert(iconv_close_ret == 0);
	}else if(PL_is_atomic(normal_term) && PL_is_variable(escaped_term)){
		// normal -> escaped
		char* normal;
		char* normal_start;
		PL_get_chars(normal_term,&normal,CVT_ALL | REP_UTF8);
		normal_start = normal;
		size_t normal_len = strlen(normal);
		unsigned int* utf32 = malloc(normal_len * 4);
		unsigned int* utf32_start = utf32;
		size_t _iconv_normal_len = normal_len;
		size_t _iconv_utf32_len = normal_len * 4;
		//iconvで変換
		assert(utf32);
#ifdef LITTLE_ENDIAN
		iconv_t _iconv = iconv_open("UTF-32LE", "UTF-8");
#else
		iconv_t _iconv = iconv_open("UTF-32BE", "UTF-8");
#endif
		assert(_iconv > (iconv_t)0);
		int len = iconv(_iconv,&normal_start, &_iconv_normal_len, (char**)&utf32_start, &_iconv_utf32_len);
		assert(len >= 0);
		int iconv_close_ret = iconv_close(_iconv);
		assert(iconv_close_ret == 0);
		//UTF32からescapedに変換
		const size_t escaped_len = (normal_len * 12) + 1;
		char* escaped = malloc(escaped_len); //&#x00000000;が最大で、１２文字。
		char escaped_letter[20];
		assert(escaped);
		escaped[0] = '\0';
		int i;
		for(i=0;i<(utf32_start - utf32);i++){
			int size = snprintf(escaped_letter,20,"&#x%x;",utf32[i]);
			assert(size <= 20);
			strcat(escaped,escaped_letter);
		}

		if(!PL_unify_atom_chars(escaped_term, escaped)){
			fprintf(stderr,"[Prolog FastCGI][escape_html/2][normal->escaped] failed to unify string.\n");
			PL_fail;
		}

		free(escaped);
		free(utf32);
	}else{
		fprintf(stderr,"[Prolog FastCGI][escape_html/2] 1st or 2nd term must be variable, and the other must be atomic.\n");
		PL_fail;
	}
	PL_succeed;
}

int regist_fcgi_util_libs(){
	if(
		PL_register_foreign("escape_html",2,(void*)pl_escape_html,0) &&
		1
	){
		return 0;
	}
	return -1;
}

