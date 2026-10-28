#pragma once
#include "../game/addresses.h"
#include "../core/settings.h"
#include "../core/pattern.h"
#include "../vendor/spoofcall/RetSpoof.hpp"
#include <ws2tcpip.h>
#include <intrin.h>
#pragma comment(lib, "ws2_32.lib")

#define TraceLog(...) do { if (RR::Config::EnableTracing) PatchLog(__VA_ARGS__); } while (0)
