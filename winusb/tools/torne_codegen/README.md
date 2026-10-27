# MB86H47 FR30 プログラム生成

このディレクトリーはcoaxコミット `2d9f2c263a3b080ecef3a8ede1eb86c6afa48405`
のFR30生成器とその試験を保持する。通常のVisual Studioビルドは生成済み
`winusb/src/DriverHost_PX4/torne/mb86h47_programs.hpp` を使うため、Goの導入は不要。
再生成にはPython 3.10以降とGo 1.26以降が必要となる。

```text
python generate.py --go <go.exeのパス>
python generate.py --go <go.exeのパス> --check
python compare_upstream.py --go <go.exeのパス> --coax <固定コミットのcoaxチェックアウト>
```

`generate.py` は外部モジュールの取得を無効化してFR30、simulator、fcの試験と
ヘッダー生成を実行する。出力とGoキャッシュは `winusb/build/` 以下に置かれる。
`--check` は生成済みデータとの比較だけを行う。
`compare_upstream.py` は未変更のcoaxパッケージを一時領域に複製し、元のruntime定数を
使った出力とヘッダーが完全一致するか確認する。どちらもUSBアクセスを含まない。

取り込み元の各ファイルのSHA256とコミットは `upstream.json` に記録されている。
著作権・SPDXは元のファイルに保持され、C++組込みではGPLv2の選択枝を適用する。
ライセンス本文は `COPYING`、元プロジェクトの説明は `NOTICE` を参照。

元ソースからの変更は以下の範囲に限定される。

- `addressing.go`: USB runtimeへ依存する末尾の2メソッドと不要なencoding/binary importを除外。
  元ファイル全体は `reference/addressing.go.txt` に保存。
- `constants.go`: `reference/` 内の元runtimeファイルからconst宣言と`core0Ring`型を抽出。
  I2C status/control/dataのアドレスは元 `regs/usb.go` の配置を数値化。
- `export_test.go`: C++配列を出力する追加のビルド用試験。
- `go.mod`: CLI用の未使用依存を除き、標準ライブラリーだけのモジュールに限定。

coreプログラムそのものの動作変更はなく、`compare_upstream.py` が完全一致を検証する。
生成器の試験成功は、C++起動シーケンスや実機受信の成功を意味しない。
