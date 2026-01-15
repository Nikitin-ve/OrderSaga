#include "test_framework.hpp"
#include "../src/services/order_service.hpp"

struct TestEnvironment {
    BillingService billing_service;
    InventoryService inventory_service;
    DiscountService discount_service;
    NotificationService notification_service;
    std::unique_ptr<OrderService> order_service;
    
    TestEnvironment() {
        order_service = std::make_unique<OrderService>(
            billing_service, inventory_service, discount_service, notification_service);
        
        // Пользователи с балансом в рублях
        billing_service.AddUser(User(1, "Иван Петров", Money::FromRubles(100000)));
        billing_service.AddUser(User(2, "Мария Сидорова", Money::FromRubles(50000)));
        billing_service.AddUser(User(3, "Бедный Студент", Money::FromRubles(500)));
        
        // Товары с ценами в рублях
        inventory_service.AddProduct(Product(1, "Ноутбук", Money::FromRubles(45000), 5));
        inventory_service.AddProduct(Product(2, "Телефон", Money::FromRubles(30000), 10));
        inventory_service.AddProduct(Product(3, "Наушники", Money::FromRubles(8000), 25));
        inventory_service.AddProduct(Product(4, "Клавиатура", Money::FromRubles(3500), 30));
        inventory_service.AddProduct(Product(5, "Мышь", Money::FromRubles(2500), 50));
        
        // Скидки
        discount_service.SetUserDiscount(1, 10.0); // 10% скидка
        discount_service.SetVipStatus(2, true);    // VIP +5%
    }
};

TEST(Saga_SuccessfulOrder) {
    TestEnvironment env;
    
    // Товары: наушники (8000) + клавиатура (3500) = 11500 руб.
    // Скидка: 10% (пользователь) + 3% (большой заказ > 10000) = 13%
    // Итого: 11500 * 0.87 = 10005 руб.
    std::vector<std::pair<std::int32_t, std::int32_t>> items = {{3, 1}, {4, 1}};
    
    auto result = env.order_service->CreateOrder(1, items);
    ASSERT_TRUE(result.success);
    ASSERT_EQ(1, result.order_id);
    
    auto process_result = env.order_service->ProcessOrder(result.order_id);
    ASSERT_TRUE(process_result.success);
    
    auto order = env.order_service->GetOrder(result.order_id);
    ASSERT_TRUE(order.has_value());
    ASSERT_TRUE(order->GetStatus() == OrderStatus::kConfirmed);
    
    ASSERT_EQ(1150000, order->GetTotalPrice().GetKopecks());   // 11500 руб.
    ASSERT_NEAR(13.0, order->GetDiscountPercent(), 0.01);
    ASSERT_EQ(1000500, order->GetFinalPrice().GetKopecks());   // 10005 руб.
    
    // Баланс: 100000 - 10005 = 89995 руб.
    auto balance = env.billing_service.GetBalance(1);
    ASSERT_TRUE(balance.has_value());
    ASSERT_EQ(8999500, balance->GetKopecks());
    
    // Склад
    ASSERT_EQ(24, env.inventory_service.GetStock(3));
    ASSERT_EQ(29, env.inventory_service.GetStock(4));
}

TEST(Saga_InsufficientFunds_Compensation) {
    TestEnvironment env;
    
    // Студент с 500 руб. пытается купить наушники за 8000 руб.
    std::vector<std::pair<std::int32_t, std::int32_t>> items = {{3, 1}};
    
    auto result = env.order_service->CreateOrder(3, items);
    ASSERT_TRUE(result.success);
    
    auto process_result = env.order_service->ProcessOrder(result.order_id);
    ASSERT_FALSE(process_result.success);
    
    auto order = env.order_service->GetOrder(result.order_id);
    ASSERT_TRUE(order.has_value());
    ASSERT_TRUE(order->GetStatus() == OrderStatus::kFailed);
    
    // Баланс не изменился
    auto balance = env.billing_service.GetBalance(3);
    ASSERT_EQ(50000, balance->GetKopecks()); // 500 руб.
    // Товар вернулся на склад
    ASSERT_EQ(25, env.inventory_service.GetStock(3));
}

