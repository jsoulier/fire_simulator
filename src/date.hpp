#pragma once

#include <chrono>
#include <ctime>
#include <string>

class Date
{
public:
    Date();
    explicit Date(std::chrono::sys_days days);
    Date(int year, int month, int day, int hm = 0);
    explicit Date(const std::string& text); // YYYY-MM-DD or YYYY-MM-DDTHH:MM
    explicit Date(const std::tm& tm);
    Date AddDays(int days) const;
    int GetDaysBetween(const Date& other) const;
    double ToEpoch() const;
    std::string ToString() const;
    std::tm ToTm() const;

private:
    std::chrono::sys_time<std::chrono::minutes> Time;
};
