# Linux の ISDB-C（J.83 Annex C）実験対応

この対応は、CXD2856ER/CXD2858ER を使う以下の PXMLT 系だけを対象にします。

- PX-MLT5PE / DTV02A-5TS-P
- PX-MLT8PE（筐体内の 3 チューナー側・5 チューナー側）
- DTV02A-4TS-P（コード上の ISDB6014 4TS）

通常の ISDB-T/S 利用へ影響させないため、ISDB-C 能力は既定で公開しません。モジュールを読み込む際に明示的に有効化します。

```sh
sudo modprobe px4_drv experimental_isdbc=1
```

`PTX_SET_SYSTEM_MODE` に `PTX_ISDB_C_SYSTEM` (`0x40`) を指定してから、従来の `PTX_SET_CHANNEL` を呼びます。`freq_no` は物理 C チャンネル番号（C13 は 3、C23 は 22）または UHF の物理番号（13ch は 63）です。`slot` は ISDB-C では周波数補正値（符号付き kHz）です。

例えば C13 の中心周波数を -200 kHz 補正する場合は、`freq_no=3`、`slot=-200` です。UHF 21ch の 520.750 MHz は、`freq_no=71`、`slot=-250` で指定できます。ISDB-T と異なり、ISDB-C の周波数計算には 143 kHz の地上波オフセットを加えません。

復調条件は Windows 版と同じ、64QAM・5274 ksym/s・帯域幅 6 MHz に固定です。チューナー/復調器の初期化列は PX-MLT5PE での実測を基にした実験用プロファイルであり、他の対象機種での実機確認は未実施です。

## TSMF

この時点の Linux カーネル側は通常の ISDB-C TS を出力します。Windows の DriverHost にある「TSMF ヘッダを解析し Relative stream ごとに 52 スロットから抽出する」処理は、既存 Linux 文字デバイス API に TSMF のストリーム番号を渡す契約がないため、まだ移植していません。

TSMF を使う場合は `discard_null_packets=0` を維持し、現段階ではユーザー空間で TSMF 分離を行ってください。null パケットを IT930x 側で除去すると 52 スロットの位置関係が崩れ、TSMF の分離が不可能になります。

Linux 用の TSMF API を追加する場合は、チャンネル選局 ioctl と互換な形で Relative stream 番号を指定する新しい ioctl を設計し、切替時の TS バッファ破棄・複数チューナー・長時間の continuity counter を実機で検証してから追加します。