TEST(Saga_InsufficientStock) {
    TestEnvironment env;
    
    std::vector<std::pair<std::int32_t, std::int32_t>> items = {{1, 100}};
    
    auto result = env.order_service->CreateOrder(1, items);
    ASSERT_TRUE(result.success);
    
    auto process_result = env.order_service->ProcessOrder(result.order_id);
    ASSERT_FALSE(process_result.success);
    
    // Баланс не тронут
    auto balance = env.billing_service.GetBalance(1);
    ASSERT_EQ(10000000, balance->GetKopecks()); // 100000 руб.
    // Склад не тронут
    ASSERT_EQ(5, env.inventory_service.GetStock(1));
}

TEST(Saga_BillingFailure_Compensation) {
    TestEnvironment env;
    
    env.billing_service.SetSimulateFailure(true);
    
    std::vector<std::pair<std::int32_t, std::int32_t>> items = {{3, 2}};
    
    auto result = env.order_service->CreateOrder(1, items);
    ASSERT_TRUE(result.success);
    
    auto process_result = env.order_service->ProcessOrder(result.order_id);
    ASSERT_FALSE(process_result.success);
    
    // Товар вернулся на склад после компенсации
    ASSERT_EQ(25, env.inventory_service.GetStock(3));
    // Баланс не тронут
    auto balance = env.billing_service.GetBalance(1);
    ASSERT_EQ(10000000, balance->GetKopecks());
}

TEST(Saga_InventoryFailure_Compensation) {
    TestEnvironment env;
    
    env.inventory_service.SetSimulateFailure(true);
    
    std::vector<std::pair<std::int32_t, std::int32_t>> items = {{3, 1}};
    
    auto result = env.order_service->CreateOrder(1, items);
    ASSERT_TRUE(result.success);
    
    auto process_result = env.order_service->ProcessOrder(result.order_id);
    ASSERT_FALSE(process_result.success);
    
    // Скидка не применена
    ASSERT_NEAR(0.0, env.discount_service.GetAppliedDiscount(result.order_id), 0.01);
}

TEST(Saga_DiscountFailure) {
    TestEnvironment env;
    
    env.discount_service.SetSimulateFailure(true);
    
    std::vector<std::pair<std::int32_t, std::int32_t>> items = {{4, 1}};
    
    auto result = env.order_service->CreateOrder(1, items);
    ASSERT_TRUE(result.success);
    
    auto process_result = env.order_service->ProcessOrder(result.order_id);
    ASSERT_FALSE(process_result.success);
    
    // Баланс не тронут
    auto balance = env.billing_service.GetBalance(1);
    ASSERT_EQ(10000000, balance->GetKopecks());
    // Товар вернулся на склад
    ASSERT_EQ(30, env.inventory_service.GetStock(4));
}

TEST(Saga_CancelConfirmedOrder) {
    TestEnvironment env;
    
    std::int64_t initial_balance = env.billing_service.GetBalance(1)->GetKopecks();
    
    std::vector<std::pair<std::int32_t, std::int32_t>> items = {{3, 2}, {4, 3}};
    
    auto result = env.order_service->CreateOrder(1, items);
    ASSERT_TRUE(result.success);
    env.order_service->ProcessOrder(result.order_id);
    
    auto cancel_result = env.order_service->CancelOrder(result.order_id);
    ASSERT_TRUE(cancel_result.success);
    
    auto order = env.order_service->GetOrder(result.order_id);
    ASSERT_TRUE(order.has_value());
    ASSERT_TRUE(order->GetStatus() == OrderStatus::kCancelled);
    
    // Баланс полностью вернулся к начальному значению
    auto balance = env.billing_service.GetBalance(1);
    ASSERT_EQ(initial_balance, balance->GetKopecks());
}

TEST(Saga_VipDiscount) {
    TestEnvironment env;
    
    // VIP пользователь (user_id=2) получает +5% скидку
    // Товар: 5 клавиатур по 3500 = 17500 руб.
    // Скидка: 5% (VIP) + 3% (большой заказ > 10000) = 8%
    // Итого: 17500 * 0.92 = 16100 руб.
    std::vector<std::pair<std::int32_t, std::int32_t>> items = {{4, 5}};
    
    auto result = env.order_service->CreateOrder(2, items);
    ASSERT_TRUE(result.success);
    
    auto process_result = env.order_service->ProcessOrder(result.order_id);
    ASSERT_TRUE(process_result.success);
    
    auto order = env.order_service->GetOrder(result.order_id);
    ASSERT_TRUE(order.has_value());
    
    ASSERT_NEAR(8.0, order->GetDiscountPercent(), 0.01);
    ASSERT_EQ(1610000, order->GetFinalPrice().GetKopecks()); // 16100 руб.
}

