// SPDX-License-Identifier: GPL-2.0-only
// Default: read-only descriptor probe, without vendor requests or resets.
// --initialize: explicitly requested MB86H47 bootstrap diagnostic; no tuning/TS.
#include <windows.h>
#include <setupapi.h>
#include <winusb.h>
#include <usb.h>

#include <cstdint>
#include <iostream>
#include <string>
#include <sstream>
#include <cstring>
#include <vector>

#include "../src/DriverHost_PX4/torne/winusb_control.hpp"
#include "../src/DriverHost_PX4/torne/dib8096_registers.hpp"
#include "torne_reference_client.hpp"
#include "torne_capture.hpp"
#include "torne_receiver_adapter.hpp"
#include "../src/DriverHost_PX4/torne/device_identity.hpp"
#include "../src/DriverHost_PX4/torne/mb86h47_card.hpp"
#include "../src/WinSCard_PX4/bcas_atr.hpp"

#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "winusb.lib")

static constexpr GUID torne_interface = {0xb8f4b677, 0xe19a, 0x4beb, {0xa6, 0x0c, 0xc0, 0xa7, 0xac, 0x2d, 0x0f, 0x6b}};

static bool Inspect(const wchar_t *path, unsigned initialization_timeout, bool identify, bool frontend, bool receive, const std::vector<unsigned> &channels, unsigned duration_ms, bool receiver_mode, bool receiver_fault, bool card_status, bool card_atr)
{
	std::wcout << L"Interface: " << path << L'\n';
	std::cout << "Interface ID: " << px4::torne::InterfaceId(path) << '\n';
	HANDLE file = CreateFileW(path, GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, nullptr);
	if (file == INVALID_HANDLE_VALUE) {
		std::cerr << "CreateFile failed: " << GetLastError() << '\n';
		return false;
	}
	WINUSB_INTERFACE_HANDLE usb = nullptr;
	if (!WinUsb_Initialize(file, &usb)) {
		std::cerr << "WinUsb_Initialize failed: " << GetLastError() << '\n';
		CloseHandle(file);
		return false;
	}
	USB_DEVICE_DESCRIPTOR descriptor{};
	ULONG length = 0;
	bool ok = WinUsb_GetDescriptor(usb, USB_DEVICE_DESCRIPTOR_TYPE, 0, 0,
		reinterpret_cast<PUCHAR>(&descriptor), sizeof(descriptor), &length) &&
		length == sizeof(descriptor) && descriptor.bLength == sizeof(descriptor) &&
		descriptor.bDescriptorType == USB_DEVICE_DESCRIPTOR_TYPE &&
		descriptor.idVendor == 0x054c && descriptor.idProduct == 0x04b2;
	if (!ok) {
		std::cerr << "Device descriptor mismatch or read failure: " << GetLastError() << '\n';
	} else {
		std::cout << "VID=054C PID=04B2 serial_descriptor_index=" << unsigned(descriptor.iSerialNumber) << '\n';
		UCHAR alternate = 0xff;
		USB_INTERFACE_DESCRIPTOR interface_descriptor{};
		ok = WinUsb_GetCurrentAlternateSetting(usb, &alternate) &&
			WinUsb_QueryInterfaceSettings(usb, alternate, &interface_descriptor);
		if (ok) {
			std::cout << "interface=" << unsigned(interface_descriptor.bInterfaceNumber)
				<< " alternate=" << unsigned(alternate) << " endpoints=" << unsigned(interface_descriptor.bNumEndpoints) << '\n';
			bool stream = false, command = false, answer = false;
			for (UCHAR i = 0; i < interface_descriptor.bNumEndpoints; ++i) {
				WINUSB_PIPE_INFORMATION pipe{};
				if (!WinUsb_QueryPipe(usb, alternate, i, &pipe)) {
					ok = false;
					std::cerr << "QueryPipe failed: " << GetLastError() << '\n';
					break;
				}
				std::cout << "endpoint=0x" << std::hex << unsigned(pipe.PipeId) << std::dec
					<< " type=" << unsigned(pipe.PipeType) << " max_packet=" << pipe.MaximumPacketSize << '\n';
				if (pipe.PipeType == UsbdPipeTypeBulk) {
					stream |= pipe.PipeId == 0x81;
					command |= pipe.PipeId == 0x02;
				}
				// 応答EPは実機descriptorではinterrupt。ReadPipeは両形式を扱える。
				answer |= pipe.PipeId == 0x83 && pipe.PipeType == UsbdPipeTypeInterrupt;
			}
			ok = ok && interface_descriptor.bInterfaceNumber == 0 && alternate == 0 && stream && command && answer;
		} else {
			std::cerr << "Interface descriptor read failed: " << GetLastError() << '\n';
		}
	}
	std::cout << (ok ? "DESCRIPTORS MATCH (tuning and TS untested)\n" : "DESCRIPTORS FAILED\n");
	if (ok && initialization_timeout) {
		HANDLE cancel = CreateEventW(nullptr, TRUE, FALSE, nullptr);
		if (!cancel) {
			ok = false;
		} else {
			px4::torne::NativeControlIo native;
			px4::torne::WinUsbControl control(file, usb, cancel, native);
			const auto deadline = px4::torne::WinUsbControl::Clock::now() + std::chrono::milliseconds(initialization_timeout);
			control.Begin(deadline);
			px4::torne::MemoryAccess memory(control);
			px4::torne::Bootstrap bootstrap(memory, control);
			const auto begin = px4::torne::WinUsbControl::Clock::now();
			const auto result = bootstrap.Initialize(GetTickCount() | 1u);
			const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(px4::torne::WinUsbControl::Clock::now() - begin).count();
			ok = result == px4::torne::BootstrapResult::ok;
			std::cout << "BOOTSTRAP result=" << static_cast<unsigned>(result)
				<< " memory_result=" << static_cast<unsigned>(bootstrap.LastMemoryResult())
				<< " os_error=" << bootstrap.LastSystemError() << " address=0x" << std::hex
				<< bootstrap.LastAddress() << std::dec << " elapsed_ms=" << elapsed << '\n';
			std::cout << (ok ? "BOOTSTRAP PASSED (demod initialization, tuning and TS untested)\n" : "BOOTSTRAP FAILED\n");
			if (ok && card_status) {
				px4::torne::CardUart card(memory, control);
				bool present = false;
				ok = card.Present(present);
				std::cout << "CARD STATUS id=" << px4::torne::InterfaceId(path)
					<< " ok=" << ok << " present=" << present
					<< " memory_result=" << static_cast<unsigned>(card.Result()) << '\n';
				if (ok && card_atr && present) {
					std::vector<std::uint8_t> atr;
					ok = card.Reset();
					const auto atr_deadline = (std::min)(deadline, px4::torne::WinUsbControl::Clock::now() + std::chrono::seconds(1));
					bool known_bcas = false;
					while (ok && !known_bcas && px4::torne::WinUsbControl::Clock::now() < atr_deadline) {
						std::array<std::uint8_t, 33> bytes{};
						std::size_t count = 0;
						ok = card.ReadBytes(bytes.data(), bytes.size(), count);
						if (count) atr.insert(atr.end(), bytes.begin(), bytes.begin() + count);
						if (atr.size() > 33) { ok = false; break; }
						known_bcas = px4::winscard::IsBcasAtr(atr.data(), atr.size());
						if (ok && !known_bcas) ok = control.Pause(5) == 0;
					}
					control.Begin(px4::torne::WinUsbControl::Clock::now() + std::chrono::seconds(2));
					const bool powered_down = card.PowerDown();
					ok = ok && known_bcas && powered_down;
					std::cout << "CARD ATR bytes=" << atr.size() << " known_bcas=" << known_bcas
						<< " powered_down=" << powered_down << " ok=" << ok << '\n';
				}
			}
			if (ok && identify) {
				const auto reset = bootstrap.ResetDemodulator();
				std::cout << "DEMOD RESET result=" << static_cast<unsigned>(reset) << " os_error=" << bootstrap.LastSystemError() << '\n';
				ok = reset == px4::torne::BootstrapResult::ok;
				px4::torne::MailboxI2c bus(memory, control);
				ok = ok && bus.BeginAfterBootstrap(bootstrap);
				px4::torne::DiB8096Registers registers(bus);
				if (ok) {
					const auto enumerated = registers.Write(1287, 3, deadline);
					std::cout << "DEMOD ENUMERATE result=" << static_cast<unsigned>(enumerated) << " os_error=" << bus.LastSystemError() << '\n';
					ok = enumerated == px4::torne::I2cResult::ok;
				}
				if (ok) {
					std::uint16_t vendor = 0, device = 0;
					bool recognized = false;
					const auto identified = registers.Identify(vendor, device, recognized, deadline);
					std::cout << "DEMOD IDENTIFY result=" << static_cast<unsigned>(identified) << " vendor=0x" << std::hex
						<< vendor << " device=0x" << device << std::dec << " recognized=" << recognized
						<< " os_error=" << bus.LastSystemError() << '\n';
					ok = identified == px4::torne::I2cResult::ok && recognized;
				}
				std::cout << (ok ? "DEMOD IDENTIFICATION PASSED (full demod initialization, tuning and TS untested)\n" : "DEMOD IDENTIFICATION FAILED\n");
				if (ok && frontend) {
					wchar_t executable[32768]{};
					const DWORD executable_length = GetModuleFileNameW(nullptr, executable, 32768);
					std::wstring helper(executable);
					helper = helper.substr(0, helper.find_last_of(L"\\/") + 1) + L"TorneFrontendDiagnostic.exe";
					TorneReferenceClient reference(bus, cancel);
					ok = executable_length && executable_length < 32768 && reference.Start(helper);
					std::vector<std::uint8_t> response;
					const auto init_begin = px4::torne::WinUsbControl::Clock::now();
					ok = ok && reference.Command(0x10, {}, response, deadline) && response.empty();
					std::cout << "FRONTEND INITIALIZE ok=" << ok << " elapsed_ms="
						<< std::chrono::duration_cast<std::chrono::milliseconds>(px4::torne::WinUsbControl::Clock::now() - init_begin).count()
						<< " i2c_requests=" << reference.I2cRequests() << " error=" << reference.Error() << '\n';
					// 選局の比較条件をcoaxボードと同じparallel gatedへ揃える。
					if (ok) ok = reference.Command(0x12, {1}, response, deadline);
					bool all_locked = true;
					if (ok && receiver_mode) ok = TestTorneReceiver(file, usb, memory, bootstrap, reference, deadline, channels, duration_ms ? duration_ms : 2000, receiver_fault);
					for (const auto channel : (receiver_mode ? std::vector<unsigned>{} : channels)) {
						if (!ok) break;
						// coaxのkHz正規化に合わせた日本UHF 6MHz・1/7MHzオフセット。
						const std::uint32_t frequency = 473143000u + (channel - 13u) * 6000000u;
						std::vector<std::uint8_t> tune(4);
						for (unsigned byte = 0; byte < 4; ++byte) tune[byte] = static_cast<std::uint8_t>(frequency >> (byte * 8));
						const auto tune_begin = px4::torne::WinUsbControl::Clock::now();
						ok = reference.Command(0x11, tune, response, deadline);
						if (ok) ok = reference.Command(0x13, {}, response, deadline) && response.size() == 24;
						bool locked = ok && response[0] == 1;
						const auto lock_deadline = (std::min)(deadline, tune_begin + std::chrono::milliseconds(5000));
						// 参照Tuneの終了はLOCKを保証しない。操作開始から5秒以内で状態を観測する。
						while (ok && !locked && px4::torne::WinUsbControl::Clock::now() < lock_deadline) {
							ok = control.Pause(20) == ERROR_SUCCESS;
							if (!ok || px4::torne::WinUsbControl::Clock::now() >= lock_deadline) break;
							ok = reference.Command(0x13, {}, response, deadline) && response.size() == 24;
							locked = ok && response[0] == 1;
						}
						all_locked = all_locked && locked;
						std::uint16_t lock_register = 0;
						const auto lock_read = registers.Read(568, lock_register, deadline);
						ok = ok && lock_read == px4::torne::I2cResult::ok;
						double cnr = 0;
						if (ok) std::memcpy(&cnr, response.data() + 8, sizeof(cnr));
						std::cout << "TUNE channel=" << channel << " frequency_hz=" << frequency << " ok=" << ok << " locked=" << locked
							<< " raw_lock=0x" << std::hex << lock_register << std::dec
							<< " cnr_db=" << cnr << " elapsed_ms="
							<< std::chrono::duration_cast<std::chrono::milliseconds>(px4::torne::WinUsbControl::Clock::now() - tune_begin).count()
							<< " i2c_requests=" << reference.I2cRequests() << " error=" << reference.Error() << '\n';
						if (ok && locked && receive) {
							px4::torne::StreamEngine engine(memory, bootstrap);
							TorneCaptureStats capture;
							const bool captured = CaptureFirstTs(file, usb, cancel, engine, deadline, capture, duration_ms);
							std::cout << "FIRST TS channel=" << channel << " ok=" << captured << " raw_bytes=" << capture.raw_bytes
								<< " packets=" << capture.packets << " tei=" << capture.tei << " discarded_bytes=" << capture.discarded_bytes
								<< " elapsed_ms=" << capture.elapsed_ms << " max_read_gap_ms=" << capture.max_read_gap_ms
								<< " cc_anomalies=" << capture.cc_anomalies << " malformed=" << capture.malformed
								<< " reads=" << capture.reads << " first_ts_ms=" << capture.first_ts_ms << " io_error=" << capture.error
								<< " prepare=" << static_cast<unsigned>(capture.prepare) << " start=" << static_cast<unsigned>(capture.start)
								<< " stop=" << static_cast<unsigned>(capture.stop) << " address=0x" << std::hex << engine.LastAddress() << std::dec << '\n';
							ok = captured;
						}
					}
					// 成功経路でも出力をHigh-Zに戻してから子プロセスを回収する。
					if (ok) {
						ok = reference.Command(0x14, {}, response, deadline);
						std::cout << "FRONTEND SLEEP ok=" << ok << " error=" << reference.Error() << '\n';
					}
					ok = ok && all_locked;
					std::cout << (ok ? (receive ? "REFERENCE CAPTURE TEST PASSED (see duration and quality counters; production integration untested)\n" : "REFERENCE FRONTEND TEST PASSED (TS reception untested)\n") : "REFERENCE FRONTEND TEST FAILED\n");
				}
			}
			CloseHandle(cancel);
		}
	}
	WinUsb_Free(usb);
	CloseHandle(file);
	return ok;
}

