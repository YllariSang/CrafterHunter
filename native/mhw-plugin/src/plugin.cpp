#include "protocol.hpp"

#include <atomic>
#include <cstdint>
#include <string_view>

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

namespace {

std::atomic_bool g_running{false};

bool send_packet(
    const SOCKET socket,
    const crafterhunter::Kind kind,
    const std::uint32_t sequence,
    const std::span<const std::uint8_t> payload = {}
) {
    const auto packet = crafterhunter::make_packet(kind, sequence, payload);
    if (packet.empty()) {
        return false;
    }
    const int sent = send(
        socket,
        reinterpret_cast<const char*>(packet.data()),
        static_cast<int>(packet.size()),
        0
    );
    return sent == static_cast<int>(packet.size());
}

DWORD WINAPI endpoint_thread(void*) {
    WSADATA winsock_data{};
    if (WSAStartup(MAKEWORD(2, 2), &winsock_data) != 0) {
        return 1;
    }

    const SOCKET socket = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (socket == INVALID_SOCKET) {
        WSACleanup();
        return 2;
    }

    sockaddr_in bridge{};
    bridge.sin_family = AF_INET;
    bridge.sin_port = htons(38470);
    inet_pton(AF_INET, "127.0.0.1", &bridge.sin_addr);
    if (connect(socket, reinterpret_cast<const sockaddr*>(&bridge), sizeof(bridge)) == SOCKET_ERROR) {
        closesocket(socket);
        WSACleanup();
        return 3;
    }

    std::uint32_t sequence = 1;
    constexpr std::string_view name = "crafterhunter-mhw/0.1.0";
    const auto name_bytes = std::span{
        reinterpret_cast<const std::uint8_t*>(name.data()),
        name.size()
    };
    send_packet(socket, crafterhunter::Kind::hello, sequence, name_bytes);

    while (g_running.load(std::memory_order_relaxed)) {
        Sleep(1000);
        send_packet(socket, crafterhunter::Kind::heartbeat, ++sequence);
    }

    closesocket(socket);
    WSACleanup();
    return 0;
}

} // namespace

extern "C" __declspec(dllexport) const char* CrafterHunterPluginVersion() {
    return "0.1.0";
}

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID) {
    switch (reason) {
    case DLL_PROCESS_ATTACH: {
        DisableThreadLibraryCalls(module);
        g_running.store(true, std::memory_order_relaxed);
        const HANDLE thread = CreateThread(nullptr, 0, endpoint_thread, nullptr, 0, nullptr);
        if (thread != nullptr) {
            CloseHandle(thread);
        }
        break;
    }
    case DLL_PROCESS_DETACH:
        g_running.store(false, std::memory_order_relaxed);
        break;
    default:
        break;
    }
    return TRUE;
}
