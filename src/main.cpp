#include "pch.hpp"
#include "ndimidi.hpp"

void receiveInteractive() {
    NDI_MIDI_Manager ndi_midi_manager;
    MIDI_IO_MANAGER  midi_io_manager(L"NDI MIDI");

    ndi_midi_manager.UpdateSources();

    const auto sources = ndi_midi_manager.GetSources();

    std::println("Which NDI source would you like to use?");

    for (size_t i = 0; i < sources.size(); i++) {
        std::println("{}: {}", i, sources[i].p_ndi_name);
    }

    std::string input;

    std::getline(std::cin, input);

    size_t source_index = std::stoi(input);

    if (source_index >= sources.size()) {
        std::println("Invalid input. Exiting...");
        return;
    }

    std::println("Connecting to source {}: {}", source_index, sources[source_index].p_ndi_name);

    ndi_midi_manager.ConnectToSource(&sources[source_index]);

    std::println("Starting reception, press enter to exit...");

    bool end = false;

    while (!end) {
        if (_kbhit()) {
            end = true;
        }
        auto data_string = ndi_midi_manager.ReceiveMIDI(100);

        if (!data_string.has_value()) {
            continue;
        }

        if (data_string.value().empty()) {
            continue;
        }
        auto data = ndi_midi_manager.ParseMIDIMessage(data_string.value());

        if (data.empty()) {
            continue;
        }

        midi_io_manager.SendMIDI(data);
    }
}

void transmitInteractive() {
    MIDI_IO_MANAGER midi_io_manager(L"NDI MIDI");

    midi_io_manager.UpdateMIDIPorts();
    auto ports = midi_io_manager.GetMIDIPorts();

    std::println("Which Midi input port would you like to use?");

    for (size_t i = 0; i < ports.size(); i++) {
        std::println("{}: {}", i, ports[i]);
    }

    std::string input;

    std::getline(std::cin, input);

    size_t port_index = std::stoi(input);

    if (port_index >= ports.size()) {
        std::println("Invalid input. Exiting...");
        return;
    }
    std::println("Opening MIDI Port {}", port_index);

    bool succ = midi_io_manager.OpenMIDIPort((uint32_t)port_index);

    if (!succ) {
        std::println("Error opening MIDI port. Exiting...");
        return;
    }

    NDI_MIDI_Manager ndi_midi_manager;

    std::println("Starting transmission, press enter to exit...");

    bool end = false;

    while (!end) {
        if (_kbhit()) {
            end = true;
        }

        auto data = midi_io_manager.ReceiveMIDI();

        if (data.size() > 0) {
            ndi_midi_manager.SendMIDI(data);
        }
    }

    std::println("Exiting...");
}

bool end_loop = false;

bool receive(const std::string_view& ndi_source, const std::string_view& midi_output_name) {
    NDI_MIDI_Manager ndi_midi_manager;

    ndi_midi_manager.UpdateSources();

    const auto sources = ndi_midi_manager.GetSources();

    int64_t source_index = -1;

    for (size_t i = 0; i < sources.size(); i++) {
        if (sources[i].p_ndi_name == ndi_source) {
            source_index = i;
            break;
        }
    }

    if (source_index == -1) {
        std::println("Invalid NDI source. Exiting...");
        return false;
    }

    ndi_midi_manager.ConnectToSource(&sources[source_index]);

    MIDI_IO_MANAGER midi_io_manager(midi_output_name);

    std::println("Starting reception, press enter to exit...");

    signal(SIGINT, [](int) {
        std::println("Exiting...");
        end_loop = true;
    });

    while (!end_loop) {
        if (_kbhit()) {
            end_loop = true;
        }
        auto data_string = ndi_midi_manager.ReceiveMIDI(100);
        if (!data_string.has_value()) {
            continue;
        }
        if (data_string.value().empty()) {
            continue;
        }
        auto data = ndi_midi_manager.ParseMIDIMessage(data_string.value());
        if (data.empty()) {
            continue;
        }
        midi_io_manager.SendMIDI(data);
    }

    return true;
}

