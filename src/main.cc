#include <iostream>
#include <vector>
#include <getopt.h>
#include <string.h>
#include <stdlib.h>
#include <SWI-Prolog.h>
#include <boost/shared_ptr.hpp>
#include <boost/foreach.hpp>
using namespace std;
using namespace boost;

#include "predicates.h"
#include "worker-pred.h"

#define DEFAULT_BACKLOGS (16)
#define MIN_BACKLOGS (0)
#define MAX_BACKLOGS (256)

/**
 *　その他：シグナル処理
 */

/**
 *　起動第５段階：independent/workerスレッドを起動
 */

int start_threads(const int fd,const int backlogs,const int debug_level){
	vector<int> thread_id_vec;
	typedef shared_ptr<string> StrPtr;
	vector<StrPtr> thread_atom_vec;
	/* 述語の作成 */
	predicate_t thread_create = PL_predicate("thread_create", 3, NULL);
	predicate_t thread_join = PL_predicate("thread_join", 2, NULL);

	/* independentスレッド起動 */

	//タームの確保・生成
	term_t independent_thread_term = PL_new_term_refs(3);

	//実行される述語のタームを生成←正直用語が無茶苦茶
	term_t independent_functor_term = PL_new_term_ref();
	PL_cons_functor(independent_functor_term,PL_new_functor(PL_new_atom("fcgid_independent"), 0));

	//オプションタームの構築
	term_t independent_thread_alias_opt_term = PL_new_term_ref();
	term_t independent_thread_alias_val_term = PL_new_term_ref();
	PL_put_atom_chars(independent_thread_alias_val_term,"independent");
	PL_cons_functor(independent_thread_alias_opt_term, PL_new_functor(PL_new_atom("alias"), 1), independent_thread_alias_val_term);

	//create_thread用タームの構築
	PL_put_term(independent_thread_term+0,independent_functor_term);
	PL_put_variable(independent_thread_term+1);//変数
	//オプション
	PL_put_nil(independent_thread_term+2);
	PL_cons_list(independent_thread_term+2,independent_thread_alias_opt_term,independent_thread_term+2);

	//スレッド生成
	qid_t independent_qid = PL_open_query(NULL, PL_Q_NORMAL, thread_create, independent_thread_term);
	while(PL_next_solution(independent_qid) == TRUE){
		int num;
		char* atom;
		switch(PL_term_type(independent_thread_term+1)){
			case PL_INTEGER:
				if(!PL_get_integer(independent_thread_term+1, &num)){
					fprintf(stderr,"[FastCGI-Prolog][independent] failed to get Thread ID.\n");
				}
				thread_id_vec.push_back(num);
				break;
			case PL_ATOM:
				if ( !PL_get_atom_chars(independent_thread_term+1, &atom)){
				}
				thread_atom_vec.push_back(StrPtr( new string(atom) ));
				break;
			default:
				fprintf(stderr,"[FastCGI-Prolog][independent] Invalid Thread ID.\n");
				return -1;
		}
	}
	PL_close_query(independent_qid);

	/* wokerスレッド起動 */
	for(int i=0;i<backlogs;i++){
		term_t worker_thread_term = PL_new_term_refs(3);

		term_t worker_functor_term = PL_new_term_ref();
		term_t worker_functor_fd_term = PL_new_term_ref();
		term_t worker_functor_debug_term = PL_new_term_ref();

		PL_put_integer(worker_functor_fd_term,fd);
		PL_put_integer(worker_functor_debug_term,debug_level);
		PL_cons_functor(worker_functor_term,PL_new_functor(PL_new_atom("fcgi_worker"), 2),worker_functor_fd_term,worker_functor_debug_term);

		PL_put_term(worker_thread_term+0,worker_functor_term);
		PL_put_variable(worker_thread_term+1);//変数
		PL_put_nil(worker_thread_term+2);//オプション

		//クエリ実行
		qid_t worker_qid = PL_open_query(NULL, PL_Q_NORMAL, thread_create, worker_thread_term);
		while(PL_next_solution(worker_qid) == TRUE){
			int num;
			char* atom;
			switch(num = PL_term_type(worker_thread_term+1)){
				case PL_INTEGER:
					if(!PL_get_integer(worker_thread_term+1, &num)){
						fprintf(stderr,"[FastCGI-Prolog][worker] failed to get Thread ID.\n");
					}
					thread_id_vec.push_back(num);
					break;
				case PL_ATOM:
					if ( !PL_get_atom_chars(worker_thread_term+1, &atom)){
					}
					thread_atom_vec.push_back(StrPtr( new string(atom) ));
					break;
				default:
					fprintf(stderr,"[FastCGI-Prolog][worker] Invalid Thread ID.type %d\n",num);
					return -1;
			}
		}
		PL_close_query(worker_qid);
	}

	/* 終了まで待機 */
	BOOST_FOREACH(int n, thread_id_vec) {
		term_t join_term = PL_new_term_refs(2);
		PL_put_integer(join_term+0,n);//スレッドID
		PL_put_variable(join_term+1);//変数

		qid_t join_qid = PL_open_query(NULL, PL_Q_NORMAL, thread_join, join_term);
		while(PL_next_solution(join_qid) == TRUE){}
		PL_close_query(join_qid);
	}
	BOOST_FOREACH(StrPtr atom, thread_atom_vec) {
		term_t join_term = PL_new_term_refs(2);
		PL_put_atom_chars(join_term+0,(*atom).c_str());//スレッドID
		PL_put_variable(join_term+1);//変数

		qid_t join_qid = PL_open_query(NULL, PL_Q_NORMAL, thread_join, join_term);
		while(PL_next_solution(join_qid) == TRUE){}
		PL_close_query(join_qid);
	}

	return 0;
}