TEST(Saga_MultipleOrders) {
    TestEnvironment env;
    
    std::vector<std::pair<std::int32_t, std::int32_t>> items1 = {{4, 5}};
    auto result1 = env.order_service->CreateOrder(1, items1);
    ASSERT_TRUE(env.order_service->ProcessOrder(result1.order_id).success);
    
    std::vector<std::pair<std::int32_t, std::int32_t>> items2 = {{4, 3}};
    auto result2 = env.order_service->CreateOrder(2, items2);
    ASSERT_TRUE(env.order_service->ProcessOrder(result2.order_id).success);
    
    // 30 - 5 - 3 = 22
    ASSERT_EQ(22, env.inventory_service.GetStock(4));
    
    auto all_orders = env.order_service->GetAllOrders();
    ASSERT_EQ(2, static_cast<std::int32_t>(all_orders.size()));
}

TEST(Saga_GetUserOrders) {
    TestEnvironment env;
    
    // Создаём заказы для пользователя 1
    std::vector<std::pair<std::int32_t, std::int32_t>> items1 = {{4, 1}};
    auto result1 = env.order_service->CreateOrder(1, items1);
    env.order_service->ProcessOrder(result1.order_id);
    
    std::vector<std::pair<std::int32_t, std::int32_t>> items2 = {{3, 1}};
    auto result2 = env.order_service->CreateOrder(1, items2);
    env.order_service->ProcessOrder(result2.order_id);
    
    // Создаём заказ для пользователя 2
    std::vector<std::pair<std::int32_t, std::int32_t>> items3 = {{4, 1}};
    auto result3 = env.order_service->CreateOrder(2, items3);
    env.order_service->ProcessOrder(result3.order_id);
    
    auto user1_orders = env.order_service->GetUserOrders(1);
    ASSERT_EQ(2, static_cast<std::int32_t>(user1_orders.size()));
    
    bool found_order1 = false, found_order2 = false;
    for (const auto& order : user1_orders) {
        if (order.GetId() == result1.order_id) found_order1 = true;
        if (order.GetId() == result2.order_id) found_order2 = true;
        ASSERT_EQ(1, order.GetUserId());
    }
    ASSERT_TRUE(found_order1);
    ASSERT_TRUE(found_order2);
    
    auto user2_orders = env.order_service->GetUserOrders(2);
    ASSERT_EQ(1, static_cast<std::int32_t>(user2_orders.size()));
    ASSERT_EQ(result3.order_id, user2_orders[0].GetId());
    ASSERT_EQ(2, user2_orders[0].GetUserId());
}

TEST(Saga_NotificationsSent) {
    TestEnvironment env;
    
    std::vector<std::pair<std::int32_t, std::int32_t>> items = {{4, 1}};
    auto result = env.order_service->CreateOrder(1, items);
    env.order_service->ProcessOrder(result.order_id);
    
    auto notifications = env.notification_service.GetNotificationsForOrder(result.order_id);
    ASSERT_GE(static_cast<std::int32_t>(notifications.size()), 2);
    

    bool has_created = false, has_confirmed = false;
    for (const auto& n : notifications) {
        if (n.type == NotificationType::kOrderCreated) {
            has_created = true;
            ASSERT_EQ(1, n.user_id);
            ASSERT_EQ(result.order_id, n.order_id);
        }
        if (n.type == NotificationType::kOrderConfirmed) {
            has_confirmed = true;
            ASSERT_EQ(1, n.user_id);
            ASSERT_EQ(result.order_id, n.order_id);
        }
    }
    ASSERT_TRUE(has_created);
    ASSERT_TRUE(has_confirmed);
}

TEST(Saga_FailureNotification) {
    TestEnvironment env;
    
    env.billing_service.SetSimulateFailure(true);
    
    std::vector<std::pair<std::int32_t, std::int32_t>> items = {{4, 1}};
    auto result = env.order_service->CreateOrder(1, items);
    env.order_service->ProcessOrder(result.order_id);
    
    auto notifications = env.notification_service.GetNotificationsForOrder(result.order_id);
    
    bool has_failure_notification = false;
    for (const auto& n : notifications) {
        if (n.type == NotificationType::kOrderFailed) {
            has_failure_notification = true;
            ASSERT_EQ(1, n.user_id);
            ASSERT_EQ(result.order_id, n.order_id);
            break;
        }
    }
    ASSERT_TRUE(has_failure_notification);
}

