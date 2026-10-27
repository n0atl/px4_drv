# Sony torne CECH-ZD1J 対応設計

## 対象と確認範囲

対象は Windows amd64 の WinUSB 版、地上 ISDB-T 13セグメント・6 MHz。
調査基準は coax `2d9f2c263a3b080ecef3a8ede1eb86c6afa48405`、
px4_drv `cee43301cd76125d4f5d27a4876a9c8d99311a22`
（上流の既定ブランチ `feature/tsmf-tlv-v61`）。以下の通信手順はソースからの確認であり、
本変更による実機確認結果ではない。coax でまれに発生する選局後の無受信は未再現で、原因も未確定。

段階1～2の追加コードはホストのビルド・機種登録・既定設定から独立した部品である。
現時点では torne 用 BonDriver としての利用は未対応。既存機種のコードに変更を加えず、
USB要求の符号化、短い転送の検出、192→188バイト変換を先に検証する。

## 1. coax の通信経路

主な参照箇所:

| 層 | coax のファイル |
|---|---|
| ボード、Open/Tune/Start/Stop | `internal/coaxd/usb/board/cechzd1j/cechzd1j.go` |
| USBメモリアクセス、クロック、GPIO | `internal/coaxd/usb/bridge/mb86h47/mb86h47.go` |
| アドレス拡張、USBハンドラー | 同ディレクトリ `addressing.go` |
| I2C、core1、メールボックス | 同ディレクトリ `i2c.go`、`core1prog.go` |
| コマンドの一括転送 | 同ディレクトリ `command.go` |
| core0、TSリング、DMA、フレーマー | 同ディレクトリ `stream.go`、`core0prog.go` |
| 復調器・チューナー | `internal/coaxd/demod/dib8096/*.go` |

USB ID は `054C:04B2`。interface 0、alternate 0。
接続個体のdescriptorでは `0x81` bulk/512、`0x02` bulk/512、`0x83` interrupt/64、
`0x84` bulk/512を確認した。`0x83` はcoax内部の `Bulk` メソッドを通るが、
実際のendpoint種別はinterruptである。Windowsの `WinUsb_ReadPipe` は両種別を扱える。
vendor request `0xBC`、OUT `0x40`、IN `0xC0` でメモリへアクセスする。
拡張後のアドレスは `wValue=((addr>>8)&0xff00)|(addr>>24)`、`wIndex=addr&0xffff`。
データは big endian。16bit レジスターは32bitワードの上位半分にあり、
下位半分を保存する read-modify-write が必要。ブロック要求は最大64バイト。
書き込みも実転送長を確認し、短い転送を成功扱いにしないことが移植時の追加条件となる。

Open は以下の順序になる。

1. USBを開き、interface/alternateを選択。
2. `Bridge.Start()` が状態 `0x82008` を検査。非ゼロなら再挿入を要求する。
   アドレス拡張、クロック、SDRAM、例外ベクター、I2C初期化、core1プログラムのロードと生存確認を実行。
3. 高速コマンドチャネルは OUT `0x02` と IN `0x83`。512バイト、16スロット、
   先頭と末尾の連番、応答の連番とエラーを検証する。
   セットアップに失敗するとメモリ要求経由のリングへ戻る。
4. GPIO6で電源、GPIO0でDiB8096リセット（50ms、解除後10ms）。
5. I2C 8bitアドレス `0x12`、レジスター1287へ3を書いて単一復調器を列挙。
6. DiB8000識別・リセット、内部I2C master経由でDiB0090登録・較正、両者のwake、出力設定。

core1 はI2Cを実行する。通常レジスター操作は16スロットのリング、その他は最大32バイトの
メールボックス転送で、write/readの間にrepeated startを使う。
I2C側500ms、USB制御1,000ms、コマンド500msはcoaxの値であり、Windows移植後の測定値ではない。
core0/core1やUSB要求ハンドラーの機械語はcoax内の生成器から生成される独自プログラム。
単なる固定レジスター列の移植だけでは動作しない。

## 2. 初期化、選局、ロック

