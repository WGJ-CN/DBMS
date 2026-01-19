#include "database.h"
#include <fstream>
#include <string>
#include <cstring>
#include <cstdio>
#include <sys/stat.h>

// 辅助函数：构建带路径的文件名（项目根目录下的database文件夹）
inline std::string get_db_file_path(const char *filename) {
    return std::string("../../database/") + filename;
}

// 辅助函数：检查文件夹是否存在，不存在则创建
inline void ensure_database_folder() {
    struct stat st;
    if (stat("../../database", &st) != 0) {
        #ifdef _WIN32
        mkdir("../../database");
        #else
        mkdir("../../database", 0755);
        #endif
    }
}

database::database() : opened(false)
{
}

database::~database()
{
	if(is_opened()) close();
}

void database::open(const char *db_name)
{
	assert(!is_opened());
	ensure_database_folder();
	std::string filename = get_db_file_path(db_name);
	filename += ".database";
	std::ifstream ifs(filename, std::ios::binary);
	ifs.read((char*)&info, sizeof(info));
	std::memset(tables, 0, sizeof(tables));
	for(int i = 0; i < info.table_num; ++i)
	{
		tables[i] = new table_manager;
		tables[i]->open(info.table_name[i]);
	}
	opened = true;
}

void database::create(const char *db_name)
{
	assert(!is_opened());
	std::memset(&info, 0, sizeof(info));
	std::memset(tables, 0, sizeof(tables));
	std::strncpy(info.db_name, db_name, MAX_NAME_LEN);
	opened = true;
}

void database::close()
{
	assert(is_opened());
	for(table_manager *tb : tables)
	{
		if(tb != nullptr)
		{
			tb->close();
			delete tb;
			tb = nullptr;
		}
	}

	std::string filename = get_db_file_path(info.db_name);
	filename += ".database";
	std::ofstream ofs(filename, std::ios::binary);
	ofs.write((char*)&info, sizeof(info));
	opened = false;
}

void database::create_table(const table_header_t *header)
{
	if(!is_opened())
	{
		std::fprintf(stderr, "[Error] database not opened.\n");
	} else if(get_table(header->table_name)) {
		std::fprintf(stderr, "[Error] table `%s` already exists.\n", header->table_name);
	} else {
		int id = info.table_num++;
		std::strncpy(info.table_name[id], header->table_name, MAX_NAME_LEN);
		tables[id] = new table_manager;
		tables[id]->create(header->table_name, header);
	}
}

void database::drop()
{
	assert(is_opened());
	for(int i = 0; i != info.table_num; ++i)
	{
		tables[i]->drop();
		delete tables[i];
		tables[i] = nullptr;
	}

	info.table_num = 0;
	std::string filename = get_db_file_path(info.db_name);
	filename += ".database";
	close();
	std::remove(filename.c_str());
}

table_manager* database::get_table(const char *name)
{
	assert(is_opened());
	int id = get_table_id(name);
	return id >= 0 ? tables[id] : nullptr;
}

table_manager* database::get_table(int id)
{
	assert(is_opened());
	if(id >= 0 && id < info.table_num)
		return tables[id];
	else return nullptr;
}

int database::get_table_id(const char *name)
{
	assert(is_opened());
	for(int i = 0; i < info.table_num; ++i)
	{
		if(std::strcmp(name, info.table_name[i]) == 0)
			return i;
	}

	return -1;
}

void database::drop_table(const char *name)
{
	assert(is_opened());
	int id = get_table_id(name);
	if(id < 0)
	{
		std::fprintf(stderr, "[Error] DROP TABLE: table `%s` not found!\n", name);
		return;
	}

	--info.table_num;
	tables[id]->drop();
	delete tables[id];
	for(int i = id; i < info.table_num; ++i)
	{
		tables[i] = tables[i + 1];
		std::strcpy(info.table_name[i], info.table_name[i + 1]);
	}

	tables[info.table_num] = nullptr;
}