/**
 *　起動第３段階：prologエンジンの起動／終了処理
 */
int launch_prolog(const int fd,const int backlogs,const int debug_level,int pl_argc,char** pl_argv){
	char swi_home[4096];
	if(strlen(SWI_HOME_DIR)+14 >= 4096){
		fprintf(stderr,"[FastCGI-Prolog] SWI_HOME_DIR is too long!\n");
		return -1;
	}
	sprintf(swi_home,"SWI_HOME_DIR=%s",SWI_HOME_DIR);
	putenv(swi_home);
	if(!PL_initialise(pl_argc,(char**)pl_argv)){//エンジンを起動
		PL_halt(1);
	}
	/**
	 *　起動第４段階：各種述語（written in C）を登録
	 */
	if(regist_predicates()){
		fprintf(stderr,"[FastCGI-Prolog] failed to regist predicates.\n");
		PL_halt(-1);
		return -1;
	}
	int ret_code = start_threads(fd,backlogs,debug_level);
	PL_halt(ret_code);
	return ret_code;
}

#include <fcgiapp.h>
/**
 *  起動第２段階：FastCGIの初期化／終了　処理
 */
int launch_fcgi(const char* socket,const int backlogs,const int debug_level,const int pl_argc,char** pl_argv){
	/* FastCGI初期化 */
	if(FCGX_Init()){
		fprintf(stderr,"[FastCGI-Prolog] failed to init libfcgi library.\n");
		return -1;
	}
	int fd;
	if((fd = FCGX_OpenSocket(socket, backlogs)) == -1){
		fprintf(stderr,"[FastCGI-Prolog] failed to open socket: %s\n",socket);
		return -1;
	}
	/* Prologエンジンの準備を行う */
	if(!launch_prolog(fd,backlogs,debug_level,pl_argc,pl_argv)){
		return -1;
	}
	/* 処理が終了したので、止める。 */
	if(close(fd) < 0){
		fprintf(stderr,"[FastCGI-Prolog] failed to close socket file descripter: %s(%d)\n",socket,fd);
		return -1;
	}
	return 0;
}

/**
 *　うさげ
 */
void usage(){
	fprintf(stderr,"SWI-Prolog FastCGI Daemon -- a bridge between SWI-Prolog and HTTP Server.\n");
	fprintf(stderr,"usage:swpfcgid\n");
	fprintf(stderr,"\t--fastcgi-socket <socket>\n");
	fprintf(stderr,"\t[--fastcgi-backlogs <backlogs=%d>]\n",DEFAULT_BACKLOGS);
	fprintf(stderr,"\t[--fastcgi-debug <debug_level=%d>]\n",0);
	fprintf(stderr,"\t[Prolog options...]\n\n");
}

/**
 *　起動第１段階：オプション解析
 */
int main(int argc, char **argv){
	vector<char*> prolog_opts;
	prolog_opts.push_back(argv[0]);
	/* getoptで得られるオプション */
	char* socket = 0;
	int backlogs = DEFAULT_BACKLOGS;
	int debug = 0;//false
	/* オプションの指定 */
	static struct option long_options[] = {
		{"fastcgi-socket",		required_argument, 0, 1},
		{"fastcgi-backlogs",	required_argument, 0, 2},
		{"fastcgi-debug",		required_argument, 0, 3},
		{0, 0, 0, 0}
	};
	int opt;
	int option_index = 0;
	/* オプションのパーシング */
	opterr = 0;
	while ((opt = getopt_long_only(argc, argv, "", long_options, &option_index)) != -1){
		switch (opt){
		case 1:
			socket = optarg;
			break;
		case 2:
			backlogs = atoi(optarg);
			if(backlogs <= MIN_BACKLOGS){
				usage();
				fprintf(stderr,"[FastCGI-Prolog] backlogs = %d ( <= %d)\n",backlogs,MIN_BACKLOGS);
				return -1;
			}else if(backlogs > MAX_BACKLOGS){
				usage();
				fprintf(stderr,"[FastCGI-Prolog] backlogs = %d ( > %d)\n",backlogs,MAX_BACKLOGS);
				return -1;
			}
			break;
		case 3:
			debug = atoi(optarg);
			if(backlogs < 0){
				usage();
				fprintf(stderr,"[FastCGI-Prolog] debug_level = %d ( < %d)\n",backlogs,0);
				return -1;
			}
			break;
		default: //何も処理されない＝Prologの引数として処理
			prolog_opts.push_back(argv[optind-1]);
			if(argv[optind][0] != '-'){//次が引数っぽくなければ、追加してしまう。
				prolog_opts.push_back(argv[optind]);
				optind++;
			}
		}
	}
	/* オプションでないものをコピー */
	for(int i=optind;i<argc;i++){
		prolog_opts.push_back(argv[i]);
	}
	/* バリデーション */
	if(socket == 0 || strcmp("",socket) == 0){
		usage();
		fprintf(stderr,"[FastCGI-Prolog] socket must be set.\n");
		return -1;
	}
	/* オプション解釈終了 */
	return launch_fcgi(socket,backlogs,debug,prolog_opts.size(),&prolog_opts[0]); //第二段階へ以降
}

