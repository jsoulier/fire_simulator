#include <chrono>
#include <format>
#include <sstream>
#include <string>

#include "date.hpp"

Date::Date()
    : Time{}
{
}

Date::Date(std::chrono::sys_days days)
    : Time{days}
{
}

Date::Date(int year, int month, int day, int hm)
    : Time{std::chrono::sys_days{std::chrono::year{year} / month / day}
    + std::chrono::hours(hm / 100)
    + std::chrono::minutes(hm % 100)}
{
}

Date::Date(const std::string& text)
{
    std::istringstream stream(text);
    if (text.size() == 10)
    {
        std::chrono::sys_days days;
        stream >> std::chrono::parse("%F", days);
        Time = days;
    }
    else
    {
        stream >> std::chrono::parse("%FT%R", Time);
    }
}

Date::Date(const std::tm& tm)
    : Date{tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday}
{
}

Date Date::AddDays(int days) const
{
    return Date(std::chrono::floor<std::chrono::days>(Time) + std::chrono::days(days));
}

int Date::GetDaysBetween(const Date& other) const
{
    return int((std::chrono::floor<std::chrono::days>(other.Time) - std::chrono::floor<std::chrono::days>(Time)).count());
}

double Date::ToEpoch() const
{
    return Time.time_since_epoch().count();
}

std::string Date::ToString() const
{
    return std::format("{:%Y-%m-%d}", std::chrono::floor<std::chrono::days>(Time));
}

std::tm Date::ToTm() const
{
    std::chrono::year_month_day ymd{std::chrono::floor<std::chrono::days>(Time)};
    std::tm tm{};
    tm.tm_year = int(ymd.year()) - 1900;
    tm.tm_mon = unsigned(ymd.month()) - 1;
    tm.tm_mday = unsigned(ymd.day());
    return tm;
}
