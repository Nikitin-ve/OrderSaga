#include <iostream>
#include <iomanip>
#include <string>
#include <limits>
#include <vector>

#include "services/discount_service.hpp"
#include "services/inventory_service.hpp"
#include "services/billing_service.hpp"
#include "services/notification_service.hpp"
#include "services/order_service.hpp"

/**
 * @brief Консольный интерфейс для демонстрации системы OrderSaga
 */

// Глобальные сервисы
BillingService billing_service;
InventoryService inventory_service;
DiscountService discount_service;
NotificationService notification_service;
std::unique_ptr<OrderService> order_service;

// Цвета для консоли
const std::string kReset = "\033[0m";
const std::string kRed = "\033[31m";
const std::string kGreen = "\033[32m";
const std::string kYellow = "\033[33m";
const std::string kBlue = "\033[34m";
const std::string kCyan = "\033[36m";
const std::string kBold = "\033[1m";

void PrintHeader(const std::string& title) {
    std::cout << "\n" << kBold << kCyan << "═══════════════════════════════════════════════════════════" << kReset << "\n";
    std::cout << kBold << kCyan << "  " << title << kReset << "\n";
    std::cout << kBold << kCyan << "═══════════════════════════════════════════════════════════" << kReset << "\n\n";
}

void PrintSubHeader(const std::string& title) {
    std::cout << "\n" << kBold << kYellow << "--- " << title << " ---" << kReset << "\n\n";
}

void PrintSuccess(const std::string& message) {
    std::cout << kGreen << "✓ " << message << kReset << "\n";
}

void PrintError(const std::string& message) {
    std::cout << kRed << "✗ " << message << kReset << "\n";
}

void PrintInfo(const std::string& message) {
    std::cout << kBlue << "ℹ " << message << kReset << "\n";
}

