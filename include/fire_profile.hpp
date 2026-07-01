#pragma once

#if USE_TRACY
#include <tracy/Tracy.hpp>
#define FireProfileTag() ZoneScoped
#define FireProfileTagBlock(name) ZoneScopedN(name)
#define FireProfilePlot(name, value) TracyPlot(name, value)
#define FireProfileFrame() FrameMark
#else
#define FireProfileTag()
#define FireProfileTagBlock(name)
#define FireProfilePlot(name, value)
#define FireProfileFrame()
#endif
