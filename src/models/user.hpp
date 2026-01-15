#pragma once

#include <cstdint>
#include <string>
#include "money.hpp"

/**
 * @brief Модель пользователя системы
 */
class User {
public:
    User() : id_(0), name_(""), balance_() {}
    User(std::int32_t id, const std::string& name, const Money& balance);
    
    // Геттеры
    std::int32_t GetId() const;
    const std::string& GetName() const;
    Money GetBalance() const;
    
    // Операции с балансом
    void DeductBalance(const Money& amount);
    void AddBalance(const Money& amount);
    
private:
    std::int32_t id_;
    std::string name_;
    Money balance_;
};