void database::rename_table(const char *old_name, const char *new_name)
{
	if(!is_opened()) {
		std::fprintf(stderr, "[Error] database not opened.\n");
		return;
	}
	
	int id = get_table_id(old_name);
	if(id < 0) {
		std::fprintf(stderr, "[Error] RENAME TABLE: table `%s` not found!\n", old_name);
		return;
	}
	
	// 检查新表名是否已存在
	if(get_table(new_name)) {
		std::fprintf(stderr, "[Error] RENAME TABLE: table `%s` already exists!\n", new_name);
		return;
	}
	
	// 先关闭当前表，然后重命名文件
	table_manager *old_table = tables[id];
	old_table->close();
	
	// 重命名表文件（使用正确的路径）
	std::string old_data_file = get_db_file_path(old_name) + ".tdata";
	std::string old_head_file = get_db_file_path(old_name) + ".thead";
	std::string new_data_file = get_db_file_path(new_name) + ".tdata";
	std::string new_head_file = get_db_file_path(new_name) + ".thead";
	
	if(rename(old_data_file.c_str(), new_data_file.c_str()) != 0 ||
	   rename(old_head_file.c_str(), new_head_file.c_str()) != 0) {
		std::fprintf(stderr, "[Error] RENAME TABLE: failed to rename table files!\n");
		// 重命名失败，重新打开原表
		old_table->open(old_name);
		return;
	}
	
	// 更新数据库信息
	std::strncpy(info.table_name[id], new_name, MAX_NAME_LEN);
	
	// 重新打开新命名的表
	tables[id] = new table_manager;
	if(!tables[id]->open(new_name)) {
		std::fprintf(stderr, "[Error] RENAME TABLE: failed to reopen table!\n");
		delete tables[id];
		// 重命名失败，恢复原文件名并重新打开
		rename(new_data_file.c_str(), old_data_file.c_str());
		rename(new_head_file.c_str(), old_head_file.c_str());
		old_table->open(old_name);
		tables[id] = old_table;
		return;
	}
	
	// 更新表头中的表名
	tables[id]->update_table_name(new_name);
	
	// 立即保存更新后的数据库信息到文件
	std::string db_filename = get_db_file_path(info.db_name) + ".database";
	std::ofstream ofs(db_filename, std::ios::binary);
	if(ofs) {
		ofs.write((char*)&info, sizeof(info));
	}
	
	delete old_table;
	std::printf("[Info] Table renamed from `%s` to `%s`\n", old_name, new_name);
	std::printf("[Info] Database info updated and saved to disk.\n");
}

void database::alter_table_add_column(const char *table_name, const field_item_t *field)
{
	if(!is_opened()) {
		std::fprintf(stderr, "[Error] database not opened.\n");
		return;
	}
	
	table_manager *table = get_table(table_name);
	if(!table) {
		std::fprintf(stderr, "[Error] ALTER TABLE ADD COLUMN: table `%s` not found!\n", table_name);
		return;
	}
	
	if(!table->alter_table_add_column(field)) {
		std::fprintf(stderr, "[Error] ALTER TABLE ADD COLUMN: failed to add column `%s`\n", field->name);
	}
}

void database::alter_table_drop_column(const char *table_name, const char *column_name)
{
	if(!is_opened()) {
		std::fprintf(stderr, "[Error] database not opened.\n");
		return;
	}
	
	table_manager *table = get_table(table_name);
	if(!table) {
		std::fprintf(stderr, "[Error] ALTER TABLE DROP COLUMN: table `%s` not found!\n", table_name);
		return;
	}
	
	if(!table->alter_table_drop_column(column_name)) {
		std::fprintf(stderr, "[Error] ALTER TABLE DROP COLUMN: failed to drop column `%s`\n", column_name);
	}
}

void database::alter_table_rename_column(const char *table_name, const char *old_name, const char *new_name)
{
	if(!is_opened()) {
		std::fprintf(stderr, "[Error] database not opened.\n");
		return;
	}
	
	table_manager *table = get_table(table_name);
	if(!table) {
		std::fprintf(stderr, "[Error] ALTER TABLE RENAME COLUMN: table `%s` not found!\n", table_name);
		return;
	}
	
	if(!table->alter_table_rename_column(old_name, new_name)) {
		std::fprintf(stderr, "[Error] ALTER TABLE RENAME COLUMN: failed to rename column `%s` to `%s`\n", old_name, new_name);
	}
}

void database::alter_table_modify_column(const char *table_name, const field_item_t *field)
{
	if(!is_opened()) {
		std::fprintf(stderr, "[Error] database not opened.\n");
		return;
	}
	
	table_manager *table = get_table(table_name);
	if(!table) {
		std::fprintf(stderr, "[Error] ALTER TABLE MODIFY COLUMN: table `%s` not found!\n", table_name);
		return;
	}
	
	if(!table->alter_table_modify_column(field)) {
		std::fprintf(stderr, "[Error] ALTER TABLE MODIFY COLUMN: failed to modify column `%s`\n", field->name);
	}
}

void database::show_info()
{
	std::printf("======== Database Info Begin ========\n");
	std::printf("Database name = %s\n", info.db_name);
	std::printf("Table number  = %d\n", info.table_num);
	for(int i = 0; i != info.table_num; ++i)
		std::printf("  [table] name = %s\n", info.table_name[i]);
	std::printf("======== Database Info End   ========\n");
}

