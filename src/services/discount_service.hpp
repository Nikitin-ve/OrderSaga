#pragma once

#include <cstdint>
#include <unordered_map>
#include <string>
#include <vector>
#include "../common/types.hpp"
#include "../models/money.hpp"

/**
 * @brief Информация о скидке пользователя
 */
struct DiscountInfo {
    std::int32_t user_id;
    double discount_percent;
    bool is_vip;
    std::int32_t orders_count;
    
    DiscountInfo() : user_id(0), discount_percent(0.0), is_vip(false), orders_count(0) {}
    DiscountInfo(std::int32_t id, double discount, bool vip, std::int32_t orders)
        : user_id(id), discount_percent(discount), is_vip(vip), orders_count(orders) {}
};

/**
 * @brief Сервис управления скидками
 */
class DiscountService {
public:
    DiscountService();
    
    // Геттеры (доступны даже при симуляции сбоя)
    DiscountInfo GetUserDiscount(std::int32_t user_id) const;
    double CalculateOrderDiscount(std::int32_t user_id, const Money& order_amount) const;
    double GetAppliedDiscount(std::int32_t order_id) const;
    const std::vector<std::string>& GetOperationLog() const;
    bool IsSimulatingFailure() const;
    
    // Модифицирующие методы (недоступны при симуляции сбоя)
    ServiceResult SetUserDiscount(std::int32_t user_id, double discount_percent);
    ServiceResult SetVipStatus(std::int32_t user_id, bool is_vip);
    ServiceResult IncrementOrderCount(std::int32_t user_id);
    ServiceResult DecrementOrderCount(std::int32_t user_id);
    ServiceResult ApplyDiscount(std::int32_t user_id, std::int32_t order_id, const Money& order_amount);
    ServiceResult CancelDiscount(std::int32_t user_id, std::int32_t order_id);
    
    void SetSimulateFailure(bool fail);
    void ClearOperationLog();
    
private:
    std::unordered_map<std::int32_t, DiscountInfo> user_discounts_;
    std::unordered_map<std::int32_t, double> applied_discounts_;
    bool simulate_failure_;
    mutable std::vector<std::string> operation_log_;
    
    void Log(const std::string& message) const;
    ServiceResult CheckFailure(const std::string& operation) const;
    
    static constexpr double kVipBonus = 5.0;
    static constexpr double kOrderBonusPer10 = 2.0;
    static constexpr double kMaxDiscount = 30.0;
    static constexpr double kLargeOrderBonus = 3.0;
    static constexpr std::int64_t kLargeOrderThresholdKopecks = 1000000; // 10000 рублей
};
