# CECH-ZD1Jの通常DriverHost統合

## 配布構成

通常の`build.ps1`でx86/x64のDriverHost_PX4、BonDriver_PX4、WinSCardをビルドします。
復調器のC++実装はDriverHost_PX4へ静的に組み込みます。

| フォルダー | BonDriver | ホスト・カード |
|---|---|---|
| dist/BonDriver_Torne_32bit | BonDriver_Torne.dll（x86） | DriverHost_PX4.exe、WinSCard.dll（x86） |
| dist/BonDriver_Torne_64bit | BonDriver_Torne.dll（x64） | 同名ファイル（x64） |

各フォルダーに同名BonDriver ini、`BonDriver_PX4-T.ChSet.txt`、共通ホスト設定を揃えます。
torne用フォルダーではIT930xファームウェアは不要です。共通のcoax由来ライセンス文書は
`dist/licenses/coax/`へ配置します。配布単位はdist全体です。
Driverフォルダーにはtorne用INFと再署名済み共通カタログを含めます。
既にWinUSBで動作している個体へ再インストールは不要です。

旧配布名`BonDriver_CECH-ZD1J.dll`から、新配布名`BonDriver_Torne.dll`へ変更しています。
同名の`BonDriver_Torne.ini`を揃え、TVTest/EDCBで選択するDLLを変更してください。
USB機種名、GUID、ChSet名、ホスト名に変更はありません。

`[BonDriver]`の`DebugLog=false`でBonDriverログを作成しません。
診断時は`DebugLog=true`で同名`.log`を有効にできます。
設定を省略した既存PX系の診断ログ動作は従来どおりです。
現在のtorne配布設定は1024パケット単位、最大64・最小4バッファです。
torne専用の`OutputErrorPackets=true`を配布INIへ設定し、未訂正TSも出力します。
この組み合わせでの実機再試験は未実施です。過去の128パケット設定の試験結果とは区別してください。
弱い局での実測とtorne専用の`OutputErrorPackets`設定は
[ログと低ビットレート受信](torne-weak-signal.md)を参照してください。

通常ホストの機種enum末尾にCECHZD1Jを追加し、既存機種の数値と生成分岐は維持します。
既存機種の設定部分は変更せずDeviceDefinition15へtorneを追記します。
登録中はdescriptorとreceiver登録までとし、復調器初期化はOpen時に行います。
1台のtorne初期化例外を既存機種の動作へ波及させないため、新分岐内で捕捉します。

## 利用上の区別

DriverHost_PX4と共通のIPC・mutexを使います。先に起動したホストの設定が使用されるため、
古いホストが動作中のまま別フォルダーのホストを起動しても、新しい機種定義へ切り替わりません。
利用中の録画を終了できる時点でホストを入れ替えてください。別の既存アプリやインストール先を
この開発・検証手順で上書きしてはいません。

通常版はcoax由来の復調器処理をC++へ移植したホスト内実装を使うため、
`TorneFrontend.exe`は不要です。移植の構造と比較手順は
[ホスト内統合](cech-zd1j-native-frontend.md)を参照してください。
旧実験専用ホストの生成スクリプトは通常版への統合に伴い整理済みです。

カード列挙・状態・共有・排他・トランザクションは通常WinSCardの共通実装を使います。
winscard.iniによる個体選択フィルターはありません。全リーダーとカードの有無を返し、
復号ソフトが接続先を選びます。torneのカード対応は現状B-CAS/LRCに限定します。

## ビルド

通常ビルドはVisual Studioのv143とPython 3が必要です。
Go 1.26以降は開発用の通信比較試験と移植ソース再生成で使用します。
`build.ps1`は通常の両アーキテクチャの回帰テストとドライバーカタログ検証に加え、
ネイティブ復調器の模擬試験とCECH-ZD1Jの配布を行います。
`tools/check_torne_package.py`で配布物のビット数・必要ファイル・設定を確認できます。
Windows CIにもGoのセットアップと配布物検査を追加しています。CIのリモート実行は未確認です。

## 初回統合版の確認結果（2026-10-27、Go helper使用時）

通常build.ps1によるx86/x64ビルド、TS同期・カード状態・リングバッファの回帰、
Goテストと両ビット数helperビルド、配布物と全17 INFのカタログ照合に成功しました。
記録は検証ワークスペースの`torne-integrated-build.log`。

`torne-integrated-validation-20261027-160917/`では、実際のDriverHost_PX4/通常BonDriver/
通常WinSCardを用い、x86ホストとx64ホストを順に試験しました。ほかのPX機器を初期化しないよう、
試験用設定だけはtorneに限定しています。配布物の共通設定は既存定義を維持しています。

- 各ホストで2台を同時使用し、13/15/29/30の受信と16の無信号失敗を確認。
  正常区間・USB直後ともCC異常0。無信号失敗は623～632ms。
- 無信号失敗後も、次の明示的な正常局選局で受信できた。
- x86/x64ホストとも、通常のWinSCardでカードありのT=1接続と空スロットのNO_SMARTCARDを確認。
  x64 WinSCardからx86ホストへの接続も成功。
- 各ホストに対するx86 APDU試験5秒は19回成功・失敗0、最大63ms/64ms。

probeは無信号のSetChannel失敗を含むため終了コード1を返します。外側の試験が各chの期待結果と
品質を検査しています。無信号の失敗を正常局の失敗と混同しません。

`torne-integrated-concurrent-20261027-161239/`では通常x86ホストでAPDU通信35秒と2台受信を
並行しました。APDUは115回成功・失敗0、最大2790ms。初期化中の応答遅延は残っています。
13/29/30の正常な計4区間は同期/TEI/CC異常/不正ヘッダー0、途中の16は625/672msで失敗し、
次の正常局へ復帰しました。この試験も設定はtorneのみです。

既存PX機の実機受信・torneとの同時利用、統合版でのTVTest映像復号、長時間録画、USB/カード抜去は
未確認です。以前の実験版で利用者が確認したTVTest映像復号を、そのまま統合版の成功とは扱いません。
