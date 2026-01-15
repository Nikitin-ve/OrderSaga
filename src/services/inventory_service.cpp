#include "inventory_service.hpp"
#include <sstream>
#include <algorithm>

InventoryService::InventoryService() : simulate_failure_(false) {
    Log("InventoryService инициализирован");
}

void InventoryService::Log(const std::string& message) const {
    operation_log_.push_back("[InventoryService] " + message);
}

ServiceResult InventoryService::CheckFailure(const std::string& operation) const {
    if (simulate_failure_) {
        Log("ОШИБКА: Симуляция сбоя при " + operation);
        return ServiceResult::Fail("Сервис склада недоступен");
    }
    return ServiceResult::Ok();
}

ServiceResult InventoryService::AddProduct(const Product& product) {
    auto check = CheckFailure("добавлении товара");
    if (!check.success) return check;
    
    if (products_.find(product.GetId()) != products_.end()) {
        Log("ПРЕДУПРЕЖДЕНИЕ: Товар " + std::to_string(product.GetId()) + " уже существует, обновление");
    }
    
    products_[product.GetId()] = product;
    
    std::stringstream ss;
    ss << "Добавлен товар: " << product.GetName() 
       << " (ID: " << product.GetId() << ", цена: " << product.GetPrice().ToString() 
       << " руб., остаток: " << product.GetStock() << ")";
    Log(ss.str());
    
    return ServiceResult::Ok("Товар добавлен");
}

std::optional<Product> InventoryService::GetProduct(std::int32_t product_id) const {
    auto it = products_.find(product_id);
    if (it != products_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::int32_t InventoryService::GetStock(std::int32_t product_id) const {
    auto it = products_.find(product_id);
    if (it != products_.end()) {
        return it->second.GetStock();
    }
    return -1;
}

bool InventoryService::CanReserve(std::int32_t product_id, std::int32_t quantity) const {
    if (quantity <= 0) {
        return false;
    }
    auto it = products_.find(product_id);
    if (it == products_.end()) {
        return false;
    }
    return it->second.GetStock() >= quantity;
}

ServiceResult InventoryService::ReserveProduct(std::int32_t order_id, std::int32_t product_id, std::int32_t quantity) {
    auto check = CheckFailure("резервировании товара");
    if (!check.success) return check;
    
    if (!CanReserve(product_id, quantity)) {
        auto it = products_.find(product_id);
        if (it == products_.end()) {
            Log("ОШИБКА: Товар " + std::to_string(product_id) + " не найден");
            return ServiceResult::Fail("Товар не найден");
        }
        std::stringstream ss;
        ss << "ОШИБКА: Недостаточно товара " << it->second.GetName() 
           << " (требуется: " << quantity << ", доступно: " << it->second.GetStock() << ")";
        Log(ss.str());
        return ServiceResult::Fail("Недостаточно товара на складе");
    }
    
    auto it = products_.find(product_id);
    it->second.ReserveStock(quantity);
    
    reservations_.emplace_back(order_id, product_id, quantity);
    
    std::stringstream ss;
    ss << "Зарезервировано " << quantity << " шт. товара " << it->second.GetName() 
       << " для заказа " << order_id;
    Log(ss.str());
    
    return ServiceResult::Ok("Товар зарезервирован");
}

ServiceResult InventoryService::ConfirmReservation(std::int32_t order_id, std::int32_t product_id) {
    auto check = CheckFailure("подтверждении резервирования");
    if (!check.success) return check;
    
    auto it = FindReservation(order_id, product_id);
    if (it != reservations_.end()) {
        it->confirmed = true;
        
        std::stringstream ss;
        ss << "Подтверждена резервация товара " << product_id << " для заказа " << order_id;
        Log(ss.str());
    }
    
    return ServiceResult::Ok("Резервирование подтверждено");
}

ServiceResult InventoryService::ReleaseReservation(std::int32_t order_id, std::int32_t product_id) {
    // Компенсация не зависит от симуляции сбоя
    auto it = FindReservation(order_id, product_id);
    if (it != reservations_.end()) {
        auto prod_it = products_.find(it->product_id);
        if (prod_it != products_.end()) {
            prod_it->second.ReleaseStock(it->quantity);
            
            std::stringstream ss;
            ss << "Освобождено " << it->quantity << " шт. товара " << prod_it->second.GetName() 
               << " для заказа " << order_id << " (компенсация)";
            Log(ss.str());
        }
        
        reservations_.erase(it);
    }
    
    return ServiceResult::Ok("Резервирование отменено");
}

ServiceResult InventoryService::ReleaseAllReservations(std::int32_t order_id) {
    // Компенсация не зависит от симуляции сбоя
    auto it = reservations_.begin();
    while (it != reservations_.end()) {
        if (it->order_id == order_id) {
            auto prod_it = products_.find(it->product_id);
            if (prod_it != products_.end()) {
                prod_it->second.ReleaseStock(it->quantity);
                
                std::stringstream ss;
                ss << "Освобождено " << it->quantity << " шт. товара " << prod_it->second.GetName() 
                   << " для заказа " << order_id << " (компенсация)";
                Log(ss.str());
            }
            it = reservations_.erase(it);
        } else {
            ++it;
        }
    }
    
    return ServiceResult::Ok("Все резервирования отменены");
}

std::vector<Reservation>::iterator InventoryService::FindReservation(std::int32_t order_id, std::int32_t product_id) {
    return std::find_if(reservations_.begin(), reservations_.end(),
        [order_id, product_id](const Reservation& res) {
            return res.order_id == order_id && res.product_id == product_id;
        });
}

void InventoryService::SetSimulateFailure(bool fail) {
    simulate_failure_ = fail;
    if (fail) {
        Log("ВНИМАНИЕ: Включена симуляция сбоя");
    } else {
        Log("Симуляция сбоя отключена");
    }
}

bool InventoryService::IsSimulatingFailure() const {
    return simulate_failure_;
}

const std::vector<std::string>& InventoryService::GetOperationLog() const {
    return operation_log_;
}

void InventoryService::ClearOperationLog() {
    operation_log_.clear();
}

std::vector<Product> InventoryService::GetAllProducts() const {
    std::vector<Product> result;
    for (const auto& [id, product] : products_) {
        result.push_back(product);
    }
    return result;
}
