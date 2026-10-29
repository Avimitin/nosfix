#ifdef NDEBUG
#undef NDEBUG
#endif

#include "nosfix/raw_input_capture.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <thread>
#include <vector>

namespace {

template <typename T>
void Put(std::vector<std::byte>& bytes, std::size_t offset, const T& value) {
    assert(offset + sizeof(T) <= bytes.size());
    std::memcpy(bytes.data() + offset, &value, sizeof(T));
}

template <typename T>
T Get(std::span<const std::byte> bytes, std::size_t offset) {
    assert(offset + sizeof(T) <= bytes.size());
    T value{};
    std::memcpy(&value, bytes.data() + offset, sizeof(T));
    return value;
}

std::vector<std::byte> KeyboardPacket(bool pressed) {
    std::vector<std::byte> bytes(sizeof(RAWINPUT));
    RAWINPUTHEADER header{};
    header.dwType = RIM_TYPEKEYBOARD;
    header.dwSize = static_cast<DWORD>(bytes.size());
    header.hDevice = reinterpret_cast<HANDLE>(0x1234);
    header.wParam = RIM_INPUT;
    Put(bytes, 0, header);

    RAWKEYBOARD keyboard{};
    keyboard.MakeCode = 0x1e;
    keyboard.Flags = pressed ? 0 : RI_KEY_BREAK;
    keyboard.VKey = 'A';
    keyboard.Message = pressed ? WM_KEYDOWN : WM_KEYUP;
    Put(bytes, offsetof(RAWINPUT, data.keyboard), keyboard);
    return bytes;
}

std::vector<std::byte> KeyboardPacketFor(std::size_t event_index) {
    std::vector<std::byte> bytes(sizeof(RAWINPUT));
    RAWINPUTHEADER header{};
    header.dwType = RIM_TYPEKEYBOARD;
    header.dwSize = static_cast<DWORD>(bytes.size());
    Put(bytes, 0, header);

    RAWKEYBOARD keyboard{};
    keyboard.MakeCode = static_cast<USHORT>(event_index & 0xffU);
    keyboard.Flags = (event_index & 1U) ? RI_KEY_BREAK : 0;
    keyboard.VKey = static_cast<USHORT>('A' + event_index % 26U);
    keyboard.Message = (event_index & 1U) ? WM_KEYUP : WM_KEYDOWN;
    Put(bytes, offsetof(RAWINPUT, data.keyboard), keyboard);
    return bytes;
}

std::vector<std::byte> HidBatchPacket() {
    constexpr std::size_t data_offset = offsetof(RAWINPUT, data.hid.bRawData);
    constexpr std::size_t report_size = 4;
    constexpr std::size_t report_count = 2;
    const std::size_t size = data_offset + report_size * report_count;
    std::vector<std::byte> bytes(size);

    RAWINPUTHEADER header{};
    header.dwType = RIM_TYPEHID;
    header.dwSize = static_cast<DWORD>(size);
    header.hDevice = reinterpret_cast<HANDLE>(0x5678);
    header.wParam = RIM_INPUTSINK;
    Put(bytes, 0, header);

    const DWORD dw_size = report_size;
    const DWORD dw_count = report_count;
    Put(bytes, offsetof(RAWINPUT, data.hid.dwSizeHid), dw_size);
    Put(bytes, offsetof(RAWINPUT, data.hid.dwCount), dw_count);
    const std::array<std::byte, report_size * report_count> reports{
        std::byte{0x10}, std::byte{0x11}, std::byte{0x12}, std::byte{0x13},
        std::byte{0x20}, std::byte{0x21}, std::byte{0x22}, std::byte{0x23},
    };
    std::memcpy(bytes.data() + data_offset, reports.data(), reports.size());
    return bytes;
}

void TestMessageFilter() {
    MSG message{};
    const HWND target = reinterpret_cast<HWND>(0x2345);
    message.hwnd = target;
    message.message = WM_INPUT;

    assert(nosfix::IsRawInputCaptureMessage(HC_ACTION, PM_REMOVE, message, target));
    assert(!nosfix::IsRawInputCaptureMessage(HC_ACTION, PM_NOREMOVE, message, target));
    assert(!nosfix::IsRawInputCaptureMessage(HC_NOREMOVE, PM_REMOVE, message, target));
    assert(!nosfix::IsRawInputCaptureMessage(HC_ACTION, PM_REMOVE, message, nullptr));
    message.hwnd = reinterpret_cast<HWND>(0x2346);
    assert(!nosfix::IsRawInputCaptureMessage(HC_ACTION, PM_REMOVE, message, target));
    message.hwnd = target;
    message.message = WM_DEVICECHANGE;
    assert(!nosfix::IsRawInputCaptureMessage(HC_ACTION, PM_REMOVE, message, target));
}

void TestPreservationAndBatch() {
    nosfix::RawInputPacketQueue queue;
    assert(queue.Ready());

    const auto down = KeyboardPacket(true);
    const auto up = KeyboardPacket(false);
    assert(queue.PushRawInput(10'000, down));
    assert(queue.PushRawInput(12'500, up));

    nosfix::RawInputPacket packet;
    assert(queue.TryPop(packet));
    assert(packet.kind == nosfix::RawInputPacketKind::Input);
    assert(packet.sequence == 1 && packet.qpc == 10'000);
    assert(packet.raw_type == RIM_TYPEKEYBOARD && packet.payload_size == down.size());
    assert(std::memcmp(packet.payload.data(), down.data(), down.size()) == 0);
    const auto down_report = Get<RAWKEYBOARD>(
        {packet.payload.data(), packet.payload_size}, offsetof(RAWINPUT, data.keyboard));
    assert(down_report.VKey == 'A' && (down_report.Flags & RI_KEY_BREAK) == 0);

    assert(queue.TryPop(packet));
    assert(packet.qpc == 12'500 && packet.stream_epoch == 1);
    const auto up_report = Get<RAWKEYBOARD>(
        {packet.payload.data(), packet.payload_size}, offsetof(RAWINPUT, data.keyboard));
    assert(up_report.VKey == 'A' && (up_report.Flags & RI_KEY_BREAK) != 0);
    assert(!queue.TryPop(packet));

    const auto hid = HidBatchPacket();
    assert(queue.PushRawInput(15'000, hid));
    assert(queue.TryPop(packet));
    assert(packet.raw_type == RIM_TYPEHID && packet.qpc == 15'000);
    const auto report_count = Get<DWORD>(
        {packet.payload.data(), packet.payload_size}, offsetof(RAWINPUT, data.hid.dwCount));
    const auto report_size = Get<DWORD>(
        {packet.payload.data(), packet.payload_size}, offsetof(RAWINPUT, data.hid.dwSizeHid));
    assert(report_count == 2 && report_size == 4);
    const auto report_offset = offsetof(RAWINPUT, data.hid.bRawData);
    assert(packet.payload[report_offset] == std::byte{0x10});
    assert(packet.payload[report_offset + report_size] == std::byte{0x20});
}

void TestLossBoundariesAndValidation() {
    nosfix::RawInputPacketQueue queue;
    assert(queue.Ready());
    const auto valid = KeyboardPacket(true);
    nosfix::RawInputPacket packet;

    auto malformed = valid;
    RAWINPUTHEADER bad_header{};
    std::memcpy(&bad_header, malformed.data(), sizeof(bad_header));
    --bad_header.dwSize;
    Put(malformed, 0, bad_header);
    assert(!queue.PushRawInput(100, malformed));
    assert(queue.Stats().malformed_packets == 1);
    assert(queue.TryPop(packet));
    assert(packet.kind == nosfix::RawInputPacketKind::StreamReset);
    assert(packet.reset_reason == nosfix::RawInputResetReason::Malformed);
    assert(packet.stream_epoch == 2 && packet.qpc == 100);

    std::vector<std::byte> too_large(nosfix::kRawInputCapturePayloadBytes + 1);
    RAWINPUTHEADER oversize_header{};
    oversize_header.dwType = RIM_TYPEHID;
    oversize_header.dwSize = static_cast<DWORD>(too_large.size());
    Put(too_large, 0, oversize_header);
    assert(!queue.PushRawInput(200, too_large));
    assert(queue.TryPop(packet));
    assert(packet.kind == nosfix::RawInputPacketKind::StreamReset);
    assert(packet.reset_reason == nosfix::RawInputResetReason::Oversize);
    assert(queue.Stats().oversize_packets == 1);

    std::array<std::byte, sizeof(RAWINPUTHEADER) - 1> short_payload{};
    assert(!queue.PushRawInput(300, short_payload));
    assert(queue.TryPop(packet));
    assert(packet.reset_reason == nosfix::RawInputResetReason::Malformed);

    auto mouse = valid;
    RAWINPUTHEADER mouse_header{};
    std::memcpy(&mouse_header, mouse.data(), sizeof(mouse_header));
    mouse_header.dwType = RIM_TYPEMOUSE;
    Put(mouse, 0, mouse_header);
    assert(!queue.PushRawInput(400, mouse));
    assert(!queue.TryPop(packet));
    assert(queue.Stats().ignored_mouse_packets == 1);

    assert(queue.PushRawInput(500, valid));
    queue.MarkGap(600, nosfix::RawInputResetReason::DeviceChange);
    assert(queue.TryPop(packet));
    assert(packet.kind == nosfix::RawInputPacketKind::Input);
    assert(packet.sequence == 4 && packet.qpc == 500);
    assert(queue.TryPop(packet));
    assert(packet.kind == nosfix::RawInputPacketKind::StreamReset);
    assert(packet.sequence == 5);
    assert(packet.reset_reason == nosfix::RawInputResetReason::DeviceChange);
    assert(packet.qpc == 600);
    assert(!queue.TryPop(packet));

    assert(queue.PushRawInput(700, valid));
    assert(queue.TryPop(packet));
    assert(packet.sequence == 6);
    assert(packet.kind == nosfix::RawInputPacketKind::Input);
    assert(packet.qpc == 700 && packet.stream_epoch == queue.Stats().stream_epoch);
}

void TestOverflow() {
    nosfix::RawInputPacketQueue queue;
    assert(queue.Ready());
    const auto input = KeyboardPacket(true);
    for (std::size_t index = 0; index < nosfix::kRawInputCaptureMaxInputPackets; ++index) {
        assert(queue.PushRawInput(static_cast<std::int64_t>(index), input));
    }
    // Exact QPCs must be monotonic; the previous arbitrary literals would now
    // be a ClockReversal fault rather than an overflow. Capacity semantics are
    // unchanged: 8191 inputs, one reserved reset, coalesced further failures.
    assert(!queue.PushRawInput(9'000, input));
    assert(!queue.PushRawInput(9'001, input)); // coalesced while reset slot is full
    auto stats = queue.Stats();
    assert(stats.dropped_packets == 2 && stats.stream_epoch == 2);

    nosfix::RawInputPacket packet;
    for (std::size_t index = 0; index < nosfix::kRawInputCaptureMaxInputPackets; ++index) {
        assert(queue.TryPop(packet));
        assert(packet.kind == nosfix::RawInputPacketKind::Input);
        assert(packet.sequence == index + 1);
    }
    assert(queue.TryPop(packet));
    assert(packet.kind == nosfix::RawInputPacketKind::StreamReset);
    assert(packet.sequence == nosfix::kRawInputCaptureCapacity);
    assert(packet.reset_reason == nosfix::RawInputResetReason::QueueOverflow);
    assert(packet.stream_epoch == 2);
    assert(!queue.TryPop(packet));

    assert(queue.PushRawInput(10'000, input));
    assert(queue.TryPop(packet));
    assert(packet.kind == nosfix::RawInputPacketKind::Input);
    assert(packet.sequence == nosfix::kRawInputCaptureCapacity + 1);
    assert(packet.qpc == 10'000 && packet.stream_epoch == 2);
}

void TestRawPayloadValidation() {
    nosfix::RawInputPacketQueue queue;
    assert(queue.Ready());
    nosfix::RawInputPacket packet;

    std::vector<std::byte> short_keyboard(sizeof(RAWINPUTHEADER));
    RAWINPUTHEADER keyboard_header{};
    keyboard_header.dwType = RIM_TYPEKEYBOARD;
    keyboard_header.dwSize = static_cast<DWORD>(short_keyboard.size());
    Put(short_keyboard, 0, keyboard_header);
    assert(!queue.PushRawInput(1, short_keyboard));
    assert(queue.TryPop(packet));
    assert(packet.reset_reason == nosfix::RawInputResetReason::Malformed);

    auto unknown_type = KeyboardPacket(true);
    RAWINPUTHEADER unknown_header{};
    std::memcpy(&unknown_header, unknown_type.data(), sizeof(unknown_header));
    unknown_header.dwType = 99;
    Put(unknown_type, 0, unknown_header);
    assert(!queue.PushRawInput(2, unknown_type));
    assert(queue.TryPop(packet));
    assert(packet.reset_reason == nosfix::RawInputResetReason::Malformed);

    auto malformed_hid = HidBatchPacket();
    DWORD impossible_count = UINT32_MAX;
    Put(malformed_hid, offsetof(RAWINPUT, data.hid.dwCount), impossible_count);
    assert(!queue.PushRawInput(3, malformed_hid));
    assert(queue.TryPop(packet));
    assert(packet.reset_reason == nosfix::RawInputResetReason::Malformed);
    assert(queue.Stats().malformed_packets == 3);
}

void TestRapidEdgesAcrossPollStall() {
    nosfix::RawInputPacketQueue queue;
    assert(queue.Ready());
    constexpr std::int64_t qpc_frequency = 10'000'000;
    constexpr std::size_t edge_count = 4'000;
    constexpr std::int64_t event_step = qpc_frequency / 1'000;
    const auto down = KeyboardPacket(true);
    const auto up = KeyboardPacket(false);

    std::size_t next_event = 0;
    std::size_t received = 0;
    nosfix::RawInputPacket packet;
    // The producer continues at 1 kHz while the consumer polls around 60 Hz.
    // Simulate a 1.53 s game-thread stall after the fourth poll; the queue
    // must preserve each down/up edge and its original QPC value.
    for (std::size_t poll = 0; next_event < edge_count; ++poll) {
        const bool stalled = poll >= 4 && poll < 94;
        const std::size_t target_event = std::min(
            edge_count, (poll + 1) * 17);
        while (next_event < target_event) {
            const auto& payload = (next_event & 1U) ? up : down;
            const auto qpc = 5'000'000'000LL +
                static_cast<std::int64_t>(next_event) * event_step;
            assert(queue.PushRawInput(qpc, payload));
            ++next_event;
        }
        if (stalled) continue;
        while (queue.TryPop(packet)) {
            assert(packet.kind == nosfix::RawInputPacketKind::Input);
            assert(packet.sequence == received + 1 && packet.stream_epoch == 1);
            assert(packet.qpc == 5'000'000'000LL +
                static_cast<std::int64_t>(received) * event_step);
            const auto key = Get<RAWKEYBOARD>(
                {packet.payload.data(), packet.payload_size},
                offsetof(RAWINPUT, data.keyboard));
            assert(((key.Flags & RI_KEY_BREAK) != 0) == ((received & 1U) != 0));
            ++received;
        }
    }
    while (queue.TryPop(packet)) {
        assert(packet.kind == nosfix::RawInputPacketKind::Input);
        ++received;
    }
    assert(received == edge_count);
    assert(queue.Stats().dropped_packets == 0);
}

void TestConcurrentProducerConsumer() {
    nosfix::RawInputPacketQueue queue;
    assert(queue.Ready());
    constexpr std::size_t event_count = 100'000;
    constexpr std::int64_t qpc_base = 8'000'000'000LL;
    constexpr std::int64_t qpc_step = 10'000;
    std::atomic<bool> producer_done{false};
    std::thread producer([&] {
        for (std::size_t index = 0; index < event_count; ++index) {
            const auto qpc = qpc_base + static_cast<std::int64_t>(index) * qpc_step;
            if (index != 0 && index % 4093 == 0) {
                queue.MarkGap(qpc, nosfix::RawInputResetReason::DeviceChange);
            } else {
                const auto input = KeyboardPacketFor(index);
                (void)queue.PushRawInput(qpc, input);
            }
            if ((index & 127U) == 0) std::this_thread::yield();
        }
        producer_done.store(true, std::memory_order_release);
    });

    std::uint64_t previous_sequence = 0;
    std::uint64_t current_epoch = 1;
    std::int64_t previous_qpc = qpc_base;
    std::size_t inputs = 0;
    std::size_t resets = 0;
    nosfix::RawInputPacket packet;
    for (;;) {
        if (queue.TryPop(packet)) {
            assert(packet.sequence > previous_sequence);
            assert(packet.qpc >= previous_qpc);
            previous_sequence = packet.sequence;
            previous_qpc = packet.qpc;

            if (packet.kind == nosfix::RawInputPacketKind::StreamReset) {
                assert(packet.reset_reason != nosfix::RawInputResetReason::None);
                assert(packet.stream_epoch == current_epoch + 1);
                current_epoch = packet.stream_epoch;
                ++resets;
            } else {
                assert(packet.stream_epoch == current_epoch);
                assert(packet.raw_type == RIM_TYPEKEYBOARD);
                const auto event_index = static_cast<std::size_t>(
                    (packet.qpc - qpc_base) / qpc_step);
                assert(packet.qpc == qpc_base +
                    static_cast<std::int64_t>(event_index) * qpc_step);
                const auto key = Get<RAWKEYBOARD>(
                    {packet.payload.data(), packet.payload_size},
                    offsetof(RAWINPUT, data.keyboard));
                assert(key.VKey == static_cast<USHORT>('A' + event_index % 26U));
                assert(((key.Flags & RI_KEY_BREAK) != 0) == ((event_index & 1U) != 0));
                ++inputs;
            }
            continue;
        }
        if (producer_done.load(std::memory_order_acquire)) break;
        std::this_thread::yield();
    }
    producer.join();

    // Drain anything committed immediately before the producer's completion
    // flag; the consumer validates the same sequence, epoch and packet fields.
    while (queue.TryPop(packet)) {
        assert(packet.sequence > previous_sequence);
        assert(packet.qpc >= previous_qpc);
        previous_sequence = packet.sequence;
        previous_qpc = packet.qpc;
        if (packet.kind == nosfix::RawInputPacketKind::StreamReset) {
            assert(packet.stream_epoch == current_epoch + 1);
            current_epoch = packet.stream_epoch;
            ++resets;
        } else {
            assert(packet.stream_epoch == current_epoch);
            assert(packet.raw_type == RIM_TYPEKEYBOARD);
            ++inputs;
        }
    }
    assert(inputs != 0 && resets != 0);
}

} // namespace

int main() {
    TestMessageFilter();
    TestPreservationAndBatch();
    TestLossBoundariesAndValidation();
    TestOverflow();
    TestRawPayloadValidation();
    TestRapidEdgesAcrossPollStall();
    TestConcurrentProducerConsumer();
}