DiB8096はDiB8000とDiB0090の複合部品。12MHz水晶、PLL比18（選局時17～20を再計算）、
内部54MHz、188バイトMPEG出力、parallel gated clockが基準。
STK8096GP由来のAGC、PLL、WBDテーブルと、チューナー較正手順を含めて移植対象となる。
GPIO0/5はチューナーsleep/reset、GPIO3は帯域切替、GPIO1はLEDに使われる。

`frontend.Tune()` は `tuned=false`、`PrepareStream()`、周波数をHzへ正規化、
`Demod.Tune()`、`tuned=true` の順。復調器はHigh-Z、チューナー選局、AGC、
FFT/GI/TMCC探索、FEC/MPEG待機、出力モード復帰を実行する。
AGC待機を10ms単位へ切り上げる理由があるため、最適化で短縮するとロックを損なう可能性がある。

注意点は `setFrontend()` が探索失敗状態からもnilで終了し得ること。
`Tune()` 成功はUSB/I2C処理の終了を表し、受信成功とは異なる。
`Status()` のFEC bit12かつMPEG bit5～7の少なくとも一つでLOCKとなる。
`getFrontend()` の早すぎる呼び出しも同期を損なう旨がソースに明記されている。
外側の5秒期限はAGCとチューナー選局の後に作成されるため、選局全体の上限ではない。

## 3. TS開始と無受信の調査

`PrepareStream()` はセットアップをキューへ積み、I2Cの待機中 `Idle()` に進める。
TSエンジン `0xD00000`、SDRAMリング `0xAE4000`・6MiB、core0、EP1 DMAを設定。
`StartStream()` はリングの書込みをflushし、残ったステップを実行、入力とwriterを有効化する。
ボード側はその前に復調器出力をparallel gatedへ設定する。
`StopStream()` はreader停止、core0保持、入力停止、writer停止、復調器High-Z。

EP `0x81` の192バイト単位は、先頭4バイトの到着時刻と188バイトTS。
coaxの読み取りは初回Readで複数の非同期転送を投入する。要求サイズは1536の倍数、上限64KiB。
Read待機300msは「一定時間データがない」という通知であり、恒久故障の判定ではない。
`StartStream()` は実データ到着を確認する前に成功する。

切り分けが必要な候補（いずれも未確定）:

- 探索処理は終了したが未ロック、またはロック喪失。
- I2C待機に重ねたストリーム準備とflush、エラー保持の順序。
- EP1 resetのポーリングは最大100回だが、残留bitをエラーにする処理がない。
- USB転送投入が入力開始より後になることと、残留データ・DMA状態の関係。
- core0、TSリングwriter、EP1それぞれの停止。生データ到着と同期失敗は別問題。

各選局へgeneration番号を振り、個体ID、周波数、開始/ロック/先頭USB/先頭TS時刻、
生バイト数、同期済みパケット数、短転送、タイムアウト、USBエラー、ロック生値、
リングwrite位置 `0xD02038`、core0カウンター `core0Base+0xA3F00`、
trap record `0x41FF0000` を必要時に記録する。診断読取りもUSB制御帯域を使うため、
常時高頻度で読む方式はTS同時負荷試験が必要。

## 4. DriverHost と BonDriver への追加方針

現行経路は `BonDriver_PX4.dll → ctrl/data名前付きパイプ → DriverHost_PX4 →
ReceiverManager → ReceiverBase → 機種別Device`。
`DeviceDefinitionSet` が設定を読み、`DeviceManager` がinterface GUIDをSetupAPIで列挙し、
通知パスで到着・抜去を管理する。機種は明示的factory分岐で作成される。
`DeviceBase` はWinUSBハンドルを既に所有し、IT930x構造への依存はあるが、
コンストラクターそのものはIT930xコマンドを送らない。

