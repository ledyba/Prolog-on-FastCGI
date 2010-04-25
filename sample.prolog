/* アイテム一覧 */
get_item(MOD,X) :- !,nth0(MOD,
					['下心', '微妙さ', '優雅さ', '華麗さ', 'かわいさ', 'やさしさ', 'やましさ',
                  'やらしさ', 'むなしさ', 'ツンデレ', '厳しさ', '世の無常さ', 'ハッタリ', 'ビタミン',
                  '努力', '気合', '根性', '砂糖', '食塩', '愛', '電波', '毒電波', '元気玉',
                  '怨念', '大阪のおいしい水', '明太子', '勇気', '運', '電力', '小麦粉',
                  '汗と涙(化合物)', '覚悟', '大人の都合', '見栄', '欲望', '嘘', '真空', '呪詛',
                  '信念', '夢', '記憶', '鉄の意志', 'カルシウム', '魔法', '希望', '不思議',
                  '勢い', '度胸', '乙女心', '罠', '花崗岩', '宇宙の意思', '犠牲', '毒物', '鉛',
                  '海水', '蛇の抜け殻', '波動', '純金', '情報', '知識', '知恵', '魂の炎', '媚び',
                  '保存料', '着色料', '税金', '歌', '苦労', '柳の樹皮', '睡眠薬', 'スライム',
                  'アルコール', '時間', '果物', '玉露', '利益', '赤い何か', '白い何か', '鍛錬',
                  '月の光', '回路', '野望', '陰謀', '雪の結晶', '株', '黒インク', '白インク',
                  'カテキン', '祝福', '気の迷い', 'マイナスイオン', '濃硫酸', 'ミスリル', 'お菓子',
                  '言葉', '心の壁', '成功の鍵', '理論', '血'],X).

/* コードを求めるためのループ表現 */
code_loop([],_,0).
code_loop(Str,Count,Sum) :-
	[First | Left] = Str,
	NextCount is Count + 1,
	code_loop(Left,NextCount,BeforeSum),
	Sum is BeforeSum + (First << ((Count /\ 3) << 3)).

get_strcode(ITEM,X) :- !,iconv_name(ITEM,'Shift_JIS',Str),code_loop(Str,0,X).

buf_random(Seed,NextSeed,Rnd) :- 
	NextSeed is (Seed * 214013) + 2531011,
	Rnd is (NextSeed >> 16) /\ 0x7fff.

list_item(_,0,[]).
list_item(Code,Left,List) :- 
	Left > 0,
	buf_random(Code,Seed,ItemRnd),
	ItemNo is ItemRnd mod 100,
	get_item(ItemNo,Item),
	buf_random(Seed,NextSeed,PerRnd),
	Percentage is (PerRnd mod Left)+1,
	NextLeft is Left - Percentage,
	list_item(NextSeed,NextLeft,NextList),
	append(NextList,[[Percentage,Item]],List).

buffalin(ITEM,List) :- get_strcode(ITEM,Code),list_item(Code,100,FList),sort(FList,RList),reverse(RList,List).
/* HTML出力 */

write_header(REQ,Thing,EscapedThing) :- 
	fcgi_puts(REQ,'Content-type: text/html; charset=UTF-8\r\n\r\n'),
	fcgi_puts(REQ,'<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01//EN" "http://www.w3.org/TR/html4/strict.dtd">'),
	fcgi_puts(REQ,'<html lang="ja"><head>'),
	fcgi_puts(REQ,'<link rev="made" href="http://ledyba.ddo.jp/">'),
	fcgi_puts(REQ,'<meta http-equiv="Content-Script-Type" content="text/javascript">'),
	fcgi_puts(REQ,'<meta http-equiv="Content-Style-Type" content="text/css">'),
	fcgi_puts(REQ,'<meta http-equiv="Content-Type" content="text/html; charset=UTF-8">'),
	fcgi_puts(REQ,'<title>「成分解析」 on Prolog FastCGI'),
	atom_length(Thing,ThingLen),
	ThingLen > 0 -> fcgi_print(REQ,['：',Thing,'の解析結果']),
	fcgi_puts(REQ,'</title></head><body>'),
	fcgi_puts(REQ,'<h1>「成分解析」 on Prolog FastCGI</h1>'),
	fcgi_puts(REQ,'<hr>'),
	fcgi_puts(REQ,'<p>一時期流行っていた成分解析を、Prolog+FastCGIで移植しました。</p>'),
	fcgi_puts(REQ,'<p>SWI-Prolog FastCGI Daemonのデモです。</p>'),
	fcgi_puts(REQ,'<hr>'),
	fcgi_getenv(REQ,'SCRIPT_NAME',SCRIPT_NAME),
	fcgi_puts(REQ,['<form method="GET" action="',SCRIPT_NAME,'">']),
	fcgi_puts(REQ,'<p>'),
	fcgi_puts(REQ,['<input type="text" name="t" value="',EscapedThing,'" tabindex="1" accesskey="1">']),
	fcgi_puts(REQ,'<input type="submit" value="解析する" tabindex="2" accesskey="2">'),
	fcgi_puts(REQ,'<input type="reset" value="取消" tabindex="3" accesskey="3">'),
	fcgi_puts(REQ,'</p>'),
	fcgi_puts(REQ,'</form>'),
	fcgi_puts(REQ,'<hr>').

