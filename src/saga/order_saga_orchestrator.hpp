#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <functional>
#include "../models/order.hpp"
#include "../common/types.hpp"
#include "../services/discount_service.hpp"
#include "../services/billing_service.hpp"
#include "../services/inventory_service.hpp"
#include "../services/notification_service.hpp"

/**
 * @brief Шаг Saga
 */
struct SagaStep {
    std::string name;
    std::function<ServiceResult()> execute;
    std::function<void()> compensate;
    bool executed;
    bool succeeded;
    
    SagaStep(const std::string& n, 
             std::function<ServiceResult()> exec,
             std::function<void()> comp)
        : name(n), execute(std::move(exec)), compensate(std::move(comp)),
          executed(false), succeeded(false) {}
};

/**
 * @brief Оркестратор Saga для управления заказами
 * 
 * Реализует паттерн Saga (оркестратор) для координации
 * распределённых транзакций между сервисами
 */
class OrderSagaOrchestrator {
public:
    OrderSagaOrchestrator(BillingService& billing,
                         InventoryService& inventory,
                         DiscountService& discount,
                         NotificationService& notification);
    
    bool ExecuteSaga(Order& order);
    
    void CompensateSaga(Order& order);
    
    const std::vector<std::string>& GetExecutionLog() const;
    void ClearExecutionLog();
    
private:
    BillingService& billing_;
    InventoryService& inventory_;
    DiscountService& discount_;
    NotificationService& notification_;
    
    mutable std::vector<std::string> execution_log_;
    
    std::vector<SagaStep> BuildSagaSteps(Order& order);
    
    void RunCompensation(std::vector<SagaStep>& steps, std::int32_t from_step);
    
    void Log(const std::string& message) const;
};