追加候補は `CechZd1jDevice : DeviceBase`、1個の `CechZd1jReceiver : ReceiverBase`、
MB86H47メモリ/I2C/ストリーム層とDiB8096層。
IT930x、PXMLT、共通TS同期ヘルパー、既存INIsの待機値に変更を加える必要はない。
通常TS同期条件は既存 `driver/ts_sync.h` を使用し、192バイト外枠の探索のみtorne専用に置く。
TEIパケットは同期獲得の根拠から外すが、同期済み入力のTEIは計数して上位へ残す。

BonDriverの `OpenTuner()` は選局前に `SetCapture(true)` を呼ぶ。
従って `capture_requested` と `stream_running` を別に持ち、未選局時は要求のみ保持する。
選局時は対象torneだけのI/Oを停止・回収、端数・バッファ・旧generationを破棄し、
High-Z、選局、ロック、reader投入、出力/DMA開始、最初の有効TS確認へ進む。
`SetCapture(false)`、Close、抜去は待機を解除し、I/O回収後にハンドルを解放する。

`ReceiverBase::Tune()` のtimeoutは `SetFrequency()` 後に開始する。
torneの全体期限をそのまま満たせないため、統合前に専用の期限受渡しが必要。
候補はtorne用opt-in virtual hookで、既定実装は現行動作をそのまま通す。
公開パイプ構造体を変更せず、既存PX系に新しい待機を追加しないことが条件となる。
重い初期化を現行 `DeviceManager::Add()` の一覧ロック下に追加すると他機器へ影響するため、
torneのみ準備workerへ分離し、完了時の短い登録操作に限定する設計が必要。
取り外しと準備完了が競合する場合はgenerationとavailabilityを確認する。

## 5. 個体識別と複数台

coaxのINFは `USB\VID_054C&PID_04B2`、interface GUID
`{B8F4B677-E19A-4BEB-A60C-C0A7AC2D0F6B}` を登録する。
これと同じGUIDなら既存のWinUSBバインドを再利用できる可能性があるが実機確認が必要。
GUIDだけで信用せず、開いたハンドルの実VID/PID、alternateとendpoint属性も照合する。

製品GUID、interface GUID、個体IDは別概念。同じ製品の複数台は各interface pathで管理し、
I/O、I2C連番、端数、停止event、復旧状態を全て個体ごとに所有する。
serialが一意ならそれを使用し、なければinstance/location pathに基づくIDが候補。
USBポート変更後も同じIDになるとは保証できず、列挙順のindexも固定個体指定には使えない。
既存ReceiverManagerは未使用の受信機を選び、data_idで排他するので空き個体の同時利用に適する。
固定指定は専用receiver nameに個体IDを入れて既存ReceiverName条件で選ぶ方式を検討する。
現行Indexが単なるチューナー番号か個体を含むかはバックエンドの登録方式で異なるため、
torneで一律0を設定して固定個体選択の根拠にすることはできない。
外部coaxと同じ機器を同時に制御しないためのプロセス間所有権確認も統合試験項目となる。

## 6. タイムアウトと復旧案

操作全体のsteady_clock絶対期限を、チューナー/I2C/USB待機へ残時間として渡す。
coaxの既存待機値は移植比較の出発点で、製品既定値は実測後に決定する。
以下は実機で安全性と効果を確認するための順序案であり、自動復旧の実績を表さない。

| 状況 | 判定と動作案 |
|---|---|
| 未ロック | 残期限内で探索、期限超過は失敗。無条件にUSBをresetする理由にはならない |
| ロック済み・USBなし | カウンターsnapshot、対象reader cancel/join、出力High-Z、対象streamのみ再構成 |
| USBあり・TSなし | 192バイト境界、タイムスタンプ、DMAデータを記録。framer再同期と原因調査 |
| stream再開で復旧しない | 残期限内で同一周波数の再選局を試験。回数は有限・設定可能、既定有効化は実測後 |
| 切断/cancel | 再試行を中止、availabilityを無効化、パイプへ失敗を返す |
| cold-state不一致/core障害 | 闇雲な再初期化では状態を再構築できる保証がない。明示失敗と再挿入案内 |

