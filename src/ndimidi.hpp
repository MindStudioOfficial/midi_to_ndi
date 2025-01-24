#include "pch.hpp"

class NDI_MIDI_Manager {
public:
    NDI_MIDI_Manager();
    ~NDI_MIDI_Manager();

    // delete copy constructor and assignment operator
    NDI_MIDI_Manager(const NDI_MIDI_Manager&)            = delete;
    NDI_MIDI_Manager& operator=(const NDI_MIDI_Manager&) = delete;

    // delete move constructor and assignment operator
    NDI_MIDI_Manager(NDI_MIDI_Manager&&)            = delete;
    NDI_MIDI_Manager& operator=(NDI_MIDI_Manager&&) = delete;

public:
    void UpdateSources();

    [[nodiscard]]
    const std::vector<NDIlib_source_t>& GetSources() const {
        return m_p_sources;
    }

    void ConnectToSource(const NDIlib_source_t* source) const;

    void DisconnectFromSource() const;

    void SendMIDI(const std::span<uint8_t>& data) const;

    [[nodiscard]]
    const std::optional<std::string> ReceiveMIDI(uint32_t wait_time_ms) const;

    [[nodiscard]]
    std::vector<uint8_t> ParseMIDIMessage(const std::string_view& message) const;

private:
    NDIlib_find_instance_t       m_p_find    = nullptr;
    uint32_t                     m_n_sources = 0;
    std::vector<NDIlib_source_t> m_p_sources;

    NDIlib_send_instance_t m_p_send = nullptr;

    NDIlib_recv_instance_t m_p_recv = nullptr;
};

#define MAX_SYSEX_BUFFER 65535

class MIDI_IO_MANAGER {

public:
    MIDI_IO_MANAGER(const std::wstring_view& port_name);
    ~MIDI_IO_MANAGER();

    bool SendMIDI(const std::span<uint8_t>& data) const;

private:
    [[nodiscard]]
    static std::string binToStr(const uint8_t* data, DWORD length);

    LPVM_MIDI_PORT m_p_port = nullptr;

    std::unique_ptr<RtMidiIn> m_p_midi_in = nullptr;
    uint32_t                  m_n_ports   = 0;
    std::vector<std::string>  m_port_names;

public:
    void UpdateMIDIPorts();

    const std::vector<std::string>& GetMIDIPorts() {
        return m_port_names;
    }

    [[nodiscard]]
    bool OpenMIDIPort(uint32_t port_number);

    std::vector<uint8_t> ReceiveMIDI();

    const std::unordered_map<int, std::string> apiMap{
        { RtMidi::MACOSX_CORE,      "OS-X CoreMIDI"},
        {  RtMidi::WINDOWS_MM, "Windows MultiMedia"},
        { RtMidi::WINDOWS_UWP,        "Windows UWP"},
        {   RtMidi::UNIX_JACK,               "Jack"},
        {  RtMidi::LINUX_ALSA,               "ALSA"},
        {RtMidi::RTMIDI_DUMMY,              "Dummy"},
    };
};