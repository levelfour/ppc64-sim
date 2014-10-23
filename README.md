ppc64-sim
====

[#情報科学基礎実験](https://twitter.com/search?src=typd&q=%23情報科学基礎実験)のC言語上級者向け課題1のPowerPC64 Simulator．

# 特徴
+ 逆アセンブル機能付き
+ Linuxのシステムコール呼び出し可能（つまり正確にはppc64-linux simulator）
+ ELFオブジェクトファイルフォーマットに対応
+ ELF再配置情報に基づいてアドレスを再配置
+ シミュレート後にREPLを起動，レジスタ等の中身を確認可能

# 使い方
```
$ make
$ ./obj/ppc64-sim [object-file-name]	# シミュレート
$ ./obj/ppc64-sim -d [object-file-name]	# 逆アセンブル
```

## REPL-mode
レジスタの中身を表示するには以下のように入力する．
```
> %r1
r1=0x0000000000200000
```
exit，quitでREPL-mode終了．その他のコマンドは未実装．