`Start()` の「既に初期化した同じchipへ再呼出し可能」というコメントは、
実際のstateチェックやpower-up状態維持と照合が必要。USB resetやClose/Openが
power-cycleと同等とはソースだけでは断定できない。
復旧中の短い転送や停止失敗も記録し、旧TSを新チャンネルの成功として数えない。
受信中のgap監視は選局直後の先頭TS監視と分け、短いgapだけで録画中に再初期化しない。

## 7. ライセンスと配布

coaxのNOTICEと各SPDXを確認した結果、`cechzd1j` と `mb86h47` の主要ファイルは
`Apache-2.0 OR GPL-2.0-or-later`、`dib8096` はLinux media由来の `GPL-2.0-only`。
px4_drvへ組み込むコードはGPLv2の選択枝を使う方針。
ApacheのみのINF等をコピーする場合は別途その条項を保持する必要がある。

移植ファイルにはTill0196、DiBcom等の原著作権、該当SPDX、元ファイル、元コミット、
移植・変更箇所を残す。GPLv2本文を含め、バイナリ配布ではGPLv2第3条に従う対応ソース
（変更を含む全必要ソース、生成器、ビルド手順）を同梱する方式を基本とする。
単に上流URLだけ添えても変更版の対応ソース提供にはならない。
独自coreプログラムは生成元・生成条件・生成結果の対応を残す。
メーカー由来ファームウェアの再配布権はcoaxのライセンスから導けない。
この設計でベンダーバイナリを新たに配布する予定はない。