int main(int argc, char **argv)
{
	std::string requested_id;
	// 末尾の任意selectorを先に取り除き、既存診断モードの引数契約を維持する。
	if (argc >= 3 && std::string(argv[argc - 2]) == "--device-id") {
		if (!px4::torne::NormalizeInterfaceId(argv[argc - 1], requested_id)) {
			std::cerr << "--device-id requires exactly 16 hexadecimal digits\n";
			return 2;
		}
		argc -= 2;
	}
	unsigned initialization_timeout = 0;
	unsigned duration_ms = 0;
	bool identify = false;
	bool frontend = false;
	bool receive = false;
	bool receiver_mode = false;
	bool receiver_fault = false;
	bool card_status = false;
	bool card_atr = false;
	std::vector<unsigned> channels;
	if (argc != 1) {
		if ((argc != 4 && argc != 6 && argc != 8) || (std::string(argv[1]) != "--card-atr" && std::string(argv[1]) != "--card-status" && std::string(argv[1]) != "--initialize" && std::string(argv[1]) != "--identify" && std::string(argv[1]) != "--frontend" && std::string(argv[1]) != "--receive" && std::string(argv[1]) != "--receiver" && std::string(argv[1]) != "--receiver-recovery") || std::string(argv[2]) != "--timeout-ms") {
			std::cerr << "Usage: torne_usb_probe.exe [--initialize|--identify|--frontend|--receive|--receiver|--receiver-recovery --timeout-ms <positive-budget> [--channels 13,14,... [--duration-ms N]]]\n";
			return 2;
		}
		receiver_fault = std::string(argv[1]) == "--receiver-recovery";
		card_atr = std::string(argv[1]) == "--card-atr";
		card_status = card_atr || std::string(argv[1]) == "--card-status";
		receiver_mode = receiver_fault || std::string(argv[1]) == "--receiver";
		receive = receiver_mode || std::string(argv[1]) == "--receive";
		frontend = receive || std::string(argv[1]) == "--frontend";
		identify = frontend || std::string(argv[1]) == "--identify";
		try {
			const std::string value = argv[3];
			std::size_t parsed = 0;
			const auto timeout = std::stoul(value, &parsed);
			if (value.empty() || value[0] == '-' || parsed != value.size() || !timeout || timeout >= MAXDWORD)
				throw std::invalid_argument("timeout");
			initialization_timeout = static_cast<unsigned>(timeout);
			if (argc == 8) {
				if (!receive || std::string(argv[6]) != "--duration-ms") throw std::invalid_argument("duration mode");
				const std::string text = argv[7];
				std::size_t used = 0;
				const auto duration = std::stoul(text, &used);
				if (text.empty() || text[0] == '-' || used != text.size() || !duration || duration >= timeout) throw std::invalid_argument("duration");
				duration_ms = static_cast<unsigned>(duration);
			}
			if (argc >= 6) {
				if (!frontend || std::string(argv[4]) != "--channels") throw std::invalid_argument("channels mode");
				std::stringstream list(argv[5]);
				std::string item;
				while (std::getline(list, item, ',')) {
					std::size_t used = 0;
					const auto channel = std::stoul(item, &used);
					if (used != item.size() || channel < 13 || channel > 62 || channels.size() >= 128) throw std::invalid_argument("channel");
					channels.push_back(static_cast<unsigned>(channel));
				}
				if (channels.empty()) throw std::invalid_argument("empty channel list");
			}
		} catch (...) {
			std::cerr << "Invalid initialization timeout\n";
			return 2;
		}
	}
	if (receive && channels.empty()) {
		std::cerr << "--receive requires an explicit --channels list\n";
		return 2;
	}
	HDEVINFO info = SetupDiGetClassDevsW(&torne_interface, nullptr, nullptr, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
	if (info == INVALID_HANDLE_VALUE)
		return 1;
	bool ok = true;
	unsigned count = 0;
	std::vector<std::wstring> paths;
	for (DWORD i = 0; ; ++i) {
		SP_DEVICE_INTERFACE_DATA entry{};
		entry.cbSize = sizeof(entry);
		if (!SetupDiEnumDeviceInterfaces(info, nullptr, &torne_interface, i, &entry)) {
			ok = ok && GetLastError() == ERROR_NO_MORE_ITEMS;
			break;
		}
		DWORD required = 0;
		SetupDiGetDeviceInterfaceDetailW(info, &entry, nullptr, 0, &required, nullptr);
		if (GetLastError() != ERROR_INSUFFICIENT_BUFFER || required < sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W)) {
			ok = false;
			continue;
		}
		// ワイド文字構造体を格納できるアラインメントを確保する。
		std::vector<std::uint64_t> storage((required + sizeof(std::uint64_t) - 1) / sizeof(std::uint64_t));
		auto *detail = reinterpret_cast<SP_DEVICE_INTERFACE_DETAIL_DATA_W *>(storage.data());
		detail->cbSize = sizeof(*detail);
		if (!SetupDiGetDeviceInterfaceDetailW(info, &entry, detail, required, nullptr, nullptr)) {
			ok = false;
			continue;
		}
		++count;
		paths.emplace_back(detail->DevicePath);
	}
	SetupDiDestroyDeviceInfoList(info);
	// 不完全な列挙・未指定の複数台・ID衝突は拒否。選択失敗で他の個体へfallbackしない。
	if (initialization_timeout || !requested_id.empty()) {
		std::size_t selected = paths.size();
		const auto selection = px4::torne::SelectInterface(paths, requested_id, selected);
		if (!ok || selection != px4::torne::SelectionResult::ok) {
			std::cerr << "Exactly one completely enumerated device must match; selection=" << static_cast<unsigned>(selection) << '\n';
			return 1;
		}
		const std::wstring chosen = paths[selected];
		paths.assign(1, chosen);
	}
	if (initialization_timeout)
		std::cout << "INITIALIZATION MODE: caller must have stopped BonDriverCoax/coax on this device.\n";
	for (const auto &path : paths) {
		const bool device_ok = Inspect(path.c_str(), initialization_timeout, identify, frontend, receive, channels, duration_ms, receiver_mode, receiver_fault, card_status, card_atr);
		ok = ok && device_ok;
	}
	std::cout << "Present interfaces: " << count << '\n';
	return count && ok ? 0 : 1;
}
