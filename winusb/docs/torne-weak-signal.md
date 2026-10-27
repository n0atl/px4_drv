# torneのログと低ビットレート受信

対象はWindows WinUSB版のCECH-ZD1J。BonDriverの配布名は`BonDriver_Torne.dll`と
`BonDriver_Torne.ini`、ホストは`DriverHost_PX4.exe`と`DriverHost_PX4.ini`。
以前の`BonDriver_CECH-ZD1J.dll`を使う設定から切り替える場合、TVTest/EDCB側でもDLLを選び直す。

## ログ

`BonDriver_Torne.ini`の`[BonDriver]`に`DebugLog=false`を指定すると、BonDriverのログファイルを
作成しない。診断時は`true`で同名の`.log`を作成できる。設定省略時の既存PX系は従来の出力を維持。
ホストのログは別設定で、`[DriverHost_PX4]`の`DebugLog`に従う。
過去に作成済みのログファイルの削除は利用者の判断に任せる。

## TSが少ない局での待ち時間

2026-10-27の2台試験では、弱い13・14chのTSはPCRから約0.416Mbps、正常な41chは約17.27Mbps。
弱い局ではワンセグ相当のH.264/AACを持つPMT（PID 0x1FC8）を確認した。
正常局のMPEG-2映像PIDはこの弱い局の受信TSには見つからなかった。

旧設定の1024パケット単位では、選局自体が終わっても固定長バッファの充填とPurge後の破棄で
さらに約5秒待つ場合があり、6秒の計測区間にTSが届かない例もあった。
同じバイナリーで128パケット単位へ変えると、両個体で選局後約689～805msでTSが届いた。
これはチューナー初期化・選局時間を含まない。プローブは選局直後にPurgeしてから計測している。

以下は低ビットレート時の待ちを短縮した過去の試験設定。現在の配布既定値とは異なる。
PX-MLTなどの既存配布設定には変更を加えていない。

```ini
[BonDriver]
NumberOfPacketsPerBuffer=128
MaximumNumberOfBuffers=512
MinimumNumberOfBuffers=32
DebugLog=false
```

## 未訂正パケットも出す場合

`DriverHost_PX4.ini`のtorne定義の設定を変更する。

```ini
[DeviceDefinition15.Config]
DeviceId="auto"
OutputErrorPackets=true
```

この設定は`Type="CECHZD1J"`のバックエンドだけが読む。PX-MLTなどの別機種は対象外。
標準の自動個体選択構成では、この定義に該当する接続中のtorne全台へ適用される。
反映には利用アプリを閉じてホストが終了した後、起動し直す必要がある。
旧バイナリーにはこの設定の実装がないため、対応する新版のホストが必要。

現在の配布INIは`true`。項目省略時の実装既定値は`false`で、復調器の従来のエラー破棄を維持する。`true`ではレジスター299のSMO
error-discardを解除し、読戻しを確認する。入力データを復元したり、ホストでnullパケットを
生成したりする処理ではない。同期取得はこのモードに限りTEIも許容し、誤同期を抑えるため
8個の192byte周期と有効AFCを確認する。通常モードの3個・TEI除外条件は従来どおり。

実機の13・14chでは、この設定により2台とも約17.268Mbpsで出力し、選局後7～18msで最初の
BonDriverバッファを受信した。ただし6秒の区間で約68,736～68,864パケット中、
67,080～67,204がTEI付きかつTS形式不正だった。良好な受信とは評価できず、TVTestでの
Eカウント表示、映像の復号・再生は利用者による確認が必要。正常41chは同じ設定でも
同期・TEI・形式・CC異常0を確認した。

破損パケットを含むプローブ結果はFAILのまま記録している。従来のプローブは形式検査後にTEIを
数えるため、形式不正を伴うTEIはその欄に含まれない。上記TEI数は保存TSのヘッダーを別途解析した値。

最終の通常モードではx86/x64とも2台で41・42・47・49chを受信し、17・21・26chの無信号判定と
正常局への復帰を確認した。`channel_switch_stress`は各ビット数で2台を同時に4周期ずつ実行し、
各クライアント92,032パケット、選局失敗・空受信・同期・TEI・CC異常は0。
x86ではカードAPDUを35秒間重ね、105回成功・失敗0、最大応答2,637ms。
未訂正出力モードはx86ホストで2台を比較した結果で、x64での同モードの実機確認は対象外。
TVTestの映像・E表示と長時間運用は、この新版では別途確認が必要。

参考: [Linux dib8000.cの出力設定とSMO既定値](https://code.googlesource.com/linux/torvalds/linux/+/95940a04bfe8a4d246f4ca17c6a3b00148bdead0/drivers/media/dvb-frontends/dib8000.c)。
レジスター切替の効果と既存動作への復帰はCECH-ZD1J実機の保存TSで比較した。

## 現在の配布既定値（2026-10-28）

`BonDriver_Torne.ini`は1024パケット単位、最大64・最小4バッファ。
`DriverHost_PX4.ini`のtorne専用設定は`OutputErrorPackets=true`。
この組み合わせでの実機再試験は未実施であり、上記の過去の測定結果をそのまま適用できない。
未訂正TSの出力は受信品質を改善する処理ではなく、アプリケーションに渡すパケットの扱いを変える。