参照: [coax NOTICE](https://github.com/Till0196/coax/blob/2d9f2c263a3b080ecef3a8ede1eb86c6afa48405/NOTICE)、
[px4_drv LICENSE](https://github.com/n0atl/px4_drv/blob/cee43301cd76125d4f5d27a4876a9c8d99311a22/LICENSE)。

## 8. 段階と合格条件

1. **基盤**: 既存amd64基準ビルド、USB要求のmock試験、短転送、framerの分割・誤同期・reset試験。
   ホストから未参照にして既存動作への変更を避ける。
2. **USB専用試験**: 個体一覧・descriptor照合、メモリ読書き、coreプログラム生成の再現、
   cold/warm起動、制御のエラー注入、キャンセル。ドライバー変更はこの段階で対象個体だけに実施。
3. **復調器**: DiB8096移植、mock I2Cによる重要レジスター列・整数演算の比較、実機の初期化、
   複数周波数でロックと生TS取得。自動復旧無効で無受信時snapshotを採取。
4. **ホスト統合**: 明示的なCECHZD1J型、専用設定、capture予約と期限hook、非同期登録、
   専用BonDriver設定。初期はopt-in、既定の既存設定へ追加する前に回帰試験。
5. **安定化**: 実測した有限タイムアウトと復旧、TVTest/EDCB、複数台、USB抜去/再挿入、
   Close/Open反復、ホスト再起動、長時間録画、PX-MLT5PEとの同時受信。
6. **配布**: amd64本体・試験、必要なx86互換確認、INF CRLF正規化・catalog署名/検証、
   対応ソース・著作権・検証範囲を同梱。

実機試験ではチャンネル切替回数、lock/先頭TSのp50/p95/max、無受信数、復旧成否、
同期/TEI/CCエラー、ドロップ、D/E/S、受信時間を記録する。
PX-MLT5PE側はtorneの初期化・切替・抜去中も計数を継続し、前後比較する。
自分が起動していない録画・DriverHostを停止して試験することは避け、別の試験環境を用意する。

## 9. 実装済み部品と再現手順

`src/DriverHost_PX4/torne/` の以下の部品をamd64でコンパイル・模擬試験済み。
いずれも `DeviceManager` へ未登録で、通常のホスト起動から実機へ要求を送る経路はない。

| ファイル | 実装範囲 |
|---|---|
| `mb86h47_protocol.hpp` | メモリ要求、big endian、最大64バイト、短転送、16bit RMW |
| `stream_framer.hpp` | 192→188変換、分割・端数、3単位での同期獲得、TEI計数 |
| `winusb_control.*` | OVERLAPPED制御転送、共有絶対期限、停止event、対象要求だけのcancelと回収 |
| `mb86h47_programs.hpp` | 固定したcoaxと完全一致するFR30プログラムとhandlerパッチ |
| `mb86h47_bootstrap.*` | handler照合・二重bankの検証/切替、クロック、trap、I2C core1、生存確認、復調器reset |
| `mb86h47_i2c.*` | 同期mailbox経路、repeated start要求、応答連番、NACK分類、期限超過後のsession無効化 |
| `mb86h47_stream.*` | core0生成コード、TS ring、EP1 DMA設定、reset完了照合、prepare/start/stop |

mailbox経路は初期統合用であり、coaxの高速ring/command転送と同じ速度は未確認。
bulkコマンド経路、DiB8096のC++移植、ホスト登録は残っている。
診断では元DiB8096のGo実装を限定IPCの子プロセスで動かし、USBをC++側だけに持たせた。
`tests/torne_capture.*`で有限時間の非同期readと品質計数を行う。
`tests/torne_receiver_adapter.*`はReceiverBaseの契約と既存StreamBufferを試す
実機診断専用アダプターであり、製品用バックエンドではない。
選局前capture予約、kHz→Hz変換、最初のTS待ち、worker停止/cancel回収/join、
次選局前のpurgeを実装する。read workerと制御は同時に復調器コマンドを発行しない。
TS到着までの2秒は診断の暫定値で、量産既定値ではない。
状態監視/統計、抜去、有限復旧、操作ごとの期限、非同期機種登録は本統合前に必要である。

Developer Command Promptまたは通常のPython環境から:

```text
python winusb/tests/run_torne_tests.py
```

このスクリプトはv143のMSBuildを探索して既存ソリューションと独立したtorne試験を
`Release-static|x64` でビルドし、USBを開かない試験だけを実行する。
ログは `winusb/build/torne-validation/` 以下。
USB descriptor用の `torne_usb_probe.exe` はビルドだけで、自動実行の対象外。
引数なしの実行は現在のUSB descriptorの確認に限定される。
`--initialize --timeout-ms <操作全体の予算ms>` を明示すると、MB86H47の起動だけを実行する。
この操作はチップ内のコードとクロック状態を変えるため、同じ個体を利用中の
BonDriverCoax/coaxを終了し、録画などで使用していないことの確認が必要。
初期化モードは列挙が完全に成功し、対象interfaceがちょうど1台の場合だけ動作する。
予算は診断用に呼出し元が指定する値であり、製品の既定タイムアウトではない。
初期化の失敗状態によっては再挿入が必要になる可能性があり、失敗を繰り返して復旧扱いにする
自動再試行は含まれていない。成功してもDiB8096初期化・選局・TS受信は未確認のままである。

生成器の再現手順と由来は `tools/torne_codegen/README.md` を参照。
全ての起動転送位置へ失敗を注入した1,129ケース、冷間状態不一致、未知handler、
SRAM検証失敗、同一bank再利用・反対bank移行、core停止、wait失敗、
I2C連番周回、NACK、古い応答、期限超過、旧generation拒否を模擬環境で確認した。
この回数はC++実装の要求列に対する試験数で、実機での成功回数ではない。

実機では1台のPnP情報・USB descriptor、MB86H47のC++起動2回、core1生存確認、
復調器reset・I2C列挙・識別を確認した。起動所要時間は62msと58ms。
WinUSB、上記interface GUID、serial descriptor index=0を確認している。
復調器IDはvendor=0x01B3、device=0x8002。詳細は `cech-zd1j-validation.md` を参照。
追加診断では全初期化・8チャンネルのロックとTS受信、13の60秒連続受信、
既存StreamBuffer経由の選局試験まで確認した。helperのGPIO 1操作で、利用者が
使用中の赤ランプも目視確認した。詳細と失敗履歴は検証記録に残す。
復旧、TVTest/EDCB、複数台、PX-MLT5PEとの同時受信・回帰は未確認。
x86ビルドと配布パッケージも今回の検証対象外。
