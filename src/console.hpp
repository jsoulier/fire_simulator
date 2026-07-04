#pragma once

#include <mutex>
#include <string>
#include <vector>

class Console
{
public:
    Console();
    void RenderImGui();

private:
    std::vector<std::string> Messages;
    std::mutex Mutex;
};
