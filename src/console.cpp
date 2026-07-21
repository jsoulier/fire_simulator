#include <SDL3/SDL.h>
#include <imgui.h>
#include <savepoint/savepoint.hpp>
#include <spdlog/sinks/callback_sink.h>
#include <spdlog/spdlog.h>

#include <memory>
#include <mutex>
#include <string>
#include <utility>

#include "console.hpp"

Console::Console()
{
    SDL_SetLogOutputFunction([](void* userdata, int category, SDL_LogPriority priority, const char* text)
    {
        spdlog::info("{}", text);
    }, nullptr);
    SavepointSetLogFunction([](const std::string_view& text)
    {
        spdlog::info("{}", text);
    });
    auto sink = std::make_shared<spdlog::sinks::callback_sink_mt>([this](const spdlog::details::log_msg& text)
    {
        std::scoped_lock lock(Mutex);
        Messages.emplace_back(text.payload.data(), text.payload.size());
    });
    spdlog::default_logger()->sinks().push_back(sink);
}

void Console::RenderImGui()
{
    std::scoped_lock lock(Mutex);
    ImGuiListClipper clipper;
    clipper.Begin(Messages.size());
    while (clipper.Step())
    {
        for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
        {
            ImGui::TextUnformatted(Messages[Messages.size() - 1 - i].c_str());
        }
    }
}
