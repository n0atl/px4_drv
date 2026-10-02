# px4_drv ISDB-C Edition

PX-MLT 系チューナーに、CATV のトランスモジュレーション方式（ISDB-C / ITU-T J.83 Annex C、以下「トラモジ」）の受信機能を追加したものです。

ベースは [tsukumijima/px4_drv](https://github.com/tsukumijima/px4_drv) です。本リポジトリは PLEX / e-Better の公式ドライバではなく、公式ドライバと併用するものでもありません。

> [!WARNING]
> この版の ISDB-C 復調初期化は、公開されているCXD2856ER向けLinuxドライバを参考にした実験的な実装です。PX-MLT5PEでの受信を確認していますが、すべてのCATV局・周波数・機種での動作を保証しません。録画運用の前に、対象チャンネルで十分な視聴・録画試験を行ってください。

## 対応範囲

| 機種 | チップ構成 | ISDB-C の状況 |
| --- | --- | --- |
| PX-MLT5PE | CXD2856ER / CXD2858ER ×5 | 実機で受信確認済み |
| DTV02A-5TS-P | PX-MLT5PE互換 ×5 | ソース上で対応、実機未確認 |
| PX-MLT8PE | 同一筐体内に3チューナー側と5チューナー側 | ソース上で対応、実機未確認 |
| DTV02A-4TS-P | CXD2856ER / CXD2858ER ×4 | 実機で受信確認済み |

上記以外の PX4 / PX5 / ISDB2056 系は、このトラモジ機能の対象外です。

## 実装している受信条件

- 変調方式: 64QAM
- シンボルレート: 5274 ksym/s
- 帯域幅: 6 MHz
- CATV C13～C63 と、UHF帯のトラモジ再送信
- TSMF: Relative stream 番号を指定したTSMFストリームの抽出

これらの復調条件は現時点では固定です。ChSetで64QAM以外や別のシンボルレートを指定しても、復調器設定は変わりません。

## Windows (WinUSB)

Windows版は、`BonDriver_PX-MLT.dll` ひとつで地デジ・BS/CS・トラモジを切り替えられます。同じDLLを複数のTVTestやEDCBプロセスから開いた場合、空いているチューナーが割り当てられます。

例: PX-MLT5PEでは「地デジ3番組 + トラモジ2番組」、PX-MLT8PEでは合計8番組の同時利用が可能です。実際に同時受信できる番組数は、アンテナ配線、USB帯域、PC性能、信号品質にも依存します。

### 配置

ビルド後に生成される `BonDriver_PX-MLT_64bit` または `BonDriver_PX-MLT_32bit` フォルダの内容を、TVTest / EDCB が使用するBonDriverフォルダへ**すべて**コピーしてください。

最低限、次のファイルは同じフォルダに必要です。

```text
BonDriver_PX-MLT.dll
BonDriver_PX-MLT.ini
BonDriver_PX4-C.ChSet.txt
BonDriver_PX4-S.ChSet.txt
BonDriver_PX4-T.ChSet.txt
DriverHost_PX4.exe
DriverHost_PX4.ini
it930x-firmware.bin
```

スクランブル解除に内蔵カードリーダーを使う場合は、同梱の `WinSCard.dll` をTVTestやEDCBの実行ファイルと同じフォルダへ配置してください。DLLのビット数はアプリケーションに合わせます。

WinUSBドライバのインストール方法、自己署名証明書、B-CASカードリーダーの詳細は、ベースプロジェクトの [README](https://github.com/tsukumijima/px4_drv/blob/master/README.md) を参照してください。

### トラモジChSet

トラモジのチャンネル一覧は `BonDriver_PX4-C.ChSet.txt` です。TSMFを処理する場合は、各チャンネルの末尾にTSMF用の設定値を追加します。

```text
通常：
C57Ch	0	43	56	0	0	-550
TSMFを処理する場合：
C57Ch	0	43	56	0	0	-550	0
```

各行の項目は次のとおりです。

```text
名前 / 空間番号 / チャンネル番号 / PX-MLT物理チャンネル番号 / TSID /
任意のRF中心周波数[kHz] / 任意の周波数補正[kHz] / 任意のTSMF Relative stream番号
```

- CATV Cチャンネルでは、6番目を `0` にすると物理チャンネル番号から中心周波数を計算します。
- UHF帯はCチャンネルと番号体系が異なるため、6番目に中心周波数を指定します。
- 7番目は受信周波数の補正値です。放送側の設定ではありません。
- 8番目を指定した行はTSMFとして扱います。`0` はRelative stream 0を意味します。

周波数補正は、映る範囲の中央を目安に25 kHz刻みで設定してください。たとえば `-550` が中心付近なら、ロックしないときは `-525`、`-575` のように調整します。補正値は受信環境（CATV局、宅内配線、機器個体差）ごとに異なります。

### nullパケット除去

PID 0x1FFF のnullパケットを除去します。TSMFにも対応しています。

```ini
DiscardNullPackets=true
```

`false` にすると、TSMF・通常チャンネルのどちらでもnullパケットをそのまま出力します。

### TSMFヘッダ PID `0x002F` の除去

`0x002F` はTSMFの多重フレームヘッダです。`DiscardTsmfHeaderPackets=true` にすると、ISDB-C出力だけからこのPIDを除去します。

```ini
DiscardTsmfHeaderPackets=true
```

TSMFチャンネルではヘッダを解析して52スロットの対象TSを抽出した後、非TSMFチャンネルでは出力直前に除去します。地デジ・BS/CSには適用されません。入力側では常に保持されるため、TSMFの処理時は問題ありません。

### ログ

`DriverHost_PX4.ini` の以下でログを制御できます。

```ini
[DriverHost_PX4]
DebugLog=true
DebugLogPath="DriverHost_PX4.log"
```

`DebugLog=false` にするとログを作成しません。ログには選局した方式・周波数・復調ロック状態が出力されます。

## Linux

Linux版にも実験的なISDB-C選局経路を追加しています。対象はPX-MLT5PE / DTV02A-5TS-P、PX-MLT8PE、DTV02A-4TS-Pです。

既存のISDB-T/S利用に影響させないため、ISDB-Cは既定で無効です。モジュールをロードするときだけ明示的に有効化します。

```sh
sudo modprobe px4_drv experimental_isdbc=1
```

Linuxでは `PTX_SET_SYSTEM_MODE` へ `PTX_ISDB_C_SYSTEM` (`0x40`) を指定してから選局します。`PTX_SET_CHANNEL` の `freq_no` は物理チャンネル番号、`slot` は符号付きの周波数補正値（kHz）です。

```text
C13: freq_no=3,  slot=-200
UHF 21ch (520.750 MHz): freq_no=71, slot=-250
```

> [!IMPORTANT]
> Linux版には、Windows版のTSMF Relative stream抽出はまだ移植されていません。通常のISDB-C TS受信だけが対象です。TSMFをLinuxで利用するには、ユーザー空間でのTSMF分離、または専用ioctlを含むカーネル側APIの設計・実機試験が必要です。

より詳しいLinux側の使い方は [docs/isdbc_linux_experimental.md](docs/isdbc_linux_experimental.md) を参照してください。

## ビルド

### Windows

Visual Studio 2022 / 2026で、次のコンポーネントをインストールしてから実行します。

- **C++によるデスクトップ開発** ワークロード
- MSBuild
- Windows 10 または Windows 11 SDK
- MSVC v143 C++ ビルドツール

> [!IMPORTANT]
> `build.ps1` は右クリックの「PowerShellで実行」でも起動できますが、失敗時にウィンドウが閉じて原因を読めません。下記のPowerShellから実行する方法を推奨します。

リポジトリの一番上のフォルダでPowerShellを開き、次の**2行を順番に**実行します。

```powershell
Set-Location .\winusb
.\build.ps1
```

1行で実行する場合は、コマンドを `;` で区切ります。

```powershell
Set-Location .\winusb; .\build.ps1
```

実行ポリシーで止まる場合は、リポジトリの一番上のフォルダから次を実行します。

```powershell
powershell -ExecutionPolicy Bypass -File .\winusb\build.ps1
```

ダブルクリックで実行したい場合は、`winusb\build.cmd` を使ってください。成功・失敗にかかわらず最後に画面を停止するため、エラーメッセージを確認できます。

`Set-Location .\winusb .\build.ps1` のように、2つのコマンドを空白だけで続けて書くことはできません。`Set-Location` が `build.ps1` を余分な引数として扱うため失敗します。

`winusb/dist/BonDriver_PX-MLT_64bit` と `winusb/dist/BonDriver_PX-MLT_32bit` に配布用ファイルが生成されます。

### Linux

対象カーネルのヘッダー、gcc、makeを用意してから実行します。

```sh
cd driver
make
```

Linux版のISDB-Cは実機未検証です。モジュールのビルド成功は、選局や長時間録画の成功を意味しません。


## 謝辞・ライセンス

ベース実装は [nns779/px4_drv](https://github.com/nns779/px4_drv) および [tsukumijima/px4_drv](https://github.com/tsukumijima/px4_drv) に基づきます。CXD2856ERのISDB-C初期化は、LinuxメインラインのSony復調器ドライバを比較資料として利用しています。

ライセンスは [GPL-2.0-only](LICENSE) です。
