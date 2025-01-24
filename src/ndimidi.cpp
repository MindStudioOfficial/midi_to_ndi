#include "ndimidi.hpp"

NDI_MIDI_Manager::NDI_MIDI_Manager() {

    NDIlib_initialize();

    auto find_create_desc = NDIlib_find_create_t(
        true, nullptr, nullptr);

    m_p_find = NDIlib_find_create_v2(&find_create_desc);

    if (!m_p_find) {
        std::println("cannot create NDI find instance");
        return;
    }

    auto send_create_desc = NDIlib_send_create_t(
        "NDI MIDI",
        nullptr,
        false,
        false);

    m_p_send = NDIlib_send_create(&send_create_desc);

    auto recv_create_desc            = NDIlib_recv_create_v3_t();
    recv_create_desc.p_ndi_recv_name = "NDI MIDI";

    m_p_recv = NDIlib_recv_create_v3(&recv_create_desc);
}

NDI_MIDI_Manager::~NDI_MIDI_Manager() {

    DisconnectFromSource();

    if (m_p_find) {
        NDIlib_find_destroy(m_p_find);
    }

    m_p_find = nullptr;

    if (m_p_send) {
        NDIlib_send_destroy(m_p_send);
    }

    m_p_send = nullptr;

    if (m_p_recv) {
        NDIlib_recv_destroy(m_p_recv);
    }

    m_p_recv = nullptr;

    NDIlib_destroy();
}

void NDI_MIDI_Manager::UpdateSources() {

    if (!m_p_find) {
        return;
    }

    if (!NDIlib_find_wait_for_sources(m_p_find, 5000)) {
        std::println("timeout waiting for sources");
    } else {

        const auto p_sources = NDIlib_find_get_current_sources(m_p_find, &m_n_sources);

        m_p_sources.clear();
        m_p_sources.reserve(m_n_sources);

        if (p_sources) {
            std::println("found {} sources", m_n_sources);

            for (uint32_t i = 0; i < m_n_sources; i++) {
                std::println("source {}: {}", i, p_sources[i].p_ndi_name);
                m_p_sources.push_back(p_sources[i]);
            }
        }
    }
}

void NDI_MIDI_Manager::ConnectToSource(const NDIlib_source_t* source) const {
    if (!m_p_recv) {
        return;
    }

    if (!source) {
        return;
    }

    NDIlib_recv_connect(m_p_recv, source);
}

void NDI_MIDI_Manager::DisconnectFromSource() const {

    if (!m_p_recv) {
        return;
    }
    NDIlib_recv_connect(m_p_recv, nullptr);
}

void NDI_MIDI_Manager::SendMIDI(const std::span<uint8_t>& data) const {
    std::string metadata_message("<MIDI>");
    metadata_message.reserve(data.size() + 7);

    for (const auto& byte : data) {
        metadata_message += std::format("{:02X}", byte);
    }

    metadata_message += "</MIDI>";

    const NDIlib_metadata_frame_t metadata_frame{
        static_cast<int>(metadata_message.size()),
        NDIlib_send_timecode_synthesize,
        const_cast<char*>(metadata_message.c_str())};

    NDIlib_send_send_metadata(m_p_send, &metadata_frame);
}

const std::optional<std::string> NDI_MIDI_Manager::ReceiveMIDI(uint32_t wait_time_ms) const {

    if (!m_p_recv) {
        return std::nullopt;
    }

    NDIlib_metadata_frame_t metadata_frame;

    switch (NDIlib_recv_capture_v3(
        m_p_recv, nullptr, nullptr, &metadata_frame, wait_time_ms)) {
    case NDIlib_frame_type_metadata:
        if (metadata_frame.length > 0) {
            auto str = std::string(metadata_frame.p_data, metadata_frame.length);
            NDIlib_recv_free_metadata(m_p_recv, &metadata_frame);
            return str;
        }
        break;
    case NDIlib_frame_type_none:
        break;
    // The device has changed status in some way (see notes below)
    case NDIlib_frame_type_status_change:
        break;
    }
    return std::nullopt;
}

std::vector<uint8_t> NDI_MIDI_Manager::ParseMIDIMessage(const std::string_view& message) const {
    auto data = std::vector<uint8_t>();

    if (message.length() < 13) {
        return data;
    }

    if (message.substr(0, 6).compare("<MIDI>") != 0 || message.substr(message.size() - 7, 7).compare("</MIDI>") != 0) {
        return data;
    }

    auto midi_hex_message = message.substr(6, message.size() - 13);

    for (size_t i = 0; i < midi_hex_message.size(); i += 2) {
        data.push_back(
            (uint8_t)(std::stoi(
                          std::string(midi_hex_message.substr(i, 2)),
                          nullptr, 16) &
                      0xFF));
    }

    return data;
}

