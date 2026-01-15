#ifndef __TRIVIALDB_DATABASE__
#define __TRIVIALDB_DATABASE__
#include "../defs.h"
#include "../table/table.h"
#include "../expression/expression.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <vector>

class database
{
	struct database_info
	{
		int table_num;
		char db_name[MAX_NAME_LEN];
		char table_name[MAX_TABLE_NUM][MAX_NAME_LEN];
	} info;

	table_manager *tables[MAX_TABLE_NUM];

	bool opened;
public:
	database();
	~database();
	bool is_opened() { return opened; }
	void open(const char *db_name);
	void create(const char *db_name);
	void drop();
	void close();
	const char *get_name() { return info.db_name; }

	table_manager *get_table(const char *name);
	table_manager *get_table(int id);
	void drop_table(const char *name);
	int get_table_id(const char *name);
	void create_table(const table_header_t *header);
	void rename_table(const char *old_name, const char *new_name);
	void alter_table_add_column(const char *table_name, const field_item_t *field);
	void alter_table_drop_column(const char *table_name, const char *column_name);
	void alter_table_rename_column(const char *table_name, const char *old_name, const char *new_name);
	void alter_table_modify_column(const char *table_name, const field_item_t *field);
	void show_info();
};

#endif