write_footer(REQ):-
	fcgi_puts(REQ,'<script type="text/javascript">'),
	fcgi_puts(REQ,'var gaJsHost = (("https:" == document.location.protocol) ? "https://ssl." : "http://www.");'),
	fcgi_puts(REQ,'document.write(unescape("%3Cscript src=\'" + gaJsHost + "google-analytics.com/ga.js\' type=\'text/javascript\'%3E%3C/script%3E"));'),
	fcgi_puts(REQ,'</script>'),
	fcgi_puts(REQ,'<script type="text/javascript">'),
	fcgi_puts(REQ,'var pageTracker = _gat._getTracker("UA-4766333-1");'),
	fcgi_puts(REQ,'pageTracker._initData();'),
	fcgi_puts(REQ,'pageTracker._trackPageview();'),
	fcgi_puts(REQ,'</script>'),
	fcgi_puts(REQ,'<script type="text/javascript"><!--'),
	fcgi_puts(REQ,'google_ad_client = "pub-3121031347596821";'),
	fcgi_puts(REQ,'google_ad_slot = "5134098739";'),
	fcgi_puts(REQ,'google_ad_width = 234;'),
	fcgi_puts(REQ,'google_ad_height = 60;'),
	fcgi_puts(REQ,'//--></script>'),
	fcgi_puts(REQ,'<script type="text/javascript" src="http://pagead2.googlesyndication.com/pagead/show_ads.js"></script>'),
	fcgi_puts(REQ,'<hr>'),
	fcgi_puts(REQ,'<p>'),
	fcgi_puts(REQ,'Analyzed and written by <a href="http://ledyba.ddo.jp/">ψ（プサイ）</a><br>'),
	fcgi_puts(REQ,'</p>'),
    fcgi_puts(REQ,'</body></html>').

write_elem(REQ,EscapedThing, 50,Elem,true ,Before) :- Before = true,fcgi_puts(REQ,[EscapedThing,'のもう半分は',Elem,'で出来ています。<br>']).
write_elem(REQ,EscapedThing, 50,Elem,false,Before) :- Before = true,fcgi_puts(REQ,[EscapedThing,'の半分は',Elem,'で出来ています。<br>']).
write_elem(REQ,EscapedThing,100,Elem,_     ,Before) :- Before = false,fcgi_puts(REQ,[EscapedThing,'はすべて',Elem,'で出来ています。<br>']).
write_elem(REQ,EscapedThing,Per,Elem,_     ,Before) :- Before = false,fcgi_puts(REQ,[EscapedThing,'の',Per,'\%は',Elem,'で出来ています。<br>']).

write_lists(_,_,[],_).
write_lists(REQ,EscapedThing,List,BeforeHalf) :- 
	[First | Left] = List,
	[Per,Elem] = First,
	write_elem(REQ,EscapedThing,Per,Elem,BeforeHalf,Before),!, %ここでカットしないと複数探してしまう
	write_lists(REQ,EscapedThing,Left,Before).

write_list(REQ,EscapedThing,List) :- 
	fcgi_puts(REQ,[EscapedThing,'の成分解析結果 : <br><br>']),
	write_lists(REQ,EscapedThing,List,false).

fcgid_independent :- !.
fcgid_handler(REQ) :- Thing = 'ツンデレ', /* とりあえずテスト */
	escape_html(Thing,EscapedThing),
	write_header(REQ,Thing,EscapedThing),
	buffalin(Thing,LIST),
	write_list(REQ,EscapedThing,LIST),
	write_footer(REQ).

/* QUERY_STRINGのパース */


