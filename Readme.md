# InputPipePlugin　　　　　　　　　　　　
 
## ■はじめに
このソフトは、L-SMASH_Works File Reader(lwinput.aui)を別プロセスで実行してあげることによって  
aviutlのメモリ使用量削減を目論む、aviutlの入力プラグインです

ダウンロードはこちらから  
https://github.com/amate/InputPipePlugin/releases/latest

## ■動作環境
・Windows 10 home 64bit バージョン 2004  
・AviUtl 1.00 or 1.10  
・拡張編集 0.92  
※ 拡張編集 0.93rc1 はシーン周りに不具合があるので推奨しません  
・L-SMASH Works r935 release2  
・L-SMASH-Works_rev1096_Mr-Ojii_Mr-Ojii_AviUtl  
上記の環境で動作を確認しています  
XPではたぶん動きません(コンパイラが対応していないため)  

## ■導入方法

https://github.com/amate/InputPipePlugin/releases/latest  
上記リンクから"InputPipePlugin_1.x.zip"をダウンロードして、適当なフォルダに展開します   

lwinput.aui が置いてあるフォルダ(aviutl.exeがあるフォルダ、もしくは "aviutl.exeがあるフォルダ\plugins\"に存在するはず)に、  
InputPipePlugin.aui と InputPipeMain.exe をコピーするだけです  

その後、aviutlのメインメニューから、[ファイル]->[環境設定]->[入力プラグイン優先度の設定]を選択し、  
"InputPipePlugin"を"L-SMASH Works File Reader"より上にドラッグして移動してあげれば完了です  

## ■導入の確認
拡張編集のウィンドウに適当に動画ファイルを放り込んで、"InputPipeMain.exe"のプロセスが立ち上がっていれば成功です  

- 導入が確認できない  
入力プラグイン優先度で、"DirectShow File Reader"がInputPipePluginより上に存在すると動作しないので、InputPipePluginをそれより上に移動させてください

- aviutlが落ちる  
"InputPipePlugin.aui"と"InputPipeMain.exe"が"lwinput.aui"と同じフォルダに存在するか確認してください  
"InputPipeMain.exe"を起動してみて、"lwinput.aui が同じフォルダに存在しています！"と表示されれば大丈夫です  
もしくは、[InputPipePluginの設定]から[プロセス間通信を有効にする]のチェックを外してください

## ■InputPipeMain64.exe について

64bit版 L-SMASH_Works File Reader(lwinput64.aui)が InputPipePlugin.aui と同じフォルダ内に存在するとき、そちらを利用するようになります  
リリース時に 64bit版L-SMASH_Works File Readerが存在しないので正常に動作するかは不明です  

- 想定している内部仕様について  

32bitと64bitでは INPUT_INFO や INPUT_HANDLE のサイズが異なりますが、InputPipeMain64.exeが内部で変換してからInputPipePlugin.auiへ伝えるので、lwinput64.aui側で特に何かする必要はありません  


## ■設定
[ファイル]->[環境設定]->[入力プラグインの設定]->[InputPipePluginの設定]で
プラグインの設定ができます

- ハンドルキャッシュを有効  
動画では説明しませんでしたが、編集で同じ動画ファイルのカットを多用するなどの場合、  
拡張編集プラグインが同じ動画に対して毎回ファイルオープンのリクエストを送ってくるので、  
いちいちL-SMASH Worksプラグインへ ファイルオープンのリクエストを送らずに、  
一度開いたファイルのハンドルを使いまわすようにする設定です  

ぶっちゃけ外部プロセスなんて使わずに、これにチェック入れるだけでいいんじゃないかなって  
実装した後に気づきました  
でも、無駄にはなってないはずです…  

※ハンドルキャッシュ有効時に、同一ファイル名の動画または音声ファイルが、同じフレームの違うレイヤーで使われている時に問題が発生する  

別のレイヤーに同じファイルを配置した場合、1フレーム再生する毎に動画や音声のシークが起きて負荷の重い処理が走り、再生や出力がおかしくなる場合がある  
上記の問題を回避するために、"Altキーを押しながら拡張編集のタイムラインウィンドウにメディアファイルをドラッグドロップする"操作によって、ハンドルキャッシュを有効にしていても、ドロップされたファイルのハンドルをキャッシュから使いまわさずに、新しいファイルとしてオープンするようにします  
ファイル名の末尾に".[alt_XXXXXXXX]"などが付けば、それは別のファイルとして開かれたことを示します  
この機能を使った場合、ドロップされたファイルがあるフォルダに、"ファイル名.[alt_XXXXXXXX].\#junk"というゴミファイルができますが、AviUtl再起動時に、タイムラインに配置したオブジェクトが消えないようにするための処理です  
動画が完成したら申し訳ありませんが、手動で消してください・・・

- プロセス間通信を有効にする  
デフォルトではオンになってます  
外部プロセスを立ち上げて、その中でlwinput.auiを実行するようにします  

外部プロセスとの通信はコストがかかるので、重いと感じたらチェック外してください

- 内部のデータ交換方式  
  * 名前付きパイプ  
    v1.4以前の方式です

  * 共有メモリ   
    v1.5から実装された方式です  
    予めデータ用のメモリを確保しておくので、こちらの方が少し高速です

設定の保存に失敗するので、AviUtlのフォルダを [C:\Program Files]や[C:\Program Files (x86)]フォルダ内に置かないでください

## ■プラグインを入れて不安定になった場合
- 拡張編集の環境設定から、動画ファイルのハンドル数を8から16程度に増やしてみてください
- InputPipePluginの設定から[プロセス間通信を有効にする]のチェックを外してみてください
- 入力プラグインの優先度設定で、"Wave File Reader"を"InputPipePlugin"より上に移動させてみてください
- 動画ファイルがmp4の場合、コンテナをmkvに変更してみるとか(※実験的なもので効果は出ないかも…)

## ■アンインストールの方法
AviUtlのフォルダにコピーした  
InputPipePlugin.aui と   
InputPipeMain.exe  と   
InputPipePluginConfig.ini を削除してください

## ■免責
作者(原著者＆改変者)は、このソフトによって生じた如何なる損害にも、  
修正や更新も、責任を負わないこととします。  
使用にあたっては、自己責任でお願いします。  
 
何かあれば下記のURLにあるメールフォームにお願いします。  
https://ws.formzu.net/fgen/S37403840/
 
## ■著作権表示
Copyright (C) 2019-2023 amate

私が書いた部分のソースコードは、MIT License とします。

## ■謝辞
aviutlの作者様、L-SMASH Worksの作者様  
あなた方の活躍のおかげで、素晴らしい動画がたくさん生まれていることに感謝いたします
ありがとう！

## ■...
よろしければ、\[ [sm35585310](https://www.nicovideo.jp/watch/sm35585310) \] を親作品登録してくれると色々助かります

## ■誰かの役に立つかもしれない情報

- GeForce Experience(ShadowPlay)で録画したファイルを編集しようとしたら音ズレしてる！  
動画プレイヤーで再生したときは音ズレしていないのに、aviutlで編集しようとしたらなぜか音ズレしてしまっている場合、  
→Avidemuxを使ってコンテナをmkvに変更する  
AvidemuxのVideo OutputとAudio OutputをCopyに、Output FormatをMkv Muxerに変更して  
ファイルを読み込んだ後、Saveで出力する  
出来上がったmkvファイルを読み込むとなぜか音ズレしていない  

- OBS Studioで録画したファイルのシークが死ぬほど遅い！  
→Avidemuxを使ってコンテナをmkvに変更する  
上記の手順を実行  
なぜかシークが軽くなっている  

- Detected CTS duplication at frame XXX ってダイアログが出る！  
→Avidemuxを使ってコンテナをmkvに変更する  
以下略

コンテナを変更するだけでなぜ直るのかは謎
 
## ■ビルドについて
Visual Studio 2022 が必要です  
ビルドには boost(1.70~)とWTL(10_9163) が必要なのでそれぞれ用意してください。

Boost::Logを使用しているので、事前にライブラリのビルドが必要になります

Boostライブラリのビルド方法  
https://boostjp.github.io/howtobuild.html  
//コマンドライン  
b2.exe install -j 16 --prefix=lib toolset=msvc-14.2  runtime-link=static --with-log --with-filesystem

◆boost  
http://www.boost.org/

◆WTL  
http://sourceforge.net/projects/wtl/


## ■更新履歴
<pre>

v2.0
・[add] InputPipeMain64.exeを追加 (64bit版 L-SMASH_Works File Reader(lwinput64.aui)が存在するときはそちらを利用するようにした)

v1.10
・[fix] 動画ファイルの幅、高さが4の倍数でないときに強制終了するバグを修正 #5

v1.9
・[update] 開発環境を Visual Studio 2022 に更新
・[fix] lwinput.auiに func_init、func_exitが実装されていれば、それぞれ呼ぶようにした (オリジナルはnullptrだったので呼んでいなかった)
・[change] 内部データ交換方式は、"共有メモリ"をデフォルトにした
・[fix] InputPipePlugin.aui から InputPipeMain.exe 実行時に、カレントディレクトリを設定するようにした (lsmash.iniを読み込めるようになったはず)

v1.8
・[add] 同一フレーム同一ファイル問題に対処、Altを押しながらのファイルドロップで、新規ファイルとして開く処理を追加

v1.7
・[fix] IPC有効時、プラグインの終了時にInputPipeMain.exeをまともに終了させるようにした
・[change] lwinput.auiの各種関数呼び出しにラッパーを噛ませた (例外も握り潰すようにしたので多少落ちなくなったはず)
・[add] 設定の保存に失敗した場合、メッセージボックスを表示するようにした (AviUtlフォルダを [Program Files]や[Program Files (x86)]フォルダ内に置かないでください)
・[fix] IPC有効時、名前付きパイプの準備やInputPipeMain.exeの起動に失敗した場合、IPCを無効化して動作させるようにした
・[fix] ConnectNamedPipeの呼び出しが、タイミングによって失敗することがあるのを修正 
・[fix] 音声が存在しない動画ファイル読み込み時、iip->audio_formatが無効なアドレスを指していたのを修正
・[misc] 使用していないソースコードの削除や INPUT_INFO周りの処理を共通化
・[change] 名前付きパイプのバッファサイズを増加させた (512->4096)

v1.6
・[change]設定で内部データの交換方式を選択できるようにした(デフォルトを名前付きパイプへ変更)
共有メモリだと落ちる場合があるらしいが、再現しないのでエラーが起こりそうな場所にメッセージボックスを埋め込んどいた
・[fix]InputPipeMain.exeがboost::logに依存しないようにした
アンチウィルスがうるさいらしいのでその対策

v1.5
・[fix] aviutl起動時に、InputPipePlugin.auiと同じフォルダに lwinput.aui が存在しない場合プロセスが強制終了するのを修正
・[add] aviutl起動時に、InputPipePlugin.auiと同じフォルダに lwinput.aui が存在しない場合メッセージボックスを表示するようにした
・[change] プロセス間通信有効時、画像と音声の転送に共有メモリを使用するようにした (6割ほど高速化した)
・[change] boostを1.72.0へ更新
・[chagen] std::filesystemからboost::filesystemへ使用ライブラリを変更

v1.4
・[change]プロセス間通信有効、func_read_videoで画像の取得に失敗した場合の処理をInputPipePlugin.auiではなく、InputPipeMain.exeで実行するようにした
・[add]プロセス間通信無効時でも、func_info_getでキャッシュ情報を返すようにした

v1.3
・[fix] func_read_videoで画像の取得に失敗した場合、一度前のフレームを取得してから目的のフレームを取得するようにした [緑の画面が出る対策](ワークアラウンドっぽいがとりあえず動く)

v1.2
・[fix] CreateNamedPipeでPIPE_TYPE_MESSAGEのままだったのを修正
・[add] NamedPipe::Read failed 時に、GetLastErrorの内容を書き込むようにした(上の修正でもう書き込まれないはず…)

v1.1
・[fix] IPC有効時、外部プロセスとの通信にコケるのを修正(マルチスレッドで各種関数が実行されることがあるようなので、排他制御を入れた)
・[change] 名前付きパイプの設定を、バイトストリームモードへ変更(上のバグのせいで変なことしてた)
・[fix] 外部プロセスにて、func_info_getが失敗することを考慮に入れてなかったのを修正
・[change] 名前付きパイプの名前をランダムに変更するようにした

v1.0
・完成
</pre>