// 辅助函数：复制文件
inline bool copy_file(const std::string &src, const std::string &dst) {
    std::ifstream src_file(src, std::ios::binary);
    if (!src_file) {
        std::fprintf(stderr, "[Error] Failed to open source file: %s\n", src.c_str());
        return false;
    }
    
    std::ofstream dst_file(dst, std::ios::binary);
    if (!dst_file) {
        std::fprintf(stderr, "[Error] Failed to open destination file: %s\n", dst.c_str());
        return false;
    }
    
    dst_file << src_file.rdbuf();
    return true;
}

void database::backup(const char *backup_file) {
    if (!is_opened()) {
        std::fprintf(stderr, "[Error] No database opened for backup.\n");
        return;
    }
    
    // 先关闭数据库确保所有数据已保存
    close();
    
    // 构建备份目录
    std::string backup_path = get_db_file_path(backup_file);
    
    // 复制数据库元信息文件
    std::string db_file = get_db_file_path(info.db_name) + ".database";
    std::string backup_db_file = backup_path + ".database";
    if (!copy_file(db_file, backup_db_file)) {
        std::fprintf(stderr, "[Error] Failed to backup database file.\n");
        // 重新打开数据库
        open(info.db_name);
        return;
    }
    
    // 复制所有表的文件
    for (int i = 0; i < info.table_num; ++i) {
        const char *table_name = info.table_name[i];
        
        // 复制表结构文件
        std::string table_head_file = get_db_file_path(table_name) + ".thead";
        std::string backup_head_file = backup_path + "_" + table_name + ".thead";
        if (!copy_file(table_head_file, backup_head_file)) {
            std::fprintf(stderr, "[Error] Failed to backup table head file: %s\n", table_name);
            // 重新打开数据库
            open(info.db_name);
            return;
        }
        
        // 复制表数据文件
        std::string table_data_file = get_db_file_path(table_name) + ".tdata";
        std::string backup_data_file = backup_path + "_" + table_name + ".tdata";
        if (!copy_file(table_data_file, backup_data_file)) {
            std::fprintf(stderr, "[Error] Failed to backup table data file: %s\n", table_name);
            // 重新打开数据库
            open(info.db_name);
            return;
        }
    }
    
    // 重新打开数据库
    open(info.db_name);
    
    std::printf("[Info] Database '%s' backed up successfully to '%s'.\n", info.db_name, backup_file);
}

void database::restore(const char *backup_file) {
    // 先关闭当前数据库
    if (is_opened()) {
        close();
    }
    
    // 构建备份文件路径
    std::string backup_path = get_db_file_path(backup_file);
    
    // 检查备份文件是否存在
    std::string backup_db_file = backup_path + ".database";
    std::ifstream backup_file_check(backup_db_file);
    if (!backup_file_check) {
        std::fprintf(stderr, "[Error] Backup file not found: %s\n", backup_file);
        return;
    }
    
    // 读取备份的数据库信息
    database_info backup_info;
    std::ifstream backup_info_file(backup_db_file, std::ios::binary);
    backup_info_file.read((char*)&backup_info, sizeof(backup_info));
    
    // 恢复数据库元信息文件
    std::string db_file = get_db_file_path(backup_info.db_name) + ".database";
    if (!copy_file(backup_db_file, db_file)) {
        std::fprintf(stderr, "[Error] Failed to restore database file.\n");
        return;
    }
    
    // 恢复所有表的文件
    for (int i = 0; i < backup_info.table_num; ++i) {
        const char *table_name = backup_info.table_name[i];
        
        // 恢复表结构文件
        std::string backup_head_file = backup_path + "_" + table_name + ".thead";
        std::string table_head_file = get_db_file_path(table_name) + ".thead";
        if (!copy_file(backup_head_file, table_head_file)) {
            std::fprintf(stderr, "[Error] Failed to restore table head file: %s\n", table_name);
            return;
        }
        
        // 恢复表数据文件
        std::string backup_data_file = backup_path + "_" + table_name + ".tdata";
        std::string table_data_file = get_db_file_path(table_name) + ".tdata";
        if (!copy_file(backup_data_file, table_data_file)) {
            std::fprintf(stderr, "[Error] Failed to restore table data file: %s\n", table_name);
            return;
        }
    }
    
    // 打开恢复后的数据库
    open(backup_info.db_name);
    
    std::printf("[Info] Database restored successfully from '%s'.\n", backup_file);
}
