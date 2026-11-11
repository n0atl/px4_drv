// px4_signal_stats.h

// BonDriver_PX4.dll が公開する信号品質取得関数の定義。
// BonDriver の GetSignalLevel() は値を 1 つしか返せないため、C/N を表示したまま
// BER と PER を読み出したい TVTest プラグインなどの利用者向けに別の入口を設ける。
// 利用者は GetModuleHandle() で読み込み済みの BonDriver を取得し、
// GetProcAddress() で PX4_SIGNAL_STATS_PROC_NAME を探す。

#pragma once

#include <stdint.h>
#include <windows.h>

#define PX4_SIGNAL_STATS_PROC_NAME	"PX4GetSignalStats"

// Px4SignalStats::valid のビット
enum {
	PX4_SIGNAL_STATS_VALID_CNR	= 0x00000001,
	PX4_SIGNAL_STATS_VALID_BER_A	= 0x00000002,
	PX4_SIGNAL_STATS_VALID_BER_B	= 0x00000004,
	PX4_SIGNAL_STATS_VALID_BER_C	= 0x00000008,
	PX4_SIGNAL_STATS_VALID_PER_A	= 0x00000010,
	PX4_SIGNAL_STATS_VALID_PER_B	= 0x00000020,
	PX4_SIGNAL_STATS_VALID_PER_C	= 0x00000040,
	PX4_SIGNAL_STATS_VALID_VBER	= 0x00000080,
};

// Px4SignalStats::system の値
enum {
	PX4_SIGNAL_SYSTEM_UNKNOWN	= 0,
	PX4_SIGNAL_SYSTEM_ISDB_T	= 1,
	PX4_SIGNAL_SYSTEM_ISDB_S	= 2,
	PX4_SIGNAL_SYSTEM_ISDB_C	= 3,
};

struct Px4SignalStats {
	// 呼び出し側が sizeof(Px4SignalStats) を設定する。将来の項目追加時に、
	// 古い呼び出し側の構造体より後ろへ書き込まないための判定に使う。
	uint32_t size;
	uint32_t valid;		// PX4_SIGNAL_STATS_VALID_* の組み合わせ
	uint32_t system;	// PX4_SIGNAL_SYSTEM_*
	int32_t cnr;		// C/N [dB] x 1000
	// ISDB-T は [0]-[2] が A/B/C 階層、ISDB-S は [0] が高階層、[1] が低階層。
	int32_t ber[3];		// RS 復号前の BER x 1e7
	int32_t per[3];		// RS 復号後のパケット誤り率 x 1e6
	// ビタビ復号前の BER x 1e7。ISDB-T のみで、階層の区別はない。
	// 公開資料にないモニタを使う参考値で、換算の単位は実機と資料で確定できていない。
	int32_t vber;
};

// vber を追加する前の構造体の大きさ。この大きさの呼び出し側には vber を書き込まない。
#define PX4_SIGNAL_STATS_MIN_SIZE	(sizeof(uint32_t) * 3 + sizeof(int32_t) * 7)

// チューナーを開いていない場合や、引数が不正な場合は FALSE を返す。
// 個別の値を取得できない場合は TRUE を返し、valid の該当ビットを 0 にする。
// DriverHost_PX4 との名前付きパイプ通信を行うため、UI スレッドからの呼び出しは避ける。
typedef BOOL (WINAPI *Px4GetSignalStatsFunc)(Px4SignalStats *stats);
