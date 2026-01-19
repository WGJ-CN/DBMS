#include <iostream>
#include <string>
#include "src/database/dbms.h"

// 显示测试分隔线
void show_separator(const std::string& title) {
    std::cout << "\n=====================================" << std::endl;
    std::cout << "    " << title << std::endl;
    std::cout << "=====================================" << std::endl;
}

int main() {
    std::cout << "=====================================" << std::endl;
    std::cout << "    Database Content Viewer" << std::endl;
    std::cout << "=====================================" << std::endl;
    
    const std::string db_name = "test_backup_db";
    
    // 获取DBMS实例
    dbms* dbms_instance = dbms::get_instance();
    
    // 尝试连接到数据库
    show_separator("1. Connecting to Database");
    std::cout << "Connecting to database: " << db_name << std::endl;
    
    try {
        // 使用show_database方法显示数据库信息
        show_separator("2. Database Information");
        std::cout << "Displaying database information for: " << db_name << std::endl;
        dbms_instance->show_database(db_name.c_str());
        
        // 也可以尝试连接到数据库并显示表信息
        show_separator("3. Accessing Database");
        dbms_instance->switch_database(db_name.c_str());
        std::cout << "[OK] Successfully connected to database: " << db_name << std::endl;
        
        // 关闭数据库连接
        show_separator("4. Closing Connection");
        dbms_instance->close_database();
        std::cout << "[OK] Database connection closed" << std::endl;
        
        std::cout << "\n=====================================" << std::endl;
        std::cout << "    Database content displayed successfully!" << std::endl;
        std::cout << "=====================================" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Failed to access database: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}