TEST(Saga_OrderCountIncrement) {
    TestEnvironment env;
    
    DiscountInfo info_before = env.discount_service.GetUserDiscount(1);
    
    std::vector<std::pair<std::int32_t, std::int32_t>> items = {{4, 1}};
    auto result = env.order_service->CreateOrder(1, items);
    env.order_service->ProcessOrder(result.order_id);
    
    DiscountInfo info_after = env.discount_service.GetUserDiscount(1);
    ASSERT_EQ(info_before.orders_count + 1, info_after.orders_count);
}

TEST(Saga_DataConsistency) {
    TestEnvironment env;
    
    std::int64_t initial_balance = env.billing_service.GetBalance(1)->GetKopecks();
    std::int32_t initial_stock = env.inventory_service.GetStock(4);
    
    // Заказ 1: 5 клавиатур по 3500 = 17500
    // Скидка: 10% + 3% (большой заказ) = 13%
    // Итого: 17500 * 0.87 = 15225 руб.
    std::vector<std::pair<std::int32_t, std::int32_t>> items1 = {{4, 5}};
    auto result1 = env.order_service->CreateOrder(1, items1);
    env.order_service->ProcessOrder(result1.order_id);
    
    // Заказ 2: сбой биллинга - откат
    env.billing_service.SetSimulateFailure(true);
    std::vector<std::pair<std::int32_t, std::int32_t>> items2 = {{4, 3}};
    auto result2 = env.order_service->CreateOrder(1, items2);
    env.order_service->ProcessOrder(result2.order_id);
    env.billing_service.SetSimulateFailure(false);
    
    // Заказ 3: 2 клавиатуры по 3500 = 7000
    // Скидка: 10% (< 10000 - нет бонуса)
    // Итого: 7000 * 0.90 = 6300 руб.
    std::vector<std::pair<std::int32_t, std::int32_t>> items3 = {{4, 2}};
    auto result3 = env.order_service->CreateOrder(1, items3);
    env.order_service->ProcessOrder(result3.order_id);
    
    // Проверка склада: 30 - 5 - 2 = 23 (заказ 2 откатился)
    ASSERT_EQ(initial_stock - 7, env.inventory_service.GetStock(4));
    
    // Проверка баланса: 100000 - 15225 - 6300 = 78475 руб.
    std::int64_t expected_balance = initial_balance - 1522500 - 630000;
    auto balance = env.billing_service.GetBalance(1);
    ASSERT_EQ(expected_balance, balance->GetKopecks());
}

TEST(Saga_PrecisionTest) {
    TestEnvironment env;
    
    env.inventory_service.AddProduct(
        Product(100, "Товар с копейками", Money::FromRublesAndKopecks(99, 99), 10));
    
    std::vector<std::pair<std::int32_t, std::int32_t>> items = {{100, 3}};
    auto result = env.order_service->CreateOrder(1, items);
    
    auto order = env.order_service->GetOrder(result.order_id);
    ASSERT_TRUE(order.has_value());
    
    // 3 * 99.99 = 299.97 руб. = 29997 копеек
    ASSERT_EQ(29997, order->GetTotalPrice().GetKopecks());
}

TEST(Saga_CreateOrderReturnsId) {
    TestEnvironment env;
    
    std::vector<std::pair<std::int32_t, std::int32_t>> items = {{4, 1}};
    
    auto result1 = env.order_service->CreateOrder(1, items);
    ASSERT_TRUE(result1.success);
    ASSERT_EQ(1, result1.order_id);
    
    auto result2 = env.order_service->CreateOrder(1, items);
    ASSERT_TRUE(result2.success);
    ASSERT_EQ(2, result2.order_id);
    
    auto result3 = env.order_service->CreateOrder(1, items);
    ASSERT_TRUE(result3.success);
    ASSERT_EQ(3, result3.order_id);
}

TEST(Saga_GetOrderNotFound) {
    TestEnvironment env;
    
    auto order = env.order_service->GetOrder(999);
    ASSERT_FALSE(order.has_value());
}
