// SPDX-License-Identifier: GPL-2.0-or-later
#include <cstdlib>
extern "C" const char *SpiralPlugin_GetHostABI() { return "ssm-fltk-audio-6"; }
// Old virtual interfaces must be rejected before this factory can be called.
extern "C" void *SpiralPlugin_CreateInstance() { abort(); }
