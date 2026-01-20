#ifndef __TRIVIALDB_DBMS__
#define __TRIVIALDB_DBMS__
#include "database.h"
#include "../table/table.h"
#include "../parser/defs.h"
#include "../expression/expression.h"
#include <cstdio>
#include <string>
#include <unordered_map>

// Privilege Constants
enum Privilege {
    PRIV_NONE = 0,
    PRIV_SELECT = 1 << 0,
    PRIV_INSERT = 1 << 1,
    PRIV_UPDATE = 1 << 2,
    PRIV_DELETE = 1 << 3,
    PRIV_CREATE = 1 << 4,
    PRIV_DROP   = 1 << 5,
    PRIV_ALTER  = 1 << 6,
    PRIV_ALL    = 0xFFFF
};

struct UserSession {
    std::string username;
    bool is_admin;
    std::unordered_map<std::string, int> table_privileges; // Table -> Priv Mask
};

class dbms
{
		FILE *output_file;
	database *cur_db;
	UserSession *current_user;
private:
	dbms();

public:
	~dbms();

	void close_database();
	void show_database(const char *db_name);
	void switch_database(const char *db_name);
	void drop_database(const char *db_name);
	void create_database(const char *db_name);

	void create_table(const table_header_t *header);
	void show_table(const char *table_name);
	void drop_table(const char *table_name);
	void rename_table(const char *old_name, const char *new_name);
	void alter_table_add_column(const char *table_name, const field_item_t *field);
	void alter_table_drop_column(const char *table_name, const char *column_name);
	void alter_table_modify_column(const char *table_name, const field_item_t *field);
	void alter_table_rename_column(const char *table_name, const char *old_name, const char *new_name);

	void create_index(const char *tb_name, const char *col_name);
	void drop_index(const char *tb_name, const char *col_name);

	void insert_rows(const insert_info_t *info);
	void delete_rows(const delete_info_t *info);
	void select_rows(const select_info_t *info);
	void update_rows(const update_info_t *info);

	void switch_select_output(const char *filename);

	void select_rows_with_groupby(
		const select_info_t* info,
		const std::vector<table_manager*>& required_tables,
		const std::vector<expr_node_t*>& exprs,
		const std::vector<std::string>& expr_names,
		bool has_aggregate);

	void select_rows_aggregate(
		const select_info_t *info,
		const std::vector<table_manager*> &required_tables,
		const std::vector<expr_node_t*> &exprs,
		const std::vector<std::string> &expr_names);

		bool value_exists(const char *table, const char *column, const char *data);

	// User Management
	void login(const char *username, const char *password);
	void logout();
	bool check_privilege(const char *table_name, int required_priv);
	void create_user(const char *username, const char *password);
	void grant_privilege(const char *username, const char *table, int priv);

public:
	bool assert_db_open();
	void cache_record(table_manager *tm, record_manager *rm);

	template<typename Callback>
	void iterate(std::vector<table_manager*> required_tables, expr_node_t *cond, Callback callback);

	template<typename Callback>
	void iterate_one_table(table_manager* table,
			expr_node_t *cond, Callback callback);
	template<typename Callback>
	bool iterate_one_table_with_index(table_manager* table,
			expr_node_t *cond, Callback callback);
	template<typename Callback>
	bool iterate_many_tables_impl(
		const std::vector<table_manager*> &table_list,
		std::vector<record_manager*> &record_list,
		std::vector<int> &rid_list,
		std::vector<std::vector<expr_node_t*>> &index_cond,
		int *iter_order, int *index_cid, index_manager** index,
		expr_node_t *cond, Callback callback, int now);
	template<typename Callback>
	void iterate_many_tables(
		const std::vector<table_manager*> &table_list,
		expr_node_t *cond, Callback callback);

	static expr_node_t *get_join_cond(expr_node_t *cond);
	static void extract_and_cond(expr_node_t *cond, std::vector<expr_node_t*> &and_cond);
	static bool find_longest_path(int now, int depth, int *mark, int *path, std::vector<std::vector<int>> &E, int excepted_len, int &max_depth);

public:
	static dbms* get_instance()
	{
		static dbms ms;
		return &ms;
	}
};

#endif
