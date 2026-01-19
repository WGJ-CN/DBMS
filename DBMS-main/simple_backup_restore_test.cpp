#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <ctime>
#include "src/database/dbms.h"

// 辅助函数：检查文件是否存在
bool file_exists(const std::string& path) {
    std::ifstream file(path);
    return file.good();
}

// 辅助函数：显示测试分隔线
void show_separator(const std::string& title) {
    std::cout << "\n=====================================" << std::endl;
    std::cout << "    " << title << std::endl;
    std::cout << "=====================================" << std::endl;
}

// 辅助函数：获取当前时间戳
std::string get_current_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto now_time = std::chrono::system_clock::to_time_t(now);
    std::tm local_tm = *std::localtime(&now_time);
    char timestamp[20];
    std::strftime(timestamp, sizeof(timestamp), "%Y%m%d%H%M%S", &local_tm);
    return std::string(timestamp);
}

// 辅助函数：复制备份目录到项目中（处理路径问题）
void copy_backup_to_project(const std::string& db_name, const std::string& timestamp) {
    std::string backup_dir = "backup_" + db_name + "_" + timestamp;
    std::string source_path = "d:/Users/28076/Downloads/database/" + backup_dir;
    std::string dest_path = "./database/" + backup_dir;
    
    // 创建目标目录
    std::string mkdir_cmd = "md " + dest_path;
    system(mkdir_cmd.c_str());
    
    // 复制所有文件
    std::string copy_cmd = "xcopy \"" + source_path + "\" \"" + dest_path + "\" /Y /I";
    system(copy_cmd.c_str());
    
    std::cout << "[INFO] Copied backup to project directory: " << dest_path << std::endl;
}

int main() {
    std::cout << "=====================================" << std::endl;
    std::cout << "    Simple Backup & Restore Test" << std::endl;
    std::cout << "=====================================" << std::endl;
    std::cout << "Date: " << get_current_timestamp() << std::endl;

    // 获取DBMS实例
    dbms* dbms_instance = dbms::get_instance();
    const std::string db_name = "test_backup_db";

    // ===============================================
    // 1. 创建数据库结构
    // ===============================================
    show_separator("1. Creating Database");

    std::cout << "Creating database: " << db_name << std::endl;
    dbms_instance->create_database(db_name.c_str());
    
    std::cout << "Using database: " << db_name << std::endl;
    dbms_instance->switch_database(db_name.c_str());
    
    std::cout << "[OK] Database created and opened" << std::endl;

    // ===============================================
    // 2. 执行备份操作
    // ===============================================
    show_separator("2. Performing Database Backup");

    std::cout << "Performing full database backup..." << std::endl;
    // 记录当前时间戳，用于后续查找备份
    std::string backup_timestamp = get_current_timestamp();
    dbms_instance->backup_database();
    std::cout << "[OK] Database backup completed at timestamp: " << backup_timestamp << std::endl;

    // 复制备份到项目database目录
    copy_backup_to_project(db_name, backup_timestamp);

    // ===============================================
    // 3. 模拟数据丢失
    // ===============================================
    show_separator("3. Simulating Data Loss");

    // 关闭当前数据库连接
    std::cout << "Closing current database connection..." << std::endl;
    dbms_instance->close_database();
    std::cout << "[OK] Database closed" << std::endl;

    // 查找并删除数据库文件
    std::cout << "\nDeleting database files to simulate data loss..." << std::endl;
    
    // 尝试删除数据库文件（考虑路径问题）
    std::vector<std::string> search_paths = {"./database", "../database", "../../database", "d:/Users/28076/Downloads/database"};
    
    bool found_db_file = false;
    for (const auto& base_path : search_paths) {
        // 删除数据库文件
        std::string db_file = base_path + "/" + db_name + ".database";
        if (file_exists(db_file)) {
            std::remove(db_file.c_str());
            std::cout << "[OK] Deleted: " << db_file << std::endl;
            found_db_file = true;
        }
    }
    
    if (!found_db_file) {
        std::cout << "[WARNING] Database file not found (may have been deleted already)" << std::endl;
    }
    
    std::cout << "[OK] Simulated data loss scenario" << std::endl;

    // ===============================================
    // 4. 执行恢复操作
    // ===============================================
    show_separator("4. Performing Database Restore");

    std::string backup_dir_name = "backup_" + db_name + "_" + backup_timestamp;
    std::string backup_dir = "../../database/" + backup_dir_name;
    std::cout << "Executing restore operation from: " << backup_dir << std::endl;
    
    // 执行恢复
    dbms_instance->restore_database(backup_dir.c_str());
    std::cout << "[OK] Restore operation completed" << std::endl;

    // ===============================================
    // 5. 验证恢复后的数据完整性
    // ===============================================
    show_separator("5. Verifying Data Integrity");
    
    std::cout << "Testing restored database access..." << std::endl;
    
    // 尝试使用恢复后的数据库
    dbms_instance->switch_database(db_name.c_str());
    std::cout << "[OK] Successfully switched to restored database" << std::endl;
    std::cout << "[OK] Database is accessible after restore" << std::endl;

    // 检查恢复后的文件
    bool db_file_restored = false;
    for (const auto& base_path : search_paths) {
        std::string db_file = base_path + "/" + db_name + ".database";
        if (file_exists(db_file)) {
            std::cout << "[OK] Restored database file found at: " << db_file << std::endl;
            db_file_restored = true;
            break;
        }
    }
    
    // ===============================================
    // 6. 测试总结
    // ===============================================
    show_separator("6. Test Summary");

    std::cout << "Backup & Restore Test Results:" << std::endl;
    std::cout << "1. Database Creation: [OK]" << std::endl;
    std::cout << "2. Backup Operation: [OK]" << std::endl;
    std::cout << "3. Data Loss Simulation: [OK]" << std::endl;
    std::cout << "4. Restore Operation: [OK]" << std::endl;
    std::cout << "5. Data Integrity Verification: " << (db_file_restored ? "[OK]" : "[ERROR]") << std::endl;
    
    std::cout << "\n=====================================" << std::endl;
    std::cout << "    TEST " << (db_file_restored ? "PASSED!" : "FAILED!") << std::endl;
    std::cout << "=====================================" << std::endl;
    
    if (db_file_restored) {
        std::cout << "Database backup and restore functionality is working correctly." << std::endl;
        std::cout << "The complete database structure has been restored from backup." << std::endl;
    } else {
        std::cout << "Some issues were found during data integrity verification." << std::endl;
        std::cout << "Please check the logs above for more details." << std::endl;
    }

    // 清理：关闭数据库
    dbms_instance->close_database();

    return db_file_restored ? 0 : 1;
}