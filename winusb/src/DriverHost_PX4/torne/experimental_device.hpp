// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#include "../device_base.hpp"
#include <atomic>
#include <memory>
#include <mutex>

namespace px4::torne {
struct ExperimentalSession;
// CECHZD1J専用の機器実装。PX系の制御経路とは共有しない。
class ExperimentalDevice final : public px4::DeviceBase {
public:
	ExperimentalDevice(const std::wstring &path, const px4::DeviceDefinition &definition,
		std::uintptr_t index, px4::ReceiverManager &manager);
	~ExperimentalDevice() override;
	int Init() override;
	void Term() override;
	void SetAvailability(bool available) override;
	px4::ReceiverBase *GetReceiver(int id) const override;
	bool HasCardReader() const noexcept override { return true; }
	int OpenCard() override;
	void CloseCard() override;
	int DetectCard(bool &detected) override;
	int ResetCard() override;
	int SetCardBaudrate(::it930x_uart_baudrate baudrate) override;
	int IsCardDataReady(bool &ready) override;
	int ReadCardData(std::uint8_t *buf, std::uint8_t &len) override;
	int WriteCardData(const std::uint8_t *buf, std::uint8_t len) override;
private:
	struct CardState;
	std::unique_ptr<CardState> card_;
	std::mutex card_mutex_;
	class Receiver;
	std::shared_ptr<ExperimentalSession> AcquireSession();
	std::mutex session_mutex_;
	std::weak_ptr<ExperimentalSession> shared_session_;
    // ホスト存続中だけ個体の較正状態を保持し、抜去・終了時に解放する。
    std::shared_ptr<ExperimentalSession> retained_session_;
	std::unique_ptr<Receiver> receiver_;
	std::atomic_bool available_{true};
	HANDLE cancel_ = nullptr;
	std::string id_;
	bool registered_ = false;
	// 信号が無いと判断したチャンネルの再探索を省くか (SkipEmptyChannels)。
	bool skip_empty_ = false;
	// TS用bulk INは複数の転送を常に投入し、受信スレッドの遅延中もbridgeのFIFOを空ける。
	unsigned int usb_transfer_size_ = 6144, usb_transfer_count_ = 16;
};
} // namespace px4::torne
