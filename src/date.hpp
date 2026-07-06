#pragma once

#include <chrono>
#include <string>

class Date
{
public:
    Date();
    explicit Date(std::chrono::sys_days days);
    Date(int year, int month, int day);
    explicit Date(const std::string& text); // YYYY-MM-DD
    Date operator+(int days) const;
    int operator-(const Date& other) const;
    double ToEpoch(int hm = 0) const;
    std::string ToString() const;

private:
    std::chrono::sys_days Days;
};
