# torne BonDriver診断プローブ

通常の `BonDriver_Torne.dll` を外部から操作して、選局とTS受信を確認する診断用ツール。
通常ビルド・利用には不要。実機試験では同じチューナーを利用中のアプリケーションを終了してから実行する。

リポジトリのルートで `python winusb/experimental/torne/build_probe.py` を実行してビルドする。
生成された `torne_bondriver_probe.exe` の引数はソースの使用方法表示を参照。

旧実験版の別ホスト・カードDLL生成スクリプトは通常版への統合に伴い整理済み。
現在の利用手順は [統合手順](../../docs/cech-zd1j-integration.md) を参照。
