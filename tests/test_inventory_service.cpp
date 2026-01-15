#include "test_framework.hpp"
#include "../src/services/inventory_service.hpp"

TEST(InventoryService_AddProduct) {
    InventoryService service;
    
    Product laptop(1, "Ноутбук", Money::FromRubles(50000), 10);
    ServiceResult result = service.AddProduct(laptop);
    
    ASSERT_TRUE(result.success);
    
    auto retrieved = service.GetProduct(1);
    ASSERT_TRUE(retrieved.has_value());
    ASSERT_EQ("Ноутбук", retrieved->GetName());
    ASSERT_EQ(10, retrieved->GetStock());
}

TEST(InventoryService_GetProductNotFound) {
    InventoryService service;
    
    auto product = service.GetProduct(999);
    ASSERT_FALSE(product.has_value());
}

TEST(InventoryService_CanReserve) {
    InventoryService service;
    
    Product phone(1, "Телефон", Money::FromRubles(30000), 5);
    service.AddProduct(phone);
    
    ASSERT_TRUE(service.CanReserve(1, 5));
    ASSERT_TRUE(service.CanReserve(1, 3));
    ASSERT_FALSE(service.CanReserve(1, 6));
    ASSERT_FALSE(service.CanReserve(1, 0));
    ASSERT_FALSE(service.CanReserve(1, -1));
    ASSERT_FALSE(service.CanReserve(999, 1));
}

TEST(InventoryService_ReserveProduct) {
    InventoryService service;
    
    Product tablet(1, "Планшет", Money::FromRubles(25000), 8);
    service.AddProduct(tablet);
    
    ServiceResult result = service.ReserveProduct(100, 1, 3);
    ASSERT_TRUE(result.success);
    
    ASSERT_EQ(5, service.GetStock(1));
}

TEST(InventoryService_ReserveUnavailable) {
    InventoryService service;
    
    Product mouse(1, "Мышь", Money::FromRubles(1500), 2);
    service.AddProduct(mouse);
    
    ServiceResult result = service.ReserveProduct(100, 1, 10);
    ASSERT_FALSE(result.success);
    
    ASSERT_EQ(2, service.GetStock(1));
}

TEST(InventoryService_ReserveMultipleProducts) {
    InventoryService service;
    
    service.AddProduct(Product(1, "Товар 1", Money::FromRubles(1000), 10));
    service.AddProduct(Product(2, "Товар 2", Money::FromRubles(2000), 5));
    service.AddProduct(Product(3, "Товар 3", Money::FromRubles(3000), 8));
    
    ASSERT_TRUE(service.ReserveProduct(100, 1, 2).success);
    ASSERT_TRUE(service.ReserveProduct(100, 2, 3).success);
    ASSERT_TRUE(service.ReserveProduct(100, 3, 1).success);
    
    ASSERT_EQ(8, service.GetStock(1));
    ASSERT_EQ(2, service.GetStock(2));
    ASSERT_EQ(7, service.GetStock(3));
}

TEST(InventoryService_ReleaseReservation) {
    InventoryService service;
    
    service.AddProduct(Product(1, "Товар", Money::FromRubles(5000), 10));
    
    service.ReserveProduct(100, 1, 4);
    ASSERT_EQ(6, service.GetStock(1));
    
    ServiceResult result = service.ReleaseReservation(100, 1);
    ASSERT_TRUE(result.success);
    
    ASSERT_EQ(10, service.GetStock(1));
}

TEST(InventoryService_ReleaseAllReservations) {
    InventoryService service;
    
    service.AddProduct(Product(1, "Товар 1", Money::FromRubles(1000), 10));
    service.AddProduct(Product(2, "Товар 2", Money::FromRubles(2000), 10));
    
    service.ReserveProduct(100, 1, 3);
    service.ReserveProduct(100, 2, 5);
    
    ASSERT_EQ(7, service.GetStock(1));
    ASSERT_EQ(5, service.GetStock(2));
    
    ServiceResult result = service.ReleaseAllReservations(100);
    ASSERT_TRUE(result.success);
    
    ASSERT_EQ(10, service.GetStock(1));
    ASSERT_EQ(10, service.GetStock(2));
}

TEST(InventoryService_ConfirmReservation) {
    InventoryService service;
    
    service.AddProduct(Product(1, "Товар", Money::FromRubles(5000), 10));
    service.ReserveProduct(100, 1, 3);
    
    ServiceResult result = service.ConfirmReservation(100, 1);
    ASSERT_TRUE(result.success);
}

TEST(InventoryService_SimulateFailure) {
    InventoryService service;
    
    service.AddProduct(Product(1, "Товар", Money::FromRubles(1000), 10));
    
    service.SetSimulateFailure(true);
    ASSERT_TRUE(service.IsSimulatingFailure());
    
    ServiceResult result = service.ReserveProduct(100, 1, 1);
    ASSERT_FALSE(result.success);
    
    ASSERT_EQ(10, service.GetStock(1));
    
    // Геттеры должны работать
    auto product = service.GetProduct(1);
    ASSERT_TRUE(product.has_value());
    
    service.SetSimulateFailure(false);
    result = service.ReserveProduct(100, 1, 1);
    ASSERT_TRUE(result.success);
    ASSERT_EQ(9, service.GetStock(1));
}

TEST(InventoryService_GetAllProducts) {
    InventoryService service;
    
    service.AddProduct(Product(1, "Товар 1", Money::FromRubles(1000), 10));
    service.AddProduct(Product(2, "Товар 2", Money::FromRubles(2000), 20));
    
    auto products = service.GetAllProducts();
    ASSERT_EQ(2, static_cast<std::int32_t>(products.size()));
    
    bool found_product1 = false, found_product2 = false;
    for (const auto& product : products) {
        if (product.GetId() == 1 && product.GetName() == "Товар 1") {
            found_product1 = true;
            ASSERT_EQ(10, product.GetStock());
        }
        if (product.GetId() == 2 && product.GetName() == "Товар 2") {
            found_product2 = true;
            ASSERT_EQ(20, product.GetStock());
        }
    }
    ASSERT_TRUE(found_product1);
    ASSERT_TRUE(found_product2);
}

TEST(InventoryService_ProductPriceInKopecks) {
    InventoryService service;
    
    // Тест на точность цен в копейках
    Product item(1, "Товар", Money::FromRublesAndKopecks(999, 99), 5);
    service.AddProduct(item);
    
    auto retrieved = service.GetProduct(1);
    ASSERT_TRUE(retrieved.has_value());
    ASSERT_EQ(99999, retrieved->GetPrice().GetKopecks()); // 999.99 рублей
}

TEST(InventoryService_GetStockNotFound) {
    InventoryService service;
    
    ASSERT_EQ(-1, service.GetStock(999));
}
