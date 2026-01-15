#pragma once

#include <cstdint>
#include <unordered_map>
#include <string>
#include <vector>
#include <optional>
#include "../models/product.hpp"
#include "../common/types.hpp"

/**
 * @brief Информация о резервировании товара
 */
struct Reservation {
    std::int32_t order_id;
    std::int32_t product_id;
    std::int32_t quantity;
    bool confirmed;
    
    Reservation(std::int32_t o_id, std::int32_t p_id, std::int32_t qty)
        : order_id(o_id), product_id(p_id), quantity(qty), confirmed(false) {}
};

/**
 * @brief Сервис управления складом (инвентаризации)
 */
class InventoryService {
public:
    InventoryService();
    
    // Геттеры (доступны даже при симуляции сбоя)
    std::optional<Product> GetProduct(std::int32_t product_id) const;
    std::int32_t GetStock(std::int32_t product_id) const;  // -1 если товар не найден
    bool CanReserve(std::int32_t product_id, std::int32_t quantity) const;
    std::vector<Product> GetAllProducts() const;
    const std::vector<std::string>& GetOperationLog() const;
    bool IsSimulatingFailure() const;
    
    // Модифицирующие методы (недоступны при симуляции сбоя)
    ServiceResult AddProduct(const Product& product);
    ServiceResult ReserveProduct(std::int32_t order_id, std::int32_t product_id, std::int32_t quantity);
    ServiceResult ConfirmReservation(std::int32_t order_id, std::int32_t product_id);
    ServiceResult ReleaseReservation(std::int32_t order_id, std::int32_t product_id);
    ServiceResult ReleaseAllReservations(std::int32_t order_id);
    
    void SetSimulateFailure(bool fail);
    void ClearOperationLog();
    
private:
    std::unordered_map<std::int32_t, Product> products_;
    std::vector<Reservation> reservations_;
    bool simulate_failure_;
    mutable std::vector<std::string> operation_log_;
    
    void Log(const std::string& message) const;
    ServiceResult CheckFailure(const std::string& operation) const;
    std::vector<Reservation>::iterator FindReservation(std::int32_t order_id, std::int32_t product_id);
};
