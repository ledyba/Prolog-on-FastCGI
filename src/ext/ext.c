#include "ext.h"
#include "pl_iconv.h"
#include "fcgi_util.h"
int regist_ext_libs(){
	if(
		regist_iconv_libs() == 0 &&
		regist_fcgi_util_libs() == 0 &&
		1
	){
		return 0;
	}
	return -1;
}

