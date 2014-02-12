rbind
=====

rbind is alternative of std::bind that is reference base.  
It has additional function using special placeholders together.

rbind は std::bind を参照ベースにしたものです。  
専用のプレースホルダと組み合わせれば追加機能も使えます。

  all binded arguments are reference base without std::ref.  
  bindしたすべての引数は参照ベースであり、std::refは必要としない。

  you can set default values for specital placeholders(_1st, _2nd, ...)  
  専用プレースホルダ( _1st, _2nd, ... )にはデフォルト引数を設定できる

  you can let arguments be optional  
  引数をオプションにできる

  you can set a value converter for each placeholder with yield method  
  yield メソッドで各placeholderに値の変換機能を付加できる

  you can set a placeholder for the first argument  
  第一引数もプレースホルダにできる

  variadic templates are not used then it accept 9 or fewer arguments  
  可変長テンプレートは使っていないので引数の数は9個まで
