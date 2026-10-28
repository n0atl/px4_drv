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

## BonDriverごとのtorne個別指定

DriverHostは接続中のtorneをすべて登録する。別系統のアンテナにつないだtorneを使い分ける場合も、
ホスト側の個体選択設定は不要。
BonDriver側の既存項目`ReceiverName`で、利用する受信機を限定できる。
共有ホストの通信仕様や、新しい設定項目の追加は不要。

たとえばDLLとINIを同じフォルダー内でA用・B用に複製し、
`BonDriver_Torne_A.dll`と`BonDriver_Torne_A.ini`の組に以下を設定する。

```ini
[ReceiverDefinition0]
DeviceName="Sony torne CECH-ZD1J"
DeviceGUID="{2924f08b-94b8-5a87-94b9-95798570cd93}"
System="ISDB-T"
ReceiverName="[0123456789abcdef]"
```

名前全体による次の指定も同じ個体を選ぶ。`DeviceName`は機種の選択なので、
個体IDを含む名前を`DeviceName`へ移す必要はない。

```ini
ReceiverName="Sony torne CECH-ZD1J ISDB-T Receiver [0123456789abcdef]"
```

DriverHost側の`DeviceId`設定は廃止し、古いINIに残っていても読み取らない。
個別指定はBonDriver側の`ReceiverName`に一本化した。BonDriver側にも`DeviceId`項目は追加していない。

角括弧内のIDは説明用の例で、実際の個体ID（16桁の16進数、大文字・小文字どちらでも可）へ置き換える。
B用のDLL・INIではBのIDを指定する。両方のBonDriverが同じDriverHostを利用し、
それぞれ別のtorneを開く構成になる。
この角括弧付きIDの省略記法は標準torne定義専用で、ホストの受信機表示名を変更しても利用できる。
従来の名前全体による完全一致指定も維持しており、それは他機種でも利用できる。
個体IDはUSB接続パス由来で、製造番号ではない。同じポート・ハブ構成では通常同じ値だが、
ポート・ハブ構成・ドライバーなどの変更で接続パスが変わればIDも変わる可能性がある。
同じポートへ別のtorneを接続した場合も、シリアル番号がないため本体の入れ替えを識別できない可能性がある。

個体名はBonDriver側の`[BonDriver]`で`DebugLog=true`にし、チューナーを開いて確認できる。
同名`.log`の次の行から、`ReceiverName`の引用符内をそのままコピーする。

```text
OpenTuner: selected DeviceName="Sony torne CECH-ZD1J" ReceiverName="Sony torne CECH-ZD1J ISDB-T Receiver [0123456789abcdef]" Index=0
```

DriverHostのログは無効のままでよい。確認後はBonDriverの`DebugLog=false`へ戻せる。
ログへ出るのは実際に開いた個体で、未使用個体の一覧ではない。
同じDLLを複数プロセスから使う場合、通常の`.log`が使用中なら
`BonDriver_Torne.<PID>.log`へ記録する。選択結果の行にもPIDを出す。
従来は通常ログを最初のプロセスが排他的に開いた後、2つ目以降のログ出力が失敗していたため、
同じファイルだけを見ても複数個体の割当結果を確認できなかった。

実機のUSB serial descriptor indexは0で、PnPの固有ID能力も報告されていなかった。
WindowsのContainer IDはシリアル番号などから生成される場合と、ランダムに生成される場合があり、
その存在だけでは本体固定IDと判断できない（[Microsoftの説明](https://learn.microsoft.com/en-us/windows-hardware/drivers/install/how-usb-devices-are-assigned-container-ids)）。
現在の個体でポート変更後も本体を追跡できる識別子は未確認のため、既存の選択IDを維持した。

`ReceiverName`は通常は完全一致フィルターで、標準torne定義だけ角括弧付き16桁IDの省略記法も使える。
指定個体が未接続・使用中なら、その定義では
別個体へ切り替わらない。自動選択へフォールバックする別のReceiverDefinitionを追加した場合は
その定義も試されるため、厳密な固定が必要なINIには自動選択の定義を併記しない。
`ReceiverName`を省略すれば、従来どおり空いているtorneを自動選択する。
省略記法・完全一致・不正ID・他機種への非適用はx86/x64の模擬試験で確認した。
x64ホスト・x86 BonDriverで2台を同時に開き、異なる個体の選択結果が通常ログとPID付きログへ
分かれること、続いて角括弧付きIDでA/Bを固定して意図した個体を開けることを実機確認した。
DriverHostのログは無効のまま実施した。長時間・抜去中の固定選択は未検証。
省略記法には更新後のDriverHostが必要で、以前のホストでは名前全体で指定する。

## 選局高速化

選局高速化は設定不要。BonDriverの配布設定は従来どおり1024パケット単位。

高速化は常時有効。初回DC較正の待機と状態確認ポーリングを短縮し、同じ個体のUHF温度補償を
10秒以内だけ再利用する。同期に関わる段階の待機下限と操作全体の期限は維持する。
搬送波といずれかのMPEG層がロックした時点でTS出力を始めるため、
`OutputErrorPackets=true`では冒頭に未訂正・形式不正パケットが出る場合がある。

成功局の伝送設定は同じ個体に記憶する。
初めて選ぶ局は通常探索。保存値で明示的に失敗した場合は破棄し、残りの期限内で通常探索へ戻る。
USB/I2Cエラー時は探索を重ねず失敗を返す。
UHF・ソフトウェア較正型のキャパシタ値はADCで再確認して再利用し、不一致なら従来の較正へ戻る。

正常なフロントエンド状態はホスト存続中だけ保持し、Bonを閉じる際は受信を止めてスリープする。
ホストの自動終了条件は従来どおりで、終了・抜去・再初期化後は記憶が消える。
ファイルへの永続化や個体間の共有はない。設定変更はホスト終了後に反映する。

同一個体の41/47/49chを使った研究版1024パケットの短時間比較では、公開時の版に対し、
初回Bonオープンから生TSまで約3.13→2.36秒、未選局局約1.97→1.51秒、
再選局約1.95→0.81秒、同一ホストでのBon再オープン約3.10→0.80秒だった。
ホスト起動時間を除いた中央値であり、映像表示開始時間ではない。
高速版の冒頭には最大約0.26秒の形式不正があり、1秒以降の窓に追加異常はなかった。
この研究版の結果を、統合後の長時間録画や弱電界での成功としては扱えない。
検証範囲は[選局調査](torne-tuning-investigation.md)も参照。
