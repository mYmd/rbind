rbind
=====

2015/10/06  
型の誤りを修正（size_t と int ）したら復活！  

2015/03/16  
clangでもgccでもVCでもコンパイルできていたのに、突然エラーが出るようになった。  
rbind以外のソースコードをmiscellaneouに移動し、こちらは無期限停止とする。  

rbind is alternative of std::bind that is reference base.  
It has additional function using special placeholders.

rbind は std::bind を参照ベースにした関数。  
専用のプレースホルダと組み合わせれば追加機能も使える。

  all binded arguments are reference base without std::ref.  
  rbindしたすべての引数は参照ベースであり、std::refは必要としない。

  you can set default values for specital placeholders _1st, _2nd, ...  
  専用プレースホルダ _1st, _2nd, ... にはデフォルト引数を設定できる
  
  You can create range of placeholder  
  プレースホルダを範囲で指定できる  
    range(_2nd, _5th)      --->    _2nd, _3rd, _4th, _5th  
    until(_4th)      --->    _1st, _2nd, _3rd, _4th  

  you can let arguments be optional  
  引数をオプションにできる

  you can set a value converter for each placeholder with yield method  
  yield メソッドで各placeholderに値の変換機能を付加できる

  you can set a placeholder for the first argument that is a functor  
  第一引数（すなわちファンクタ）もプレースホルダにできる

  rbinded object is a independent functor itself if in nested situation.  
  it is different from std::bind, but you can let it dependent functor with unary operator *.  
  rbindオブジェクトは単にファンクタであり、ネストした場合でも従属して評価はされない。  
  この点はstd::bindとは異なる。ただし単項 * 演算子を付ければ従属した評価がなされる。

