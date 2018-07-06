# Cコンパイラを作るために0からアセンブリを勉強する

## 対象にするアセンブリ。

そのへんでPCを買ってくると大抵x86_64というアーキテクチャが入っているようだ。
いわゆる32bitのアーキテクチャがx86で、その延長線上にある64bitのアーキテクチャがx86_64。x64と書かれたりもする。

## アセンブリをgccで扱う。

適当なCコードのファイル `main.c` があったとして

1. `main.c` をアセンブリにする。
    - `gcc -S main.c -o main.s`
    - アセンブラ GNU as で扱うことができるアセンブリを吐く。
    - 有名なアセンブラには他にNASMとかnaskとか。
    - AT&T記法とIntel記法とあるらしい。
1. `main.s` を実行形式のファイルにする。
    - `gcc main.s -o main.o -no-pie`
    - `-no-pie` を付けないと[位置独立コード](https://ja.wikipedia.org/wiki/%E4%BD%8D%E7%BD%AE%E7%8B%AC%E7%AB%8B%E3%82%B3%E3%83%BC%E3%83%89)を吐く？
    - `gcc` が吐くアセンブリは問題なさそうだけど、アセンブリを手書きした場合はこれをつけないと実行時に死んでしまう。
    - `-no-pie` をつけることによるデメリットは（ライブラリを作るのではない限り）少なそう。
1. `main.o` を動かす。
    - `./main.o` で動く。

Cコンパイラを作る際は、アセンブリを吐くことを目標にして、上記の手順2以降を行って動くことを保証すればよさそう。

Cコードとアセンブリの対応を見るツールとして[Compiler Explorer](https://godbolt.org/)がある。
種々の言語のプログラムとそれをコンパイルしたアセンブリを色付けで対応させて表示してくれる。

## 参考にすべき情報。

- Intel® 64 and IA-32 Architectures Software Developer's Manual
    - [PDFのあるIntelのウェブサイト](https://software.intel.com/en-us/articles/intel-sdm)
    - 長大なので、同じものを別の切り方でいくつかおいてある。
    - IntelのCPU上で動くアセンブリに関する情報がおおよそ全て載っている？
    - ただしあくまでCPUそのものに関する情報しか無い。
- System V Application Binary Interface AMD64 Architecture Processor Supplement
    - [DraftのPDF](https://www.uclibc.org/docs/psABI-x86_64.pdf)
    - どのようにアセンブリを書けばSystem V互換の（つまり\*nix系の）システムで動くかを書いたもの？
        - こういうやつを一般的にABIと呼ぶ？
    - はじめのところに「Intel386のABIを下敷きにして、それから変わったところだけを書くぜ！」（意訳）などと書いてある。
- GNU Assembler Manual
    - [ウェブサイト](https://sourceware.org/binutils/docs-2.30/as/index.html)
    - GNU as で使える擬似命令やコマンドラインオプションなどの説明。
    - `.asciz` と `.ascii` と `.string` の違いなど調べられる。
        - ちなみに `.asciz` と `.string` はナル文字終端、`.ascii` は終端処理がされないらしい。
        - `.asciz` と `.string` の違いは不明。
- x86-64プロセッサでGNU assemblerを使う
    - [Qiita](https://qiita.com/tobira-code/items/ac3169200160566c35af)
    - x86_64でGNU asを使う場合の情報について詳しくまとまっている。
    - 整数の演算について詳しい。浮動小数点の情報はない。
- Qiitaでもアセンブラ・機械語(CPU)
    - [Qiita](https://qiita.com/kaizen_nagoya/items/46f2333c2647b0e692b2)
    - Qiitaで公開されているアセンブリの記事がまとまっている。

