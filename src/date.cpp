#include <chrono>
#include <format>
#include <sstream>
#include <string>

#include "date.hpp"

Date::Date()
    : Days{}
{
}

Date::Date(std::chrono::sys_days days)
    : Days{days}
{
}

Date::Date(int year, int month, int day)
    : Date{std::chrono::year_month_day{std::chrono::year(year), std::chrono::month(month), std::chrono::day(day)}}
{
}

Date::Date(const std::string& text)
{
    std::istringstream stream(text);
    stream >> std::chrono::parse("%F", Days);
}

Date Date::operator+(int days) const
{
    return Date(Days + std::chrono::days(days));
}

int Date::operator-(const Date& other) const
{
    return int((Days - other.Days).count());
}

double Date::ToEpoch(int hm) const
{
    return (Days + std::chrono::hours(hm / 100) + std::chrono::minutes(hm % 100)).time_since_epoch().count();
}

std::string Date::ToString() const
{
    return std::format("{:%Y-%m-%d}", Days);
}