bool transmit(const std::string_view& midi_input, const std::string_view& ndi_send_name) {
    MIDI_IO_MANAGER midi_io_manager(midi_input);
    midi_io_manager.UpdateMIDIPorts();
    auto    ports      = midi_io_manager.GetMIDIPorts();
    int64_t port_index = -1;
    for (size_t i = 0; i < ports.size(); i++) {
        if (ports[i] == midi_input) {
            port_index = i;
            break;
        }
    }
    if (port_index == -1) {
        std::println("Invalid MIDI port. Exiting...");
        return false;
    }
    bool succ = midi_io_manager.OpenMIDIPort((uint32_t)port_index);
    if (!succ) {
        std::println("Error opening MIDI port. Exiting...");
        return false;
    }
    NDI_MIDI_Manager ndi_midi_manager(ndi_send_name);
    std::println("Starting transmission, press enter to exit...");
    signal(SIGINT, [](int) {
        std::println("Exiting...");
        end_loop = true;
    });
    while (!end_loop) {
        if (_kbhit()) {
            end_loop = true;
        }
        auto data = midi_io_manager.ReceiveMIDI();
        if (data.size() > 0) {
            ndi_midi_manager.SendMIDI(data);
        }
    }
    return true;
}

void list() {
    NDI_MIDI_Manager ndi_midi_manager;
    MIDI_IO_MANAGER  midi_io_manager(L"NDI MIDI");
    ndi_midi_manager.UpdateSources();
    midi_io_manager.UpdateMIDIPorts();
    std::println("NDI Sources:");
    const auto sources = ndi_midi_manager.GetSources();
    for (size_t i = 0; i < sources.size(); i++) {
        std::println("{}: {}", i, sources[i].p_ndi_name);
    }
    std::println("MIDI Ports:");
    const auto ports = midi_io_manager.GetMIDIPorts();
    for (size_t i = 0; i < ports.size(); i++) {
        std::println("{}: {}", i, ports[i]);
    }
}

int main(int argc, char** argv) {

    namespace po = boost::program_options;

    po::options_description desc("Options");

    desc.add_options()("help,h", "produce help message")
        // List devices
        ("list,l", "list MIDI devices and NDI Sources")
        // "Modes"
        ("receive,r", "receive MIDI data")("transmit,t", "transmit MIDI data")
        // "Receive" options
        ("ndi-source", po::value<std::string>(), "NDI source name (required if -r)")
        // Optional
        ("midi-output-name", po::value<std::string>()->default_value("NDI MIDI"), "Optional: MIDI output name used in receive mode")
        // "Transmit" options
        ("midi-input", po::value<std::string>(),
         "MIDI input port name (required if -t)")
        // Optional
        ("ndi-send-name", po::value<std::string>()->default_value("NDI MIDI"),
         "Optional: NDI source name to create in transmit mode");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << desc << std::endl;
        return 1;
    }

    if (vm.count("list")) {
        list();
        return 0;
    }

    if (vm.count("receive")) {
        if (!vm.count("ndi-source")) {
            std::println("NDI source name is required for receiving MIDI data. Exiting...");
            return 1;
        }

        auto ndi_source_name = vm["ndi-source"].as<std::string>();

        auto midi_output_name = vm["midi-output-name"].as<std::string>();

        return receive(ndi_source_name, midi_output_name) ? 0 : 1;
    }

    if (vm.count("transmit")) {

        if (!vm.count("midi-input")) {
            std::println("MIDI input port name is required for transmitting MIDI data. Exiting...");
            return 1;
        }

        auto midi_input_name = vm["midi-input"].as<std::string>();

        auto ndi_send_name = vm["ndi-send-name"].as<std::string>();

        return transmit(midi_input_name, ndi_send_name) ? 0 : 1;
    }

    std::string input;
    std::println("Would you like to receive (r) or transmit (t) MIDI data via NDI?");
    std::getline(std::cin, input);
    if (input.compare("r") == 0 || input.compare("R") == 0) {
        receiveInteractive();
    } else if (input.compare("t") == 0 || input.compare("T") == 0) {
        transmitInteractive();
    } else {
        std::println("Invalid input. Exiting...");
    }
    return 0;
}