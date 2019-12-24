/* ===================================
　　　　　　　InputPipePlugin　　　　　　　　　　　　
 ==================================== */
 
■はじめに
このソフトは、L-SMASH_Works File Reader(lwinput.aui)を別プロセスで実行してあげることによって
aviutlのメモリ使用量削減を目論む、aviutlの入力プラグインです

■動作環境
・Windows 10 home 64bit バージョン 1903
・AviUtl 1.00 or 1.10
・拡張編集 0.92
※ 拡張編集 0.93rc1 はシーン周りに不具合があるので推奨しません
・L-SMASH Works r935 release2
上記の環境で動作を確認しています
XPではたぶん動きません(コンパイラが対応していないため)

■導入方法
lwinput.aui が置いてあるフォルダ(aviutl.exeがあるフォルダ、もしくは "aviutl.exeがあるフォルダ\plugins\"に存在するはず)に、
InputPipePlugin.aui と InputPipeMain.exe をコピーするだけです

その後、aviutlのメインメニューから、[ファイル]->[環境設定]->[入力プラグイン優先度の設定]を選択し、
"InputPipePlugin"を"L-SMASH Works File Reader"より上にドラッグして移動してあげれば完了です

■導入の確認
拡張編集のウィンドウに適当に動画ファイルを放り込んで、"InputPipeMain.exe"のプロセスが立ち上がっていれば成功です

・導入が確認できない
入力プラグイン優先度で、"DirectShow File Reader"がInputPipePluginより上に存在すると動作しないので、InputPipePluginをそれより上に移動させてください

・aviutlが落ちる
"InputPipePlugin.aui"と"InputPipeMain.exe"が"lwinput.aui"と同じフォルダに存在するか確認してください
"InputPipeMain.exe"を起動してみて、"lwinput.aui が同じフォルダに存在しています！"と表示されれば大丈夫です

■設定
[ファイル]->[環境設定]->[入力プラグインの設定]->[InputPipePluginの設定]で
プラグインの設定ができます

〇ハンドルキャッシュを有効
動画では説明しませんでしたが、編集で同じ動画ファイルのカットを多用するなどの場合、
拡張編集プラグインが同じ動画に対して毎回ファイルオープンのリクエストを送ってくるので、
いちいちL-SMASH Worksプラグインへ ファイルオープンのリクエストを送らずに、
一度開いたファイルのハンドルを使いまわすようにする設定です

ぶっちゃけ外部プロセスなんて使わずに、これにチェック入れるだけでいいんじゃないかなって
実装した後に気づきました
でも、無駄にはなってないはずです…

〇プロセス間通信を有効にする
デフォルトではオンになってます
外部プロセスを立ち上げて、その中でlwinput.auiを実行するようにします

外部プロセスとの通信はコストがかかるので、重いと感じたらチェック外してください

■プラグインを入れて不安定になった場合
・拡張編集の環境設定から、動画ファイルのハンドル数を8から16程度に増やしてみてください
・InputPipePluginの設定から[プロセス間通信を有効にする]のチェックを外してみてください
・入力プラグインの優先度設定で、"Wave File Reader"を"InputPipePlugin"より上に移動させてみてください
・動画ファイルがmp4の場合、コンテナをmkvに変更してみるとか(※実験的なもので効果は出ないかも…)

■アンインストールの方法
InputPipePlugin.aui と 
InputPipeMain.exe  と 
InputPipePluginConfig.ini を削除してください

■免責
作者(原著者＆改変者)は、このソフトによって生じた如何なる損害にも、
修正や更新も、責任を負わないこととします。
使用にあたっては、自己責任でお願いします。
 
何かあれば下記のURLにあるメールフォームにお願いします。
https://ws.formzu.net/fgen/S37403840/
 
■著作権表示
Copyright (C) 2019 amate

私が書いた部分のソースコードは、MIT License とします。

■謝辞
aviutlの作者様、L-SMASH Worksの作者様
あなた方の活躍のおかげで、素晴らしい動画がたくさん生まれていることに感謝いたします
ありがとう！

■誰かの役に立つかもしれない情報

〇GeForce Experience(ShadowPlay)で録画したファイルを編集しようとしたら音ズレしてる！
動画プレイヤーで再生したときは音ズレしていないのに、aviutlで編集しようとしたらなぜか音ズレしてしまっている場合、
→Avidemuxを使ってコンテナをmkvに変更する
AvidemuxのVideo OutputとAudio OutputをCopyに、Output FormatをMkv Muxerに変更して
ファイルを読み込んだ後、Saveで出力する
出来上がったmkvファイルを読み込むとなぜか音ズレしていない

〇OBS Studioで録画したファイルのシークが死ぬほど遅い！
→Avidemuxを使ってコンテナをmkvに変更する
上記の手順を実行
なぜかシークが軽くなっている

〇Detected CTS duplication at frame XXX ってダイアログが出る！
→Avidemuxを使ってコンテナをmkvに変更する
以下略

コンテナを変更するだけでなぜ直るのかは謎
 
■ビルドについて
Visual Studio 2019 が必要です
ビルドには boost(1.70~)とWTL(10_9163) が必要なのでそれぞれ用意してください。

Boost::Logを使用しているので、事前にライブラリのビルドが必要になります

Boostライブラリのビルド方法
https://boostjp.github.io/howtobuild.html
//コマンドライン
b2.exe install -j 16 --prefix=lib toolset=msvc-14.2  runtime-link=static --with-log

◆boost
http://www.boost.org/

◆WTL
http://sourceforge.net/projects/wtl/


■更新履歴
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
