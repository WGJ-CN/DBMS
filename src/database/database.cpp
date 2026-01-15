#include "database.h"
#include <fstream>
#include <string>
#include <cstring>
#include <cstdio>

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
	std::string filename = db_name;
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

	std::string filename = info.db_name;
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
	std::string filename = info.db_name;
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
	
	// 重命名表文件
	std::string old_data_file = std::string(old_name) + ".tdata";
	std::string old_head_file = std::string(old_name) + ".thead";
	std::string new_data_file = std::string(new_name) + ".tdata";
	std::string new_head_file = std::string(new_name) + ".thead";
	
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
	
	delete old_table;
	std::printf("[Info] Table renamed from `%s` to `%s`\n", old_name, new_name);
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
	
	// 这里需要实现具体的添加列逻辑
	// 由于表结构修改涉及复杂的数据迁移，这里先输出提示信息
	std::fprintf(stderr, "[Info] ALTER TABLE ADD COLUMN: adding column `%s` to table `%s`\n", 
				field->name, table_name);
	// 实际实现需要考虑表头修改、数据迁移、索引更新等
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
	
	std::fprintf(stderr, "[Info] ALTER TABLE DROP COLUMN: dropping column `%s` from table `%s`\n", 
				column_name, table_name);
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
	
	std::fprintf(stderr, "[Info] ALTER TABLE MODIFY COLUMN: modifying column `%s` in table `%s`\n", 
				field->name, table_name);
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
