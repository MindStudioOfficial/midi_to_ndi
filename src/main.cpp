#include "pch.hpp"
#include "ndimidi.hpp"

void receive() {
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

void transmit() {
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

int main() {

    std::println("Would you like to receive (r) or transmit (t) MIDI data via NDI?");

    std::string input;
    std::getline(std::cin, input);

    if (input.compare("r") == 0 || input.compare("R") == 0) {
        receive();
    } else if (input.compare("t") == 0 || input.compare("T") == 0) {
        transmit();
    } else {
        std::println("Invalid input. Exiting...");
    }

    return 0;
}