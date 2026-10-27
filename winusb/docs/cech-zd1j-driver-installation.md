# CECH-ZD1Jの共通WinUSBパッケージ

`pkg/inf/CECH-ZD1J.inf`を既存の`px4_drv_winusb.cat`へ追加しています。
クラスはPLEX用INFと同じMEDIA、表示名は
`Sony torne CECH-ZD1J ISDB-T Receiver Device (WinUSB)`です。
VID/PIDとインターフェースGUIDはcoax用と同じです。
元INFはCopyright 2026 Till0196、Apache-2.0。元の著作権とSPDX表示を維持します。
ローカル変更は分類・表示名・版番号・共通カタログ参照です。

## 証明書

元から付属する`px4_drv CA`の証明書と署名鍵を再利用します。新しい証明書は生成しません。
証明書ファイルと既存機種INFは変更せず、torneを収録するカタログを再生成・署名します。
署名者の拇印は`7B45DBD534A25B779AD01DBC1D0C73B0E53995F7`です。
利用者の氏名・メール・PC名を証明書へ追加する処理はありません。

## 導入の手順

使用アプリを終了してから、通常の配布と同じ`cert-install.jse`で付属証明書を登録し、
`CECH-ZD1J.inf`を用いてtorneへドライバーを適用します。
例：管理者のターミナルで、配布Driverフォルダーを指定して
`pnputil /add-driver CECH-ZD1J.inf /install`を実行します。
PnPUtilが成功しても別の既存ドライバーが選択される場合があります。
デバイスマネージャーのクラス・表示名・INF名を確認し、結果に応じて再接続します。
現時点では準備とカタログ検査だけを実施し、証明書登録・インストールは実行していません。
MEDIA分類への変更後の受信・カード通信は未確認です。

## 検証済みの範囲

`signing-tools/sign.ps1`でCRLF/BOMなし正規化、Inf2Cat、既存鍵での署名、
`verify.ps1`による署名者と全17 INFの収録ハッシュ照合に成功しました。
既存機種INF・証明書・証明書インストーラーのバイト列が従来と一致することも確認しました。
新しいCATと新しいINFは必ず組で配布します。元のCATと混ぜたり、署名後にINFを編集したりしません。