void ClearInput() {
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

void InitializeTestData() {
    // Пользователи с балансом в рублях
    billing_service.AddUser(User(1, "Иван Петров", Money::FromRubles(100000)));
    billing_service.AddUser(User(2, "Мария Сидорова", Money::FromRubles(50000)));
    billing_service.AddUser(User(3, "Алексей Козлов", Money::FromRubles(15000)));
    billing_service.AddUser(User(4, "Бедный Студент", Money::FromRubles(1000)));
    
    // Скидки
    discount_service.SetUserDiscount(1, 10.0); // 10% скидка
    discount_service.SetVipStatus(2, true);    // VIP +5%
    discount_service.SetUserDiscount(3, 5.0);  // 5% скидка
    
    // Товары с ценами в рублях
    inventory_service.AddProduct(Product(1, "Ноутбук ASUS", Money::FromRubles(65000), 5));
    inventory_service.AddProduct(Product(2, "Смартфон Samsung", Money::FromRubles(45000), 10));
    inventory_service.AddProduct(Product(3, "Наушники Sony", Money::FromRubles(8000), 25));
    inventory_service.AddProduct(Product(4, "Клавиатура Logitech", Money::FromRubles(3500), 30));
    inventory_service.AddProduct(Product(5, "Мышь Razer", Money::FromRubles(2500), 50));
    inventory_service.AddProduct(Product(6, "Монитор LG 27\"", Money::FromRubles(35000), 8));
    
    order_service = std::make_unique<OrderService>(
        billing_service, inventory_service, discount_service, notification_service);
}

void ShowUsers() {
    PrintSubHeader("Пользователи системы");
    
    auto users = billing_service.GetAllUsers();
    
    std::cout << std::left << std::setw(5) << "ID"
              << std::setw(20) << "Имя"
              << std::setw(18) << "Баланс"
              << std::setw(10) << "Скидка"
              << std::setw(8) << "VIP"
              << std::setw(10) << "Заказов" << "\n";
    std::cout << std::string(71, '-') << "\n";
    
    for (const auto& user : users) {
        DiscountInfo discount = discount_service.GetUserDiscount(user.GetId());
        
        std::cout << std::left << std::setw(5) << user.GetId()
                  << std::setw(20) << user.GetName()
                  << std::setw(18) << (user.GetBalance().ToString() + " руб.")
                  << std::setw(10) << (std::to_string(static_cast<std::int32_t>(discount.discount_percent)) + "%")
                  << std::setw(8) << (discount.is_vip ? "Да" : "Нет")
                  << std::setw(10) << discount.orders_count << "\n";
    }
}

void ShowProducts() {
    PrintSubHeader("Товары на складе");
    
    auto products = inventory_service.GetAllProducts();
    
    std::cout << std::left << std::setw(5) << "ID"
              << std::setw(25) << "Название"
              << std::setw(18) << "Цена"
              << std::setw(10) << "Остаток" << "\n";
    std::cout << std::string(58, '-') << "\n";
    
    for (const auto& product : products) {
        std::cout << std::left << std::setw(5) << product.GetId()
                  << std::setw(25) << product.GetName()
                  << std::setw(18) << (product.GetPrice().ToString() + " руб.")
                  << std::setw(10) << product.GetStock() << "\n";
    }
}

void ShowOrders() {
    PrintSubHeader("Все заказы");
    
    auto orders = order_service->GetAllOrders();
    
    if (orders.empty()) {
        PrintInfo("Заказов пока нет");
        return;
    }
    
    std::cout << std::left << std::setw(5) << "ID"
              << std::setw(15) << "Пользователь"
              << std::setw(15) << "Сумма"
              << std::setw(10) << "Скидка"
              << std::setw(15) << "Итого"
              << std::setw(15) << "Статус" << "\n";
    std::cout << std::string(75, '-') << "\n";
    
    for (const auto& order : orders) {
        auto user = billing_service.GetUser(order.GetUserId());
        
        std::string status = order.GetStatusString();
        std::string status_color = kReset;
        if (status == "CONFIRMED") status_color = kGreen;
        else if (status == "FAILED" || status == "COMPENSATED") status_color = kRed;
        else if (status == "PROCESSING") status_color = kYellow;
        
        std::string user_name = user ? user->GetName() : "Unknown";
        if (user_name.length() > 14) user_name = user_name.substr(0, 14);
        
        std::cout << std::left << std::setw(5) << order.GetId()
                  << std::setw(15) << user_name
                  << std::setw(15) << order.GetTotalPrice().ToString()
                  << std::setw(10) << (std::to_string(static_cast<std::int32_t>(order.GetDiscountPercent())) + "%")
                  << std::setw(15) << order.GetFinalPrice().ToString()
                  << status_color << std::setw(15) << status << kReset << "\n";
    }
}

void CreateOrder() {
    PrintSubHeader("Создание нового заказа");
    
    ShowUsers();
    std::cout << "\n";
    
    std::int32_t user_id;
    std::cout << "Введите ID пользователя: ";
    std::cin >> user_id;
    
    auto user = billing_service.GetUser(user_id);
    if (!user) {
        PrintError("Пользователь не найден");
        ClearInput();
        return;
    }
    
    PrintInfo("Выбран пользователь: " + user->GetName() + 
              " (баланс: " + user->GetBalance().ToString() + " руб.)");
    
    ShowProducts();
    std::cout << "\n";
    
    std::vector<std::pair<std::int32_t, std::int32_t>> items;
    
    while (true) {
        std::int32_t product_id, quantity;
        std::cout << "Введите ID товара (0 для завершения): ";
        std::cin >> product_id;
        
        if (product_id == 0) break;
        
        auto product = inventory_service.GetProduct(product_id);
        if (!product) {
            PrintError("Товар не найден");
            continue;
        }
        
        std::cout << "Введите количество: ";
        std::cin >> quantity;
        
        if (quantity <= 0) {
            PrintError("Некорректное количество");
            continue;
        }
        
        if (!inventory_service.CanReserve(product_id, quantity)) {
            PrintError("Недостаточно товара на складе (доступно: " + 
                      std::to_string(product->GetStock()) + ")");
            continue;
        }
        
        items.emplace_back(product_id, quantity);
        PrintSuccess("Добавлено: " + product->GetName() + " x" + std::to_string(quantity));
    }
    
    if (items.empty()) {
        PrintError("Заказ пуст");
        ClearInput();
        return;
    }
    
    ClearInput();
    
    std::cout << "\n" << kBold << "Создание заказа..." << kReset << "\n\n";
    
    auto result = order_service->CreateOrder(user_id, items);
    
    if (!result.success) {
        PrintError(result.message);
        return;
    }
    
    PrintSuccess("Заказ #" + std::to_string(result.order_id) + " создан");
    
    std::cout << "\n" << kBold << "Обработка заказа (Saga)..." << kReset << "\n\n";
    
    auto process_result = order_service->ProcessOrder(result.order_id);
    
    std::cout << "\n";
    if (process_result.success) {
        PrintSuccess(process_result.message);
        
        auto order = order_service->GetOrder(result.order_id);
        if (order) {
            std::cout << "\n" << kBold << "Детали заказа #" << result.order_id << ":" << kReset << "\n";
            std::cout << "  Сумма до скидки: " << order->GetTotalPrice().ToString() << " руб.\n";
            std::cout << "  Скидка: " << order->GetDiscountPercent() << "%\n";
            std::cout << "  Итого к оплате: " << kGreen << order->GetFinalPrice().ToString() << " руб." << kReset << "\n";
        }
    } else {
        PrintError(process_result.message);
        
        auto order = order_service->GetOrder(result.order_id);
        if (order) {
            std::cout << "  Причина: " << order->GetFailureReason() << "\n";
            std::cout << "  Статус: " << order->GetStatusString() << "\n";
        }
    }
}

void CancelOrder() {
    PrintSubHeader("Отмена заказа");
    
    ShowOrders();
    std::cout << "\n";
    
    std::int32_t order_id;
    std::cout << "Введите ID заказа для отмены: ";
    std::cin >> order_id;
    ClearInput();
    
    ServiceResult result = order_service->CancelOrder(order_id);
    
    if (result.success) {
        PrintSuccess(result.message);
    } else {
        PrintError(result.message);
    }
}

void ShowLogs() {
    PrintSubHeader("Логи операций");
    
    std::cout << kBold << "Лог OrderService:" << kReset << "\n";
    for (const auto& log : order_service->GetOperationLog()) {
        std::cout << "  " << log << "\n";
    }
    
    std::cout << "\n" << kBold << "Лог BillingService:" << kReset << "\n";
    for (const auto& log : billing_service.GetOperationLog()) {
        std::cout << "  " << log << "\n";
    }
    
    std::cout << "\n" << kBold << "Лог InventoryService:" << kReset << "\n";
    for (const auto& log : inventory_service.GetOperationLog()) {
        std::cout << "  " << log << "\n";
    }
    
    std::cout << "\n" << kBold << "Лог NotificationService:" << kReset << "\n";
    for (const auto& log : notification_service.GetOperationLog()) {
        std::cout << "  " << log << "\n";
    }
}

void ShowNotifications() {
    PrintSubHeader("Уведомления");
    
    std::cout << "Введите ID пользователя (0 для всех): ";
    std::int32_t user_id;
    std::cin >> user_id;
    ClearInput();
    
    std::vector<Notification> notifications;
    if (user_id == 0) {
        for (const auto& log : notification_service.GetOperationLog()) {
            std::cout << "  " << log << "\n";
        }
        return;
    } else {
        notifications = notification_service.GetNotificationsForUser(user_id);
    }
    
    if (notifications.empty()) {
        PrintInfo("Уведомлений нет");
        return;
    }
    
    for (const auto& n : notifications) {
        std::string type_color = kBlue;
        if (n.type == NotificationType::kOrderConfirmed || n.type == NotificationType::kPaymentProcessed) {
            type_color = kGreen;
        } else if (n.type == NotificationType::kOrderFailed || n.type == NotificationType::kOrderCancelled) {
            type_color = kRed;
        }
        
        std::cout << type_color << "[" << n.GetTypeString() << "]" << kReset 
                  << " Заказ #" << n.order_id << ": " << n.message << "\n";
    }
}

void SimulateFailure() {
    PrintSubHeader("Симуляция сбоев");
    
    std::cout << "1. Сбой сервиса скидок\n";
    std::cout << "2. Сбой сервиса склада\n";
    std::cout << "3. Сбой сервиса оплаты\n";
    std::cout << "4. Сбой сервиса уведомлений\n";
    std::cout << "5. Отключить все сбои\n";
    std::cout << "0. Назад\n\n";
    
    std::cout << "Текущее состояние:\n";
    std::cout << "  Скидки: " << (discount_service.IsSimulatingFailure() ? kRed + "СБОЙ" : kGreen + "OK") << kReset << "\n";
    std::cout << "  Склад: " << (inventory_service.IsSimulatingFailure() ? kRed + "СБОЙ" : kGreen + "OK") << kReset << "\n";
    std::cout << "  Оплата: " << (billing_service.IsSimulatingFailure() ? kRed + "СБОЙ" : kGreen + "OK") << kReset << "\n";
    std::cout << "  Уведомления: " << (notification_service.IsSimulatingFailure() ? kRed + "СБОЙ" : kGreen + "OK") << kReset << "\n\n";
    
    std::int32_t choice;
    std::cout << "Выбор: ";
    std::cin >> choice;
    ClearInput();
    
    switch (choice) {
        case 1:
            discount_service.SetSimulateFailure(!discount_service.IsSimulatingFailure());
            break;
        case 2:
            inventory_service.SetSimulateFailure(!inventory_service.IsSimulatingFailure());
            break;
        case 3:
            billing_service.SetSimulateFailure(!billing_service.IsSimulatingFailure());
            break;
        case 4:
            notification_service.SetSimulateFailure(!notification_service.IsSimulatingFailure());
            break;
        case 5:
            discount_service.SetSimulateFailure(false);
            inventory_service.SetSimulateFailure(false);
            billing_service.SetSimulateFailure(false);
            notification_service.SetSimulateFailure(false);
            PrintSuccess("Все сбои отключены");
            break;
    }
}

void RunDemo() {
    PrintHeader("ДЕМОНСТРАЦИЯ ПАТТЕРНА SAGA");
    
    std::cout << "Эта демонстрация покажет работу паттерна Saga с компенсационными транзакциями.\n\n";
    
    // Демо 1: Успешный заказ
    PrintSubHeader("Сценарий 1: Успешный заказ");
    std::cout << "Иван Петров (10% скидка) покупает наушники и мышь.\n\n";
    
    std::vector<std::pair<std::int32_t, std::int32_t>> items1 = {{3, 2}, {5, 1}};
    
    auto result1 = order_service->CreateOrder(1, items1);
    if (result1.success) {
        auto process_result1 = order_service->ProcessOrder(result1.order_id);
        if (process_result1.success) {
            PrintSuccess("Заказ успешно создан!");
            auto order = order_service->GetOrder(result1.order_id);
            if (order) {
                std::cout << "  Сумма: " << order->GetTotalPrice().ToString() << " -> " 
                          << order->GetFinalPrice().ToString() 
                          << " руб. (скидка " << order->GetDiscountPercent() << "%)\n";
            }
        } else {
            PrintError("Заказ не обработан: " + process_result1.message);
        }
    } else {
        PrintError("Заказ не создан: " + result1.message);
    }
    
    std::cout << "\nНажмите Enter для продолжения...";
    std::cin.get();
    
    // Демо 2: Недостаточно средств
    PrintSubHeader("Сценарий 2: Недостаточно средств (компенсация)");
    std::cout << "Бедный студент (1000 руб.) пытается купить смартфон (45000 руб.).\n\n";
    
    std::vector<std::pair<std::int32_t, std::int32_t>> items2 = {{2, 1}};
    
    auto result2 = order_service->CreateOrder(4, items2);
    if (result2.success) {
        auto process_result2 = order_service->ProcessOrder(result2.order_id);
        if (!process_result2.success) {
            PrintError("Заказ не выполнен: " + process_result2.message);
            std::cout << "\n" << kYellow << "Saga выполнила компенсацию:" << kReset << "\n";
            std::cout << "  - Скидка отменена\n";
            std::cout << "  - Товар возвращён на склад\n";
            std::cout << "  - Баланс не изменился\n";
        }
    } else {
        PrintError("Заказ не создан: " + result2.message);
    }
    
    std::cout << "\nНажмите Enter для продолжения...";
    std::cin.get();
    
    // Демо 3: Сбой сервиса оплаты
    PrintSubHeader("Сценарий 3: Сбой сервиса оплаты");
    std::cout << "Симулируем сбой платёжной системы.\n\n";
    
    billing_service.SetSimulateFailure(true);
    
    std::vector<std::pair<std::int32_t, std::int32_t>> items3 = {{4, 3}};
    
    auto result3 = order_service->CreateOrder(2, items3);
    if (result3.success) {
        auto process_result3 = order_service->ProcessOrder(result3.order_id);
        if (!process_result3.success) {
            PrintError("Заказ не выполнен: " + process_result3.message);
            std::cout << "\n" << kYellow << "Saga выполнила компенсацию:" << kReset << "\n";
            std::cout << "  - Скидка отменена\n";
            std::cout << "  - Зарезервированные товары возвращены на склад\n";
        }
    } else {
        PrintError("Заказ не создан: " + result3.message);
    }
    
    billing_service.SetSimulateFailure(false);
    
    std::cout << "\nНажмите Enter для продолжения...";
    std::cin.get();
    
    PrintSubHeader("Итоговое состояние системы");
    ShowOrders();
    std::cout << "\n";
    ShowUsers();
    std::cout << "\n";
    ShowProducts();
    
    std::cout << "\n" << kGreen << "Демонстрация завершена!" << kReset << "\n";
}

void ShowMenu() {
    std::cout << "\n" << kBold << "══════════════════════════════════════" << kReset << "\n";
    std::cout << kBold << "        СИСТЕМА ORDERSAGA v1.0" << kReset << "\n";
    std::cout << kBold << "══════════════════════════════════════" << kReset << "\n\n";
    
    std::cout << "1. Показать пользователей\n";
    std::cout << "2. Показать товары\n";
    std::cout << "3. Показать заказы\n";
    std::cout << "4. Создать заказ\n";
    std::cout << "5. Отменить заказ\n";
    std::cout << "6. Показать уведомления\n";
    std::cout << "7. Показать логи\n";
    std::cout << "8. Симуляция сбоев\n";
    std::cout << "9. Запустить демонстрацию\n";
    std::cout << "0. Выход\n\n";
    
    std::cout << "Выберите действие: ";
}

int main() {
    PrintHeader("ИНИЦИАЛИЗАЦИЯ СИСТЕМЫ ORDERSAGA");
    
    std::cout << "Загрузка тестовых данных...\n";
    InitializeTestData();
    
    PrintSuccess("Система инициализирована");
    PrintInfo("Добавлено пользователей: " + std::to_string(billing_service.GetAllUsers().size()));
    PrintInfo("Добавлено товаров: " + std::to_string(inventory_service.GetAllProducts().size()));
    
    std::int32_t choice;
    
    while (true) {
        ShowMenu();
        std::cin >> choice;
        
        if (std::cin.fail()) {
            ClearInput();
            PrintError("Некорректный ввод");
            continue;
        }
        
        ClearInput();
        
        switch (choice) {
            case 0:
                PrintHeader("ЗАВЕРШЕНИЕ РАБОТЫ");
                std::cout << "Спасибо за использование OrderSaga!\n\n";
                return 0;
            case 1: ShowUsers(); break;
            case 2: ShowProducts(); break;
            case 3: ShowOrders(); break;
            case 4: CreateOrder(); break;
            case 5: CancelOrder(); break;
            case 6: ShowNotifications(); break;
            case 7: ShowLogs(); break;
            case 8: SimulateFailure(); break;
            case 9: RunDemo(); break;
            default: PrintError("Неизвестная команда");
        }
    }
    
    return 0;
}
