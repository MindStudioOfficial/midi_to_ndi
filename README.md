# WIP: MIDI to NDI converter

This CLI application converts realtime MIDI IO to NDI metadata frames as specified here:
[sienna-tv.com midi over ndi](https://www.sienna-tv.com/newsite/midioverndi.html)

It has two modes: 
- Receiving MIDI from a selectable NDI source and sending it to a virtual MIDI port using `teVirtualMIDI`
- Receiving MIDI from a MIDI device using `RtMidi` and outputting it as NDI Metadata frames

## NDI Metadata Frames

NDI Metadata frames are a way to send metadata over the network using the NDI protocol.
They can theoretically contain any text data but it usually uses XML to structure the data.

The NDI Metadata frames used in this project are structured as follows:
```xml
<MIDI>E00040</MIDI>
```
Where `E00040` is the hexadecimal MIDI message to be sent.

## Current state of development

The project is currently in a very early stage of development and might not work as expected.

- hardcoded windows NDI binaries: you might need to specify the path to the NDI dlls in `CMakeLists.txt` depending on your system
- hardcoded winmm.lib in `CMakeLists.txt`: For other platforms than Windows you might need to replace this with the appropriate library
- hardcoded virtual MIDI port name and NDI Source name: you might need to change these in the source code to match your circumstances

## Dependencies

- [NDI SDK](https://www.ndi.tv/sdk/) - needs to be installed on your system
- [RtMidi](https://www.music.mcgill.ca/~gary/rtmidi/index.html) - included in the source
- [teVirtualMIDI SDK](https://www.tobias-erichsen.de/software/virtualmidi.html) - needs to be installed on your system

## Building

The project uses CMake to build. You need to have the NDI SDK and teVirtualMIDI SDK installed on your system and the `.lib` files and `.dll` files set in `CMakeLists.txt.`

```bash
cmake -B build -S .
```

```bash
cmake --build build --parallel --config Debug

call .\build\bin\Debug\midi_to_ndi.exe
```
or
```bash
cmake --build build --parallel --config Release

call .\build\bin\Release\midi_to_ndi.exe
```