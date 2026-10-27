# 復調器比較用helper（Windows amd64）

通常版は[ネイティブ復調器](../../docs/cech-zd1j-native-frontend.md)をホスト内に組み込むため、
このhelperのビルド・配置は不要。以下の実行手順は診断・旧版比較用。

このディレクトリは元coaxのDiB8096実装を使った独立診断用であり、通常の
DriverHost/BonDriverへの登録や配布構成は変更しない。
USBハンドルはC++診断プロセスだけが所有し、helperは標準入出力の限定プロトコルで
I2C要求を返す。元の高速USB I2C経路とのタイミング差を含めて比較する。

実験用ホストでもこのhelperを使用する。選局コマンド`0x11`は成功時に従来どおり空応答を返す。
元の探索状態が`statusTuneFailed`かつ`demodStop`で確定終了した場合だけ、通常応答の
本文1バイト`01`を返す。USB/I2Cエラーを表す`0xff`応答とは別で、sessionを壊さず
次の明示的選局を許可する。ホストはこの結果だけを早期失敗に使い、探索中の未ロックや
信号レベルでは打ち切らない。旧helperの空応答では従来のロック待ちを維持する。
元の選局・AGC・探索の待ち時間は変更していない。追加アクセサーは`tune_result.go`。

```text
python winusb/tools/torne_frontend/build.py --go <go.exe>
python winusb/tests/run_torne_tests.py
winusb/build/x64/Release-static/torne_usb_probe.exe --receive --timeout-ms 45000 --channels 13,29,30 --duration-ms 10000
```

実機診断は同じtorneを使用するアプリを終了してから実行する。初期化を伴う診断は
完全列挙できた1台だけに限定している。`--duration-ms`省略時は最初のTSで停止する。
指定時は各チャンネルをその時間だけ受信するが、全体の絶対期限は`--timeout-ms`。
選局、全チャンネルの受信時間、停止に足りる予算を指定する。
readは単一のOVERLAPPED要求を逐次処理する診断であり、製品用並列受信workerではない。
終了時は投入したreadを取消・回収してからバッファを解放し、TSエンジンを停止する。

`cc_anomalies`はPIDごとのpayload CC増分不一致（同じCCの再送も含む）。
null PIDを除外し、TEIとdiscontinuity indicatorで比較状態をリセットする。
放送由来の重複とUSB欠落を区別する計数ではない。
`discarded_bytes`には先頭の192バイト境界獲得時の端数も含む。
品質計数は結果と別に報告し、0以外でも転送成功だけで品質合格を主張しない。

`--receiver`は同じ初期化後に`ReceiverBase`派生の診断アダプターを使う。
たとえば`--receiver --timeout-ms 60000 --channels 13,29,30,13 --duration-ms 2000`。
Open→選局前SetCapture(true)→SetParameters(kHz)→Tune→StreamBufferの
Write/HandleRead→受信中の次選局→Closeを試す。workerは192→188変換後のTSを
既存バッファへ1パケットずつ渡し、停止要求でreadを取消・回収してからjoinする。
TS到着を待つ2秒は診断用の暫定値で、製品既定値を決める測定ではない。
復調器の状態監視、統計取得、抜去、障害回復や操作別deadlineを備えた製品実装ではなく、
通常のDriverHostへは登録しない。復調器は引き続きGo診断helperを使用する。

`--receiver-recovery`は最初の選局だけ出力をHigh-ZにしてTS無受信を注入し、
TS待ちの有限失敗、read取消/回収、worker join、次の明示的選局での復帰を試す。
通常のCheckLockエラーだけではReceiverBaseがpollを続けるため、診断アダプターは
capture失敗を保持して`-ECANCELED`で終了し、SetFrequencyでのみその状態を解除する。
これは既存基底クラスの停止条件を使う試験上の対応で、タイムアウトの製品用エラー表現と
操作全体の期限hookは本統合時に扱う。既存PX系の共通ループは変更しない。

`build_comparison.py --coax <original checkout> --go <go.exe>`は元coaxのUSB経路も
使用する独立比較exeを作る。比較対象チェックアウトは変更しない。

個体選択の追加変更は`python winusb/tests/build_torne_identity.py`で2プロジェクトだけ
ビルドし、USBへアクセスしない基盤試験を実行する。
ビルド後、引数なしのprobeは読み取りだけで各interfaceの16桁IDとdescriptorを表示する。
診断引数の末尾へ`--device-id <16桁ID>`を追加すると、列挙順に依存せず1台を選ぶ。
初期化モードは未指定で複数台が見つかった場合や、同じIDが複数あった場合には拒否する。
IDは大文字小文字を正規化したinterface pathのFNV-1a表示値で、製造番号ではない。
USBポート変更やOSによるデバイス再登録で変わり得る。未接続IDから別個体へ自動移行しない。
この選択経路の複数台実機動作は未確認。別個体の同時受信も未確認。

出自とハッシュは`upstream.json`、著作権とライセンスは`NOTICE`、`COPYING`等を参照。
GPL側の条件を選択する。helperを配布する場合は対応ソース・変更内容・ビルド手順と
ライセンスを揃え、静的リンクされるGo runtimeの`GO-LICENSE`も同梱する。
