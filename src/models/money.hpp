#pragma once

#include <cstdint>
#include <string>

/**
 * @brief Класс для работы с денежными суммами
 * 
 * Хранит сумму в копейках (int64_t) для избежания ошибок округления.
 * Поддерживает арифметические операции и форматированный вывод.
 */
class Money {
public:
    // Конструкторы
    Money();
    explicit Money(std::int64_t kopecks);
    
    // Фабричные методы
    static Money FromRubles(std::int64_t rubles);
    static Money FromRublesAndKopecks(std::int64_t rubles, std::int64_t kopecks);
    
    // Геттеры
    std::int64_t GetKopecks() const;
    std::int64_t GetRubles() const;
    std::int64_t GetRemainingKopecks() const;
    
    // Форматированный вывод
    std::string ToString() const;
    
    // Арифметические операции
    Money operator+(const Money& other) const;
    Money operator-(const Money& other) const;
    Money& operator+=(const Money& other);
    Money& operator-=(const Money& other);
    Money operator*(std::int64_t multiplier) const;
    
    // Применить процентную скидку
    Money ApplyDiscount(double discount_percent) const;
    
    // Операторы сравнения
    bool operator==(const Money& other) const;
    bool operator!=(const Money& other) const;
    bool operator<(const Money& other) const;
    bool operator<=(const Money& other) const;
    bool operator>(const Money& other) const;
    bool operator>=(const Money& other) const;
    
    // Проверки
    bool IsZero() const;
    bool IsPositive() const;
    bool IsNegative() const;
    
private:
    std::int64_t kopecks_;
    
    static constexpr std::int64_t kKopecksPerRuble = 100;
};

// Умножение: количество * цена
Money operator*(std::int64_t multiplier, const Money& money);
