#include "money.hpp"
#include <sstream>
#include <iomanip>
#include <cmath>

Money::Money() : kopecks_(0) {}

Money::Money(std::int64_t kopecks) : kopecks_(kopecks) {
    if (kopecks < 0) {
        throw std::invalid_argument("Money amount cannot be negative");
    }
}

Money Money::FromRubles(std::int64_t rubles) {
    if (rubles < 0) {
        throw std::invalid_argument("Rubles amount cannot be negative");
    }
    return Money(rubles * kKopecksPerRuble);
}

Money Money::FromRublesAndKopecks(std::int64_t rubles, std::int64_t kopecks) {
    if (rubles < 0 || kopecks < 0) {
        throw std::invalid_argument("Money amount cannot be negative");
    }
    if (kopecks >= 100) {
        throw std::invalid_argument("Kopecks must be less than 100");
    }
    return Money(rubles * kKopecksPerRuble + kopecks);
}

std::int64_t Money::GetKopecks() const { 
    return kopecks_; 
}

std::int64_t Money::GetRubles() const { 
    return kopecks_ / kKopecksPerRuble; 
}

std::int64_t Money::GetRemainingKopecks() const { 
    return kopecks_ % kKopecksPerRuble; 
}

std::string Money::ToString() const {
    std::ostringstream oss;
    oss << GetRubles() << "." << std::setfill('0') << std::setw(2) << GetRemainingKopecks();
    return oss.str();
}

Money Money::operator+(const Money& other) const {
    return Money(kopecks_ + other.kopecks_);
}

Money Money::operator-(const Money& other) const {
    if (kopecks_ < other.kopecks_) {
        throw std::invalid_argument("Cannot subtract: result would be negative");
    }
    return Money(kopecks_ - other.kopecks_);
}

Money& Money::operator+=(const Money& other) {
    kopecks_ += other.kopecks_;
    return *this;
}

Money& Money::operator-=(const Money& other) {
    if (kopecks_ < other.kopecks_) {
        throw std::invalid_argument("Cannot subtract: result would be negative");
    }
    kopecks_ -= other.kopecks_;
    return *this;
}

Money Money::operator*(std::int64_t multiplier) const {
    if (multiplier < 0) {
        throw std::invalid_argument("Multiplier cannot be negative");
    }
    return Money(kopecks_ * multiplier);
}

Money Money::ApplyDiscount(double discount_percent) const {
    if (discount_percent < 0 || discount_percent > 100) {
        throw std::invalid_argument("Discount percent must be between 0 and 100");
    }
    double multiplier = (100.0 - discount_percent) / 100.0;
    return Money(static_cast<std::int64_t>(std::round(kopecks_ * multiplier)));
}

bool Money::operator==(const Money& other) const { return kopecks_ == other.kopecks_; }
bool Money::operator!=(const Money& other) const { return kopecks_ != other.kopecks_; }
bool Money::operator<(const Money& other) const { return kopecks_ < other.kopecks_; }
bool Money::operator<=(const Money& other) const { return kopecks_ <= other.kopecks_; }
bool Money::operator>(const Money& other) const { return kopecks_ > other.kopecks_; }
bool Money::operator>=(const Money& other) const { return kopecks_ >= other.kopecks_; }

bool Money::IsZero() const { return kopecks_ == 0; }
bool Money::IsPositive() const { return kopecks_ > 0; }
bool Money::IsNegative() const { return kopecks_ < 0; }

Money operator*(std::int64_t multiplier, const Money& money) {
    return money * multiplier;
}