MIDI_IO_MANAGER::MIDI_IO_MANAGER(const std::wstring_view& port_name) {

    // MIDI Output via virtualMIDI

    WORD major, minor, release, build;

    virtualMIDIGetVersion(&major, &minor, &release, &build);

    std::println("teVirtualMIDI Version: {}.{}.{}.{}", major, minor, release, build);

    WORD driver_major, driver_minor, driver_release, driver_build;

    virtualMIDIGetDriverVersion(&driver_major, &driver_minor, &driver_release, &driver_build);

    std::println("using dll-version: {}.{}.{}.{}", driver_major, driver_minor, driver_release, driver_build);

#ifdef _DEBUG

    virtualMIDILogging(TE_VM_LOGGING_MISC | TE_VM_LOGGING_RX | TE_VM_LOGGING_TX);

#endif

    m_p_port = virtualMIDICreatePortEx2(
        port_name.data(),
        [](LPVM_MIDI_PORT midiPort,
           LPBYTE         midiDataBytes,
           DWORD          length,
           DWORD_PTR      dwCallbackInstance) {
            if ((NULL == midiDataBytes) || (0 == length)) {
                std::println("empty command - driver was probably shut down!");
                return;
            }

            if (!virtualMIDISendData(midiPort, midiDataBytes, length)) {
                std::println("error sending data: {}", GetLastError());
                return;
            }

            std::println("command: {}", binToStr(midiDataBytes, length));
        },
        0, MAX_SYSEX_BUFFER, TE_VM_FLAGS_PARSE_RX);

    if (!m_p_port) {
        std::println("could not create port: {}", GetLastError());
    }

    // MIDI Input via RtMidi

    std::vector<RtMidi::Api> apis;

    RtMidi::getCompiledApi(apis);

    std::println("compiled RtMidi APIs:");

    for (const auto& api : apis) {
        std::println("\t- {}", RtMidi::getApiName(api));
    }

    try {
        m_p_midi_in = std::make_unique<RtMidiIn>(
            RtMidi::Api::UNSPECIFIED, 
            "RtMidi Input Client", 
            1000
        );
    } catch (RtMidiError& error) {
        std::println("error creating RtMidiIn: {}", error.getMessage());
        exit(EXIT_FAILURE);
        return;
    }
}

MIDI_IO_MANAGER::~MIDI_IO_MANAGER() {
    if (m_p_port) {
        virtualMIDIClosePort(m_p_port);
    }

    if (m_p_midi_in && m_p_midi_in->isPortOpen()) {
        m_p_midi_in->closePort();
    }
}

std::string MIDI_IO_MANAGER::binToStr(const uint8_t* data, DWORD length) {
    std::string dumpBuffer;
    dumpBuffer.reserve(MAX_SYSEX_BUFFER * 3);
    for (DWORD i = 0; i < length; i++) {
        dumpBuffer += std::format("{:02x}", data[i]);
        if (i < length - 1) {
            dumpBuffer += ":";
        }
    }
    return dumpBuffer;
}

void MIDI_IO_MANAGER::UpdateMIDIPorts() {

    if (!m_p_midi_in) {
        return;
    }

    m_port_names.clear();
    m_n_ports = m_p_midi_in->getPortCount();

    for (unsigned int i = 0; i < m_n_ports; i++) {
        std::string port_name("unknown");
        try {
            port_name = m_p_midi_in->getPortName(i);
        } catch (RtMidiError& error) {
            std::println("error getting port name: {}", error.getMessage());
        }
        m_port_names.push_back(port_name);
        // std::println("port {}: {}", i, port_name);
    }
}

bool MIDI_IO_MANAGER::OpenMIDIPort(uint32_t port_number) {
    try {
        m_p_midi_in->openPort(port_number);
    } catch (RtMidiError& error) {
        error.printMessage();
        return false;
    }

    m_p_midi_in->ignoreTypes(false, false, false);

    std::println("Reading MIDI from API {}, port {}",
                 m_p_midi_in->getApiDisplayName(m_p_midi_in->getCurrentApi()),
                 m_p_midi_in->getPortName(port_number));

    return true;
}

std::vector<uint8_t> MIDI_IO_MANAGER::ReceiveMIDI() {
    if (!m_p_midi_in->isPortOpen()) {
        return std::vector<uint8_t>();
    }

    std::vector<unsigned char> message;
    double                     timestamp = 0.0;

    timestamp = m_p_midi_in->getMessage(&message);

    return message;
}

bool MIDI_IO_MANAGER::SendMIDI(const std::span<uint8_t>& data) const {
    bool res = virtualMIDISendData(m_p_port, data.data(), (DWORD)data.size());

    if (!res) {
        std::println("error sending data: {}", GetLastError());
    }

    return res;
}
