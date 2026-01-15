#pragma once

#include <cstdint>
#include <string>
#include "money.hpp"

/**
 * @brief Модель товара
 */
class Product {
public:
    Product() : id_(0), name_(""), price_(), stock_(0) {}
    Product(std::int32_t id, const std::string& name, const Money& price, std::int32_t stock);
    
    // Геттеры
    std::int32_t GetId() const;
    const std::string& GetName() const;
    Money GetPrice() const;
    std::int32_t GetStock() const;
    
    // Операции со складом
    // Возвращает true если резервирование успешно, false если недостаточно товара
    bool ReserveStock(std::int32_t quantity);
    void ReleaseStock(std::int32_t quantity);
    
private:
    std::int32_t id_;
    std::string name_;
    Money price_;
    std::int32_t stock_;
};
