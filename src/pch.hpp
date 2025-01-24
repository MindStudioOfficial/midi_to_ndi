#pragma once

#include <teVirtualMIDI.h>

#include <Processing.NDI.Lib.h>

#ifdef _DEBUG
#define RTMIDI_DEBUG
#endif

#define __WINDOWS_MM__

#include "RtMidi.h"

#include <cstdlib>
#include <print>
#include <string>
#include <format>
#include <memory>
#include <vector>
#include <span>
#include <optional>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <chrono>

#include <conio.h>