# torne BonDriver診断プローブ

通常の `BonDriver_Torne.dll` を外部から操作して、選局とTS受信を確認する診断用ツール。
通常ビルド・利用には不要。実機試験では同じチューナーを利用中のアプリケーションを終了してから実行する。

リポジトリのルートで `python winusb/experimental/torne/build_probe.py` を実行してビルドする。
生成された `torne_bondriver_probe.exe` の引数はソースの使用方法表示を参照。

旧実験版の別ホスト・カードDLL生成スクリプトは通常版への統合に伴い整理済み。
現在の利用手順は [統合手順](../../docs/cech-zd1j-integration.md) を参照。

`--receive <index>... --measure-ms 2000 --no-purge`は選局後の診断用Purgeを省く。
既定は従来どおりPurge後に計測する。first_ts_msはSetChannelが返ってからの時間で、
tune_msと合計してアプリケーション境界での最初のTS到着を評価する。
このフラグはBonDriver内部の選局時Purgeを無効にするものではない。

`OPEN PASSED open_ms`はOpenTunerの所要時間、`INITIAL_TS open_to_ts_ms`はその開始から最初のTSまでの時間。DLLロードや映像表示の時間は含まない。

`BAD_TIMING`は観測開始後の最初・最後の異常到着ms（異常なしは-1）、
`WINDOW`は1秒ごとの計数。Bonから返るバッファの到着時刻であり、
USB上のパケット時刻ではない。CC履歴と全区間の合否は窓をまたいで維持する。
形式不正を先に分類するため、形式不正かつTEI付きのパケットはtei欄へ重複計上されない。
