#include "dbms.h"
#include "database.h"
#include "../table/table.h"
#include "../index/index.h"
#include "../expression/expression.h"
#include "../utils/type_cast.h"
#include "../table/record.h"
#include "../logger/logger.h"
#include <vector>
#include <limits>
#include <algorithm>
#include <unordered_set>  // 添加这行

struct __cache_clear_guard
{
    ~__cache_clear_guard() { expression::cache_clear(); }
};

dbms::dbms()
    : output_file(stdout), cur_db(nullptr)
{
}

dbms::~dbms()
{
    close_database();
}

void dbms::switch_select_output(const char* filename)
{
    if (output_file != stdout)
        std::fclose(output_file);
    if (std::strcmp(filename, "stdout") == 0)
        output_file = stdout;
    else output_file = std::fopen(filename, "w");
}

template<typename Callback>
void dbms::iterate(
    std::vector<table_manager*> required_tables,
    expr_node_t* cond,
    Callback callback)
{
    if (required_tables.size() == 1)
    {
        std::vector<record_manager*> rm_list(1);
        std::vector<int> rid_list(1);
        iterate_one_table_with_index(required_tables[0], cond, [&](table_manager*, record_manager* rm, int rid) -> bool {
            rm_list[0] = rm;
            rid_list[0] = rid;
            return callback(required_tables, rm_list, rid_list);
            });
    }
    else {
        iterate_many_tables(required_tables, cond, callback);
        std::puts("[Info] Join many tables by enumerating.");
    }
}

template<typename Callback>
bool dbms::iterate_one_table_with_index(
    table_manager* table,
    expr_node_t* cond,
    Callback callback)
{
    std::vector<expr_node_t*> and_cond;
    extract_and_cond(cond, and_cond);
    expr_node_t* index_cond = nullptr;
    index_manager* index = nullptr;

    auto get_index = [&](column_ref_t* col) -> index_manager*
        {
            int cid = table->lookup_column(col->column);
            if (cid < 0) return nullptr;
            return table->get_index(cid);
        };

    for (expr_node_t* expr : and_cond)
    {
        if (expr->op == OPERATOR_EQ)
        {
            if (expr->right->term_type == TERM_COLUMN_REF)
                std::swap(expr->right, expr->left);

            if (expr->left->term_type == TERM_COLUMN_REF && expr->right->term_type != TERM_COLUMN_REF)
            {
                index = get_index(expr->left->column_ref);
                if (index)
                {
                    index_cond = expr;
                    break;
                }
            }
        }
    }

    if (!index_cond)
    {
        iterate_one_table(table, cond, callback);
        return false;
    }

    char* key = nullptr;
    switch (index_cond->right->term_type)
    {
    case TERM_INT:
    case TERM_DATE:
        key = (char*)&index_cond->right->val_i;
        break;
    case TERM_FLOAT:
        key = (char*)&index_cond->right->val_f;
        break;
    case TERM_STRING:
        key = index_cond->right->val_s;
        break;
    case TERM_BOOL:
        key = (char*)&index_cond->right->val_b;
        break;
    default:
        break;
    }

    auto it = index->get_iterator_lower_bound(key);
    for (; !it.is_end(); it.next())
    {
        int rid;
        record_manager rm = table->open_record_from_index_lower_bound(it.get(), &rid);
        table->cache_record(&rm);

        bool join_ret = false;
        try {
            join_ret = typecast::expr_to_bool(expression::eval(index_cond));
        }
        catch (const char* msg) {
            std::puts(msg);
            iterate_one_table(table, cond, callback);
            return false;
        }

        if (!join_ret) break;

        if (!callback(table, &rm, rid))
            break;
    }

    return true;
}

template<typename Callback>
void dbms::iterate_one_table(
    table_manager* table,
    expr_node_t* cond,
    Callback callback)
{
    auto bit = table->get_record_iterator_lower_bound(0);
    for (; !bit.is_end(); bit.next())
    {
        int rid;
        record_manager rm(bit.get_pager());
        rm.open(bit.get(), false);
        rm.read(&rid, 4);
        table->cache_record(&rm);
        if (cond)
        {
            bool result = false;
            try {
                result = typecast::expr_to_bool(expression::eval(cond));
            }
            catch (const char* msg) {
                std::puts(msg);
                return;
            }

            if (!result) continue;
        }

        if (!callback(table, &rm, rid))
            break;
    }
}

void dbms::extract_and_cond(expr_node_t* cond, std::vector<expr_node_t*>& and_cond)
{
    if (!cond) return;
    if (cond->op == OPERATOR_AND)
    {
        extract_and_cond(cond->left, and_cond);
        extract_and_cond(cond->right, and_cond);
    }
    else {
        and_cond.push_back(cond);
    }
}

template<typename Callback>
void dbms::iterate_many_tables(
    const std::vector<table_manager*>& table_list,
    expr_node_t* cond, Callback callback)
{
    std::vector<record_manager*> record_list(table_list.size());
    std::vector<int> rid_list(table_list.size());
    std::vector<expr_node_t*> and_cond;
    extract_and_cond(cond, and_cond);
    auto lookup_table = [&](const char* name) {
        for (int i = 0; i < (int)table_list.size(); ++i)
            if (std::strcmp(name, table_list[i]->get_table_name()) == 0)
                return i;
        return -1;
        };

    // edge
    std::vector<std::vector<int>> E(table_list.size());
    // join cond
    std::vector<std::vector<expr_node_t*>> J(table_list.size());
    for (auto& v : E) v.resize(table_list.size());
    for (auto& v : J) v.resize(table_list.size());

    // setup join condition graph
    for (expr_node_t* c : and_cond)
    {
        if (c->op == OPERATOR_EQ &&
            c->left->term_type == TERM_COLUMN_REF &&
            c->right->term_type == TERM_COLUMN_REF)
        {
            int tid1 = lookup_table(c->left->column_ref->table);
            int tid2 = lookup_table(c->right->column_ref->table);
            if (tid1 == -1 || tid2 == -1)
            {
                std::fprintf(stderr, "[Error] Table not found!\n");
                return;
            }

            table_manager* tb1 = table_list[tid1];
            table_manager* tb2 = table_list[tid2];

            int cid1 = tb1->lookup_column(c->left->column_ref->column);
            int cid2 = tb2->lookup_column(c->right->column_ref->column);
            if (cid1 == -1 || cid2 == -1)
            {
                std::fprintf(stderr, "[Error] Column not found!\n");
                return;
            }

            index_manager* idx1 = tb1->get_index(cid1);
            index_manager* idx2 = tb2->get_index(cid2);
            if (!idx1 && !idx2)
                continue;

            if (idx2)
            {
                E[tid2][tid1] = 1;
                J[tid2][tid1] = c;
            }

            if (idx1)
            {
                E[tid1][tid2] = 1;
                J[tid1][tid2] = c;
            }
        }
    }

    // find the longest path
    int* mark = new int[table_list.size()];
    int* path = new int[table_list.size()];
    int max_depth = 0, start = 0;
    for (int i = 0; i < (int)table_list.size(); ++i)
    {
        int m = 0;
        std::memset(mark, 0, table_list.size() * sizeof(int));
        find_longest_path(i, 0, mark, path, E, ~0u >> 1, m);
        if (m > max_depth)
        {
            max_depth = m;
            start = i;
        }
    }

    int _;
    std::memset(mark, 0, table_list.size() * sizeof(int));
    _ = find_longest_path(start, 0, mark, path, E, max_depth, _);
    assert(_);

    // generate iteration sequence
    std::memset(mark, 0, table_list.size() * sizeof(int));
    for (int i = 0; i <= max_depth; ++i)
        mark[path[i]] = 1;

    int cur = max_depth, len = table_list.size();
    for (int i = 0; i < len; ++i)
        if (!mark[i])
            path[++cur] = i;

    // setup iteration variable
    index_manager** index_ref = new index_manager * [len];
    int* index_cid = new int[len];
    std::fill(index_ref, index_ref + len, nullptr);
    std::fill(index_cid, index_cid + len, -1);

    for (int i = 0; i < max_depth; ++i)
    {
        expr_node_t* join_node = J[path[i]][path[i + 1]];
        if (std::strcmp(join_node->left->column_ref->table, table_list[path[i]]->get_table_name()) == 0)
        {
            index_cid[i] = table_list[path[i + 1]]->lookup_column(
                join_node->right->column_ref->column);
            index_ref[i] = table_list[path[i]]->get_index(
                table_list[path[i]]->lookup_column(
                    join_node->left->column_ref->column)
            );
        }
        else {
            index_cid[i] = table_list[path[i + 1]]->lookup_column(
                join_node->left->column_ref->column);
            index_ref[i] = table_list[path[i]]->get_index(
                table_list[path[i]]->lookup_column(
                    join_node->right->column_ref->column)
            );
        }

        assert(index_ref[i]);
    }

    iterate_many_tables_impl(
        table_list, record_list, rid_list,
        J, path, index_cid, index_ref,
        cond, callback, len - 1);

    // debug info
    std::printf("[Info] Iteration order: ");
    for (int i = 0; i < len; ++i)
    {
        if (i != 0) std::printf(", ");
        std::printf("%s", table_list[path[len - i - 1]]->get_table_name());
    }

    std::printf("\n[Info] Index use: ");
    for (int i = 0; i < max_depth; ++i)
    {
        if (i != 0) std::printf(", ");
        expr_node_t* node = J[path[i]][path[i + 1]];
        std::printf("%s.%s-%s.%s",
            node->left->column_ref->table,
            node->left->column_ref->column,
            node->right->column_ref->table,
            node->right->column_ref->column
        );
    }

    std::puts("");

    delete[]mark;
    delete[]path;
    delete[]index_cid;
    delete[]index_ref;
}

template<typename Callback>
bool dbms::iterate_many_tables_impl(
    const std::vector<table_manager*>& table_list,
    std::vector<record_manager*>& record_list,
    std::vector<int>& rid_list,
    std::vector<std::vector<expr_node_t*>>& index_cond,
    int* iter_order, int* index_cid, index_manager** index,
    expr_node_t* cond, Callback callback, int now)
{
    if (now < 0)
    {
        if (cond)
        {
            bool result = false;
            try {
                result = typecast::expr_to_bool(expression::eval(cond));
            }
            catch (const char* msg) {
                std::puts(msg);
                return false; // stop
            }

            if (!result)
                return true; // continue
        }

        if (!callback(table_list, record_list, rid_list))
            return false;  // stop
        return true;  // continue
    }
    else {
        if (!index[now])
        {
            auto it = table_list[iter_order[now]]->get_record_iterator_lower_bound(0);
            for (; !it.is_end(); it.next())
            {
                record_manager rm(it.get_pager());
                rm.open(it.get(), false);
                rm.read(&rid_list[iter_order[now]], 4);
                // std::printf("%d\n", rid_list[iter_order[now]]);
                table_list[iter_order[now]]->cache_record(&rm);
                record_list[iter_order[now]] = &rm;
                bool ret = iterate_many_tables_impl(
                    table_list, record_list, rid_list,
                    index_cond, iter_order, index_cid, index,
                    cond, callback, now - 1
                );

                if (!ret) return false;
            }
        }
        else {
            const char* tb_col = table_list[iter_order[now + 1]]->get_cached_column(index_cid[now]);
            table_manager* tb2 = table_list[iter_order[now]];
            auto tb2_it = index[now]->get_iterator_lower_bound(tb_col);
            for (; !tb2_it.is_end(); tb2_it.next())
            {
                int tb2_rid;
                record_manager tb2_rm = tb2->open_record_from_index_lower_bound(tb2_it.get(), &tb2_rid);
                tb2->cache_record(&tb2_rm);

                bool join_ret = false;
                try {
                    expr_node_t* join_cond = index_cond[iter_order[now]][iter_order[now + 1]];
                    join_ret = typecast::expr_to_bool(expression::eval(join_cond));
                }
                catch (const char* msg) {
                    std::puts(msg);
                    return false;
                }

                if (!join_ret) break;

                rid_list[iter_order[now]] = tb2_rid;
                record_list[iter_order[now]] = &tb2_rm;
                bool ret = iterate_many_tables_impl(
                    table_list, record_list, rid_list,
                    index_cond, iter_order, index_cid, index,
                    cond, callback, now - 1
                );

                if (!ret) return false;
            }
        }
    }

    return true;
}

void dbms::close_database()
{
    if (cur_db)
    {
        cur_db->close();
        delete cur_db;
        cur_db = nullptr;
    }
}

void dbms::switch_database(const char* db_name)
{
    if (cur_db)
    {
        cur_db->close();
        delete cur_db;
        cur_db = nullptr;
    }

    cur_db = new database();
    cur_db->open(db_name);
    
    // 日志记录
    Logger::get_instance()->log_database_op(OperationType::DB_USE, db_name, true, 
        std::string("Switched to database '") + db_name + "'");
}

void dbms::create_database(const char* db_name)
{
    database db;
    db.create(db_name);
    db.close();
    
    // 日志记录
    Logger::get_instance()->log_database_op(OperationType::DB_CREATE, db_name, true,
        std::string("Database '") + db_name + "' created successfully");
}

void dbms::drop_database(const char* db_name)
{
    if (cur_db && std::strcmp(cur_db->get_name(), db_name) == 0)
    {
        cur_db->close();
        delete cur_db;
        cur_db = nullptr;
    }

    database db;
    db.open(db_name);
    db.drop();
    
    // 日志记录
    Logger::get_instance()->log_database_op(OperationType::DB_DROP, db_name, true,
        std::string("Database '") + db_name + "' dropped successfully");
}

void dbms::show_database(const char* db_name)
{
    database db;
    db.open(db_name);
    db.show_info();
    
    // 日志记录
    Logger::get_instance()->log_database_op(OperationType::DB_SHOW, db_name, true);
}

void dbms::drop_table(const char* table_name)
{
    if (assert_db_open()) {
        cur_db->drop_table(table_name);
        // 日志记录
        Logger::get_instance()->log_table_op(OperationType::TABLE_DROP, table_name, true,
            std::string("Table '") + table_name + "' dropped successfully");
    }
}

void dbms::show_table(const char* table_name)
{
    if (assert_db_open())
    {
        table_manager* tm = cur_db->get_table(table_name);
        if (tm == nullptr)
        {
            std::fprintf(stderr, "[Error] Table `%s` not found.\n", table_name);
            // 日志记录失败
            Logger::get_instance()->log_table_op(OperationType::TABLE_SHOW, table_name, false,
                std::string("Table '") + table_name + "' not found");
        }
        else {
            tm->dump_table_info();
            // 日志记录成功
            Logger::get_instance()->log_table_op(OperationType::TABLE_SHOW, table_name, true);
        }
    }
}

void dbms::create_table(const table_header_t* header)
{
    if (assert_db_open()) {
        cur_db->create_table(header);
        // 日志记录
        Logger::get_instance()->log_table_op(OperationType::TABLE_CREATE, header->table_name, true,
            std::string("Table '") + header->table_name + "' created with " + 
            std::to_string(header->col_num) + " columns");
    }
}

void dbms::rename_table(const char* old_name, const char* new_name)
{
    if (assert_db_open()) {
        cur_db->rename_table(old_name, new_name);
        // 日志记录
        std::string sql = Logger::format_rename_table_sql(old_name, new_name);
        Logger::get_instance()->log(LogLevel::INFO, OperationType::TABLE_RENAME, sql, true,
            std::string("Table '") + old_name + "' renamed to '" + new_name + "'", old_name);
    }
}

void dbms::alter_table_add_column(const char* table_name, const field_item_t* field)
{
    if (assert_db_open()) {
        cur_db->alter_table_add_column(table_name, field);
        // 日志记录
        std::string sql = Logger::format_alter_add_sql(table_name, field->name);
        Logger::get_instance()->log(LogLevel::INFO, OperationType::TABLE_ALTER_ADD, sql, true,
            std::string("Column '") + field->name + "' added to table '" + table_name + "'", table_name);
    }
}

void dbms::alter_table_drop_column(const char* table_name, const char* column_name)
{
    if (assert_db_open()) {
        cur_db->alter_table_drop_column(table_name, column_name);
        // 日志记录
        std::string sql = Logger::format_alter_drop_sql(table_name, column_name);
        Logger::get_instance()->log(LogLevel::INFO, OperationType::TABLE_ALTER_DROP, sql, true,
            std::string("Column '") + column_name + "' dropped from table '" + table_name + "'", table_name);
    }
}
void dbms::alter_table_modify_column(const char* table_name, const field_item_t* field)
{
    if (!cur_db) {
        std::fprintf(stderr, "[Error] No database selected.\n");
        // 日志记录失败
        Logger::get_instance()->log_error(OperationType::TABLE_ALTER_MODIFY, 
            Logger::format_alter_modify_sql(table_name, field ? field->name : "unknown"),
            "No database selected");
        return;
    }
    
    cur_db->alter_table_modify_column(table_name, field);
    // 日志记录成功
    std::string sql = Logger::format_alter_modify_sql(table_name, field->name);
    Logger::get_instance()->log(LogLevel::INFO, OperationType::TABLE_ALTER_MODIFY, sql, true,
        std::string("Column '") + field->name + "' modified in table '" + table_name + "'", table_name);
}

void dbms::alter_table_rename_column(const char* table_name, const char* old_name, const char* new_name)
{
    if (!cur_db) {
        std::fprintf(stderr, "[Error] No database selected.\n");
        // 日志记录失败
        Logger::get_instance()->log_error(OperationType::TABLE_ALTER_RENAME,
            Logger::format_alter_rename_sql(table_name, old_name, new_name),
            "No database selected");
        return;
    }
    
    cur_db->alter_table_rename_column(table_name, old_name, new_name);
    // 日志记录成功
    std::string sql = Logger::format_alter_rename_sql(table_name, old_name, new_name);
    Logger::get_instance()->log(LogLevel::INFO, OperationType::TABLE_ALTER_RENAME, sql, true,
        std::string("Column '") + old_name + "' renamed to '" + new_name + "' in table '" + table_name + "'", table_name);
}

void dbms::update_rows(const update_info_t* info)
{
    if (!assert_db_open())
        return;

    __cache_clear_guard __guard;
    table_manager* tm = cur_db->get_table(info->table);
    if (tm == nullptr)
    {
        std::fprintf(stderr, "[Error] table `%s` doesn't exists.\n", info->table);
        return;
    }

    int col_id = tm->lookup_column(info->column_ref->column);
    if (col_id < 0)
    {
        std::fprintf(stderr, "[Error] column `%s' not exists.\n", info->column_ref->column);
        return;
    }

    int succ_count = 0, fail_count = 0;
    try {
        iterate_one_table(tm, info->where, [&](table_manager* tm, record_manager*, int rid) -> bool {
            expression val = expression::eval(info->value);
            int col_type = tm->get_column_type(col_id);
            if (!typecast::type_compatible(col_type, val))
                throw "[Error] Incompatible data type.";
            auto term_type = typecast::column_to_term(col_type);
            bool ret = tm->modify_record(rid, col_id, typecast::expr_to_db(val, term_type));
            // if(!ret) return false;
            succ_count += ret;
            fail_count += 1 - ret;
            return true;
            });
    }
    catch (const char* msg) {
        std::puts(msg);
        // 日志记录异常
        Logger::get_instance()->log_exception("dbms::update_rows", msg);
        return;
    }
    catch (...) {
        // 日志记录未知异常
        Logger::get_instance()->log_exception("dbms::update_rows", "Unknown exception");
    }

    std::printf("[Info] %d row(s) updated, %d row(s) failed.\n",
        succ_count, fail_count);
    
    // 日志记录
    std::string sql = Logger::format_update_sql(info->table, info->column_ref->column);
    Logger::get_instance()->log_data_op(OperationType::DATA_UPDATE, info->table, sql, 
        (fail_count == 0), succ_count,
        fail_count > 0 ? std::to_string(fail_count) + " row(s) failed" : "");
}

void dbms::select_rows(const select_info_t* info)
{
    if (!assert_db_open())
        return;

    __cache_clear_guard __guard;

    // get required tables
    std::vector<std::shared_ptr<table_manager>> alias_tables;
    std::vector<table_manager*> required_tables;
    for (linked_list_t* table_l = info->tables; table_l; table_l = table_l->next)
    {
        table_join_info_t* table_info = (table_join_info_t*)table_l->data;
        table_manager* tm = cur_db->get_table(table_info->table);
        if (tm == nullptr)
        {
            std::fprintf(stderr, "[Error] table `%s` doesn't exists.\n", table_info->table);
            return;
        }
        else {
            if (table_info->alias == nullptr)
            {
                required_tables.push_back(tm);
            }
            else {
                auto alias = tm->mirror(table_info->alias);
                alias_tables.push_back(alias);
                required_tables.push_back(alias.get());
            }
        }
    }

    // get select expression name
    std::vector<expr_node_t*> exprs;
    std::vector<std::string> expr_names;
    bool is_aggregate = false;
    for (linked_list_t* link_p = info->exprs; link_p; link_p = link_p->next)
    {
        expr_node_t* expr = (expr_node_t*)link_p->data;
        is_aggregate |= expression::is_aggregate(expr);
        exprs.push_back(expr);
        expr_names.push_back(expression::to_string(expr));
    }

    // 反转表达式顺序，因为解析器使用头插法构建链表，导致顺序与输入相反
    std::reverse(exprs.begin(), exprs.end());
    std::reverse(expr_names.begin(), expr_names.end());

    // output header info
    for (size_t i = 0; i < exprs.size(); ++i)
    {
        if (i != 0) std::fprintf(output_file, ",");
        std::fprintf(output_file, "%s", expr_names[i].c_str());
    }

    if (exprs.size() == 0)
    {
        // SELECT * 时，反向遍历列以保持定义顺序（存储顺序是反的）
        bool first_col = true;
        for (size_t i = 0; i < required_tables.size(); ++i)
        {
            table_manager* table = required_tables[i];
            int col_count = table->get_column_num();
            const char* table_name = table->get_table_name();
            // 排除 __rowid__ 列，并反向遍历以保持列定义顺序
            for (int j = col_count - 2; j >= 0; --j) {
                if (!first_col) std::fprintf(output_file, ",");
                first_col = false;
                std::fprintf(output_file, "%s.%s", table_name, table->get_column_name(j));
            }
        }
    }

    std::fprintf(output_file, "\n");

    if (is_aggregate)
    {
        select_rows_aggregate(
            info,
            required_tables,
            exprs,
            expr_names
        );

        return;
    }

    // ============ ORDER BY 支持 ============
    if (info->order_by != nullptr)
    {
        // 用于存储所有行的数据
        struct RowData {
            std::vector<expression> values;  // 存储 expression 对象（深拷贝）
            std::string row_string;          // 用于去重的整行字符串
        };

        std::vector<RowData> ordered_rows;
        std::unordered_set<std::string> seen_rows;

        // 确定要计算的表达式列表
        std::vector<expr_node_t*> actual_exprs = exprs;
        std::vector<std::shared_ptr<expr_node_t>> temp_exprs_holder;

        if (exprs.size() == 0) {
            // SELECT * 的情况，需要为所有列创建表达式
            for (size_t i = 0; i < required_tables.size(); ++i) {
                table_manager* table = required_tables[i];
                int col_count = table->get_column_num();
                // 排除 __rowid__ 列，并反向遍历以保持列定义顺序（存储顺序是反的）
                for (int j = col_count - 2; j >= 0; --j) {
                    const char* col_name = table->get_column_name(j);

                    column_ref_t* col_ref = new column_ref_t;
                    col_ref->table = nullptr;
                    col_ref->column = strdup(col_name);

                    expr_node_t* col_expr = new expr_node_t;
                    col_expr->term_type = TERM_COLUMN_REF;
                    col_expr->column_ref = col_ref;

                    actual_exprs.push_back(col_expr);
                    temp_exprs_holder.push_back(std::shared_ptr<expr_node_t>(col_expr,
                        [](expr_node_t* p) {
                            if (p->term_type == TERM_COLUMN_REF && p->column_ref) {
                                free(p->column_ref->column);
                                delete p->column_ref;
                            }
                            delete p;
                        }));
                }
            }
        }

        // 第一步：收集所有符合条件的行
        iterate(required_tables, info->where,
            [&](const std::vector<table_manager*>& tables,
                const std::vector<record_manager*>& records,
                const std::vector<int>&)
            {
                RowData rd;
                std::string current_row;

                for (size_t i = 0; i < actual_exprs.size(); ++i)
                {
                    expression ret;
                    try {
                        ret = expression::eval(actual_exprs[i]);
                    }
                    catch (const char* e) {
                        std::fprintf(stderr, "%s\n", e);
                        return false;
                    }

                    // 使用深拷贝
                    rd.values.push_back(expression::copy(ret));

                    // 构建去重字符串
                    std::string value_str;
                    switch (ret.type) {
                    case TERM_INT:
                        value_str = std::to_string(ret.val_i);
                        break;
                    case TERM_FLOAT: {
                        char buf[64];
                        std::snprintf(buf, sizeof(buf), "%f", ret.val_f);
                        value_str = buf;
                        break;
                    }
                    case TERM_STRING:
                        value_str = ret.val_s ? ret.val_s : "NULL";
                        break;
                    case TERM_BOOL:
                        value_str = ret.val_b ? "TRUE" : "FALSE";
                        break;
                    case TERM_DATE: {
                        char date_buf[32];
                        time_t time = ret.val_i;
                        auto tm = std::localtime(&time);
                        std::strftime(date_buf, 32, DATE_TEMPLATE, tm);
                        value_str = date_buf;
                        break;
                    }
                    case TERM_NULL:
                        value_str = "NULL";
                        break;
                    default:
                        // 注意：此 default 分支用于处理未预期的类型值（如 TERM_NONE=0 或 TERM_COLUMN_REF 等）
                        // 正常情况下不应到达此处，若到达说明 expression::eval() 返回了非终结类型
                        // 这不是错误，只是标记为 UNKNOWN 继续处理，不影响最终输出
                        value_str = "UNKNOWN";
                    }

                    if (i != 0) current_row += "|";
                    current_row += value_str;
                }

                rd.row_string = current_row;

                if (info->distinct)
                {
                    if (seen_rows.find(current_row) != seen_rows.end())
                    {
                        return true;
                    }
                    seen_rows.insert(current_row);
                }

                ordered_rows.push_back(std::move(rd));
                return true;
            }
        );

        // 第二步：排序
        if (!ordered_rows.empty())
        {
            auto compare_rows = [&](const RowData& a, const RowData& b) -> bool {
                order_by_item_t* order_item = info->order_by;

                while (order_item != nullptr) {
                    int expr_index = -1;

                    // 找到排序列的索引
                    for (size_t i = 0; i < actual_exprs.size(); ++i) {
                        if (actual_exprs[i]->term_type == TERM_COLUMN_REF) {
                            column_ref_t* ref = actual_exprs[i]->column_ref;
                            if (ref && ref->column && std::string(ref->column) == order_item->column_name) {
                                expr_index = i;
                                break;
                            }
                        }
                    }

                    if (expr_index == -1 || expr_index >= (int)a.values.size()) {
                        order_item = order_item->next;
                        continue;
                    }

                    const expression& val_a = a.values[expr_index];
                    const expression& val_b = b.values[expr_index];

                    // 处理 NULL
                    if (val_a.type == TERM_NULL && val_b.type == TERM_NULL) {
                        order_item = order_item->next;
                        continue;
                    }
                    if (val_a.type == TERM_NULL) return true;   // NULL 在最前面
                    if (val_b.type == TERM_NULL) return false;  // NULL 在最前面

                    // 比较
                    if (val_a.type == TERM_STRING && val_b.type == TERM_STRING) {
                        const char* str_a = val_a.val_s ? val_a.val_s : "";
                        const char* str_b = val_b.val_s ? val_b.val_s : "";
                        int cmp = strcmp(str_a, str_b);
                        if (cmp < 0) return order_item->ascending == 1;
                        if (cmp > 0) return order_item->ascending == 0;
                        // 相等，继续下一个排序列
                    }
                    else if (val_a.type == TERM_INT && val_b.type == TERM_INT) {
                        if (val_a.val_i < val_b.val_i) return order_item->ascending == 1;
                        if (val_a.val_i > val_b.val_i) return order_item->ascending == 0;
                        // 相等，继续下一个排序列
                    }
                    else if (val_a.type == TERM_FLOAT && val_b.type == TERM_FLOAT) {
                        if (val_a.val_f < val_b.val_f) return order_item->ascending == 1;
                        if (val_a.val_f > val_b.val_f) return order_item->ascending == 0;
                        // 相等，继续下一个排序列
                    }
                    else if (val_a.type == TERM_BOOL && val_b.type == TERM_BOOL) {
                        if (!val_a.val_b && val_b.val_b) return order_item->ascending == 1;  // false < true
                        if (val_a.val_b && !val_b.val_b) return order_item->ascending == 0;  // true > false
                        // 相等，继续下一个排序列
                    }
                    else if (val_a.type == TERM_DATE && val_b.type == TERM_DATE) {
                        if (val_a.val_i < val_b.val_i) return order_item->ascending == 1;
                        if (val_a.val_i > val_b.val_i) return order_item->ascending == 0;
                        // 相等，继续下一个排序列
                    }
                    else {
                        // 类型不匹配，跳过这个排序列
                        order_item = order_item->next;
                        continue;
                    }

                    order_item = order_item->next;
                }

                return false;  // 所有排序列都相等
                };

            std::stable_sort(ordered_rows.begin(), ordered_rows.end(), compare_rows);
        }

        // 第三步：输出
        int counter = 0;
        for (const auto& row : ordered_rows)
        {
            for (size_t i = 0; i < row.values.size(); ++i)
            {
                if (i != 0) std::fprintf(output_file, ",");

                const expression& ret = row.values[i];
                switch (ret.type)
                {
                case TERM_INT:
                    std::fprintf(output_file, "%d", ret.val_i);
                    break;
                case TERM_FLOAT:
                    std::fprintf(output_file, "%f", ret.val_f);
                    break;
                case TERM_STRING:
                    std::fprintf(output_file, "%s", ret.val_s ? ret.val_s : "NULL");
                    break;
                case TERM_BOOL:
                    std::fprintf(output_file, "%s", ret.val_b ? "TRUE" : "FALSE");
                    break;
                case TERM_DATE: {
                    char date_buf[32];
                    time_t time = ret.val_i;
                    auto tm = std::localtime(&time);
                    std::strftime(date_buf, 32, DATE_TEMPLATE, tm);
                    std::fprintf(output_file, "%s", date_buf);
                    break;
                }
                case TERM_NULL:
                    std::fprintf(output_file, "NULL");
                    break;
                default:
                    std::fprintf(stderr, "[Error] Data type not supported!\n");
                }
            }

            std::fprintf(output_file, "\n");
            ++counter;
        }

        std::printf("[Info] %d row(s) selected.\n", counter);
        std::fprintf(output_file, "\n");
        std::fflush(output_file);
    }
    else
    {
        // ============ 原来的非 ORDER BY 逻辑 ============
        // 用于存储已经出现过的行（去重）
        std::unordered_set<std::string> seen_rows;

        // 确定要计算的表达式列表（移到 lambda 外部，避免每行重复创建）
        std::vector<expr_node_t*> actual_exprs = exprs;
        std::vector<std::shared_ptr<expr_node_t>> temp_exprs_holder; // 使用 shared_ptr 自动管理内存

        if (exprs.size() == 0) {
            // SELECT * 的情况，需要为所有列创建表达式
            printf("[DEBUG] SELECT * detected, creating expressions for all columns\n");
            for (size_t i = 0; i < required_tables.size(); ++i) {
                table_manager* table = required_tables[i];
                int col_count = table->get_column_num();
                printf("[DEBUG] Table %s has %d columns\n", table->get_table_name(), col_count);
                // 排除 __rowid__ 列，并反向遍历以保持列定义顺序（存储顺序是反的）
                for (int j = col_count - 2; j >= 0; --j) {
                    const char* col_name = table->get_column_name(j);
                    printf("[DEBUG] Creating expression for column %s\n", col_name);

                    column_ref_t* col_ref = new column_ref_t;
                    col_ref->table = nullptr;
                    col_ref->column = strdup(col_name);

                    expr_node_t* col_expr = new expr_node_t;
                    col_expr->term_type = TERM_COLUMN_REF;
                    col_expr->column_ref = col_ref;

                    actual_exprs.push_back(col_expr);
                    temp_exprs_holder.push_back(std::shared_ptr<expr_node_t>(col_expr,
                        [](expr_node_t* p) {
                            if (p->term_type == TERM_COLUMN_REF && p->column_ref) {
                                free(p->column_ref->column);
                                delete p->column_ref;
                            }
                            delete p;
                        }));
                }
            }
            printf("[DEBUG] Created %zu expressions for SELECT *\n", actual_exprs.size());
        }

        // 遍历记录
        int counter = 0;
        iterate(required_tables, info->where,
            [&](const std::vector<table_manager*>& tables,
                const std::vector<record_manager*>& records,
                const std::vector<int>&)
            {
                // 构建当前行的字符串表示
                std::string current_row;

                // 为 DISTINCT 构建行字符串
                for (size_t i = 0; i < actual_exprs.size(); ++i) {
                    printf("[DEBUG] Evaluating expression %zu: ", i);
                    if (actual_exprs[i]->term_type == TERM_COLUMN_REF) {
                        printf("column %s.%s\n",
                            actual_exprs[i]->column_ref->table ? actual_exprs[i]->column_ref->table : "NULL",
                            actual_exprs[i]->column_ref->column);
                    }

                    expression ret;
                    try {
                        ret = expression::eval(actual_exprs[i]);
                        printf("[DEBUG] Result type: %d\n", ret.type);
                    }
                    catch (const char* e) {
                        std::fprintf(stderr, "%s\n", e);
                        // 内存由 shared_ptr 自动管理，无需手动清理
                        return false;
                    }

                    // 将值转换为字符串用于 DISTINCT
                    std::string value_str;
                    switch (ret.type)
                    {
                    case TERM_INT:
                        value_str = std::to_string(ret.val_i);
                        printf("[DEBUG]   int value: %d\n", ret.val_i);
                        break;
                    case TERM_FLOAT: {
                        char buf[64];
                        std::snprintf(buf, sizeof(buf), "%f", ret.val_f);
                        value_str = buf;
                        printf("[DEBUG]   float value: %f\n", ret.val_f);
                        break;
                    }
                    case TERM_STRING:
                        value_str = ret.val_s ? ret.val_s : "NULL";
                        printf("[DEBUG]   string value: %s\n", ret.val_s ? ret.val_s : "NULL");
                        break;
                    case TERM_BOOL:
                        value_str = ret.val_b ? "TRUE" : "FALSE";
                        printf("[DEBUG]   bool value: %s\n", ret.val_b ? "TRUE" : "FALSE");
                        break;
                    case TERM_DATE: {
                        char date_buf[32];
                        time_t time = ret.val_i;
                        auto tm = std::localtime(&time);
                        std::strftime(date_buf, 32, DATE_TEMPLATE, tm);
                        value_str = date_buf;
                        printf("[DEBUG]   date value: %s\n", date_buf);
                        break;
                    }
                    case TERM_NULL:
                        value_str = "NULL";
                        printf("[DEBUG]   null value\n");
                        break;
                    default:
                        // 注意：此 default 分支用于处理未预期的类型值（如 TERM_NONE=0 或 TERM_COLUMN_REF 等）
                        // 正常情况下不应到达此处，若到达说明 expression::eval() 返回了非终结类型
                        // 这不是错误，只是标记为 UNKNOWN 继续处理，不影响最终输出
                        value_str = "UNKNOWN";
                        printf("[DEBUG]   unknown type\n");
                    }

                    if (i != 0) current_row += "|";
                    current_row += value_str;
                }

                // 检查是否需要去重
                if (info->distinct)
                {
                    // 检查这一行是否已经出现过
                    if (seen_rows.find(current_row) != seen_rows.end())
                    {
                        // 重复行，跳过（内存由 shared_ptr 自动管理）
                        return true;
                    }

                    // 记录这一行
                    seen_rows.insert(current_row);
                }

                // 输出这一行（统一使用 actual_exprs，确保与 DISTINCT 计算一致）
                for (size_t i = 0; i < actual_exprs.size(); ++i)
                {
                    expression ret;
                    try {
                        ret = expression::eval(actual_exprs[i]);
                    }
                    catch (const char* e) {
                        std::fprintf(stderr, "%s\n", e);
                        // 内存由 shared_ptr 自动管理，无需手动清理
                        return false;
                    }

                    if (i != 0) std::fprintf(output_file, ",");
                    switch (ret.type)
                    {
                    case TERM_INT:
                        std::fprintf(output_file, "%d", ret.val_i);
                        break;
                    case TERM_FLOAT:
                        std::fprintf(output_file, "%f", ret.val_f);
                        break;
                    case TERM_STRING:
                        std::fprintf(output_file, "%s", ret.val_s);
                        break;
                    case TERM_BOOL:
                        std::fprintf(output_file, "%s", ret.val_b ? "TRUE" : "FALSE");
                        break;
                    case TERM_DATE: {
                        char date_buf[32];
                        time_t time = ret.val_i;
                        auto tm = std::localtime(&time);
                        std::strftime(date_buf, 32, DATE_TEMPLATE, tm);
                        std::fprintf(output_file, "%s", date_buf);
                        break;
                    }
                    case TERM_NULL:
                        std::fprintf(output_file, "NULL");
                        break;
                    default:
                        std::fprintf(stderr, "[Error] Data type not supported!\n");
                    }
                }

                // SELECT * 现在统一通过 actual_exprs 处理，不再需要 dump_record

                std::fprintf(output_file, "\n");
                ++counter;

                // 内存由 shared_ptr 自动管理，无需手动清理
                return true;
            }
        );

        std::printf("[Info] %d row(s) selected.\n", counter);
        std::fprintf(output_file, "\n");
        std::fflush(output_file);
    }
    
    // 日志记录（获取第一个表名）
    table_join_info_t *first_table = (table_join_info_t*)info->tables->data;
    std::string sql = Logger::format_select_sql(first_table->table);
    Logger::get_instance()->log_data_op(OperationType::DATA_SELECT, first_table->table, sql, true, 0);
}

void dbms::select_rows_aggregate(
    const select_info_t* info,
    const std::vector<table_manager*>& required_tables,
    const std::vector<expr_node_t*>& exprs,
    const std::vector<std::string>&)
{
    if (exprs.size() != 1)
    {
        std::fprintf(stderr, "[Error] Support only for one select expression for aggregate select.");
        return;
    }

    // check aggregate type
    expr_node_t* expr = exprs[0];
    int val_i = 0;
    float val_f = 0;
    if (expr->op == OPERATOR_MIN)
    {
        val_i = std::numeric_limits<int>::max();
        val_f = std::numeric_limits<float>::max();
    }
    else if (expr->op == OPERATOR_MAX) {
        val_i = std::numeric_limits<int>::min();
        val_f = std::numeric_limits<float>::min();
    }

    term_type_t agg_type = TERM_NONE;

    int counter = 0;
    iterate(required_tables, info->where,
        [&](const std::vector<table_manager*>&,
            const std::vector<record_manager*>&,
            const std::vector<int>&)
        {
            if (expr->op != OPERATOR_COUNT)
            {
                expression ret;
                try {
                    ret = expression::eval(expr->left);
                }
                catch (const char* e) {
                    std::fprintf(stderr, "%s\n", e);
                    return false;
                }

                agg_type = ret.type;
                if (ret.type == TERM_FLOAT)
                {
                    switch (expr->op)
                    {
                    case OPERATOR_SUM:
                    case OPERATOR_AVG:
                        val_f += ret.val_f;
                        break;
                    case OPERATOR_MIN:
                        if (ret.val_f < val_f)
                            val_f = ret.val_f;
                        break;
                    case OPERATOR_MAX:
                        if (ret.val_f > val_f)
                            val_f = ret.val_f;
                        break;
                    default: break;
                    }
                }
                else {
                    switch (expr->op)
                    {
                    case OPERATOR_SUM:
                    case OPERATOR_AVG:
                        val_i += ret.val_i;
                        break;
                    case OPERATOR_MIN:
                        if (ret.val_i < val_i)
                            val_i = ret.val_i;
                        break;
                    case OPERATOR_MAX:
                        if (ret.val_i > val_i)
                            val_i = ret.val_i;
                        break;
                    default: break;
                    }
                }
            }

            ++counter;
            return true;
        }
    );

    if (expr->op == OPERATOR_COUNT)
    {
        std::fprintf(output_file, "%d\n", counter);
    }
    else {
        if (agg_type != TERM_FLOAT && agg_type != TERM_INT)
        {
            std::fprintf(stderr, "[Error] Aggregate only support for int and float type.\n");
            return;
        }

        if (expr->op == OPERATOR_AVG)
        {
            if (agg_type == TERM_INT)
                val_f = double(val_i) / counter;
            else val_f /= counter;
            std::fprintf(output_file, "%f\n", val_f);
        }
        else if (agg_type == TERM_FLOAT) {
            std::fprintf(output_file, "%f\n", val_f);
        }
        else if (agg_type == TERM_INT) {
            std::fprintf(output_file, "%d\n", val_i);
        }
    }

    std::printf("[Info] %d row(s) selected.\n", counter);
    std::fprintf(output_file, "\n");
    std::fflush(output_file);
}

void dbms::delete_rows(const delete_info_t* info)
{
    if (!assert_db_open())
        return;
    __cache_clear_guard __guard;

    std::vector<int> delete_list;
    table_manager* tm = cur_db->get_table(info->table);
    if (tm == nullptr)
    {
        std::fprintf(stderr, "[Error] table `%s` doesn't exists.\n", info->table);
        return;
    }

    iterate_one_table_with_index(tm, info->where,
        [&delete_list](table_manager*, record_manager*, int rid) -> bool {
            delete_list.push_back(rid);
            return true;
        });

    int counter = 0;
    for (int rid : delete_list)
        counter += tm->remove_record(rid);
    std::printf("[Info] %d row(s) deleted.\n", counter);
    
    // 日志记录
    std::string sql = Logger::format_delete_sql(info->table);
    Logger::get_instance()->log_data_op(OperationType::DATA_DELETE, info->table, sql, true, counter);
}

void dbms::insert_rows(const insert_info_t* info)
{
    if (!assert_db_open())
        return;
    __cache_clear_guard __guard;

    table_manager* tb = cur_db->get_table(info->table);
    if (tb == nullptr)
    {
        std::fprintf(stderr, "[Error] table `%s` not found.\n", info->table);
        return;
    }

    std::vector<int> cols_id;
    if (info->columns == nullptr)
    {
        // exclude __rowid__, which has the largest index
        for (int i = 0; i < tb->get_column_num() - 1; ++i)
            cols_id.push_back(i);
    }
    else {
        for (linked_list_t* link_ptr = info->columns; link_ptr; link_ptr = link_ptr->next)
        {
            column_ref_t* column = (column_ref_t*)link_ptr->data;
            int cid = tb->lookup_column(column->column);
            if (cid < 0)
            {
                std::fprintf(stderr, "[Error] No column `%s` in table `%s`.\n",
                    column->column, tb->get_table_name());
                return;
            }
            cols_id.push_back(cid);
        }
    }

    int count_succ = 0, count_fail = 0;
    for (linked_list_t* list = info->values; list; list = list->next)
    {
        tb->init_temp_record();
        linked_list_t* expr_list = (linked_list_t*)list->data;
        unsigned val_num = 0;
        for (linked_list_t* i = expr_list; i; i = i->next, ++val_num);
        if (val_num != cols_id.size())
        {
            std::fprintf(stderr, "[Error] column size not equal.");
            continue;
        }

        bool succ = true;
        for (auto it = cols_id.begin(); expr_list; expr_list = expr_list->next, ++it)
        {
            expression v;
            try {
                v = expression::eval((expr_node_t*)expr_list->data);
            }
            catch (const char* e) {
                std::fprintf(stderr, "%s\n", e);
                return;
            }

            auto col_type = tb->get_column_type(*it);
            if (!typecast::type_compatible(col_type, v))
            {
                std::fprintf(stderr, "[Error] incompatible type.\n");
                return;
            }

            term_type_t desired_type = typecast::column_to_term(col_type);
            char* db_val = typecast::expr_to_db(v, desired_type);
            if (!tb->set_temp_record(*it, db_val))
            {
                succ = false;
                break;
            }
        }

        if (succ) succ = (tb->insert_record() > 0);
        count_succ += succ;
        count_fail += 1 - succ;
    }

    std::printf("[Info] %d row(s) inserted, %d row(s) failed.\n", count_succ, count_fail);
    
    // 日志记录
    std::string sql = Logger::format_insert_sql(info->table, count_succ);
    Logger::get_instance()->log_data_op(OperationType::DATA_INSERT, info->table, sql,
        (count_fail == 0), count_succ,
        count_fail > 0 ? std::to_string(count_fail) + " row(s) failed" : "");
}

void dbms::drop_index(const char* tb_name, const char* col_name)
{
	// 日志记录
	std::string sql = Logger::format_drop_index_sql(tb_name, col_name);
	Logger::get_instance()->log(LogLevel::INFO, OperationType::INDEX_DROP, sql, true,
		std::string("Index on ") + tb_name + "(" + col_name + ") dropped", tb_name);
}

void dbms::create_index(const char* tb_name, const char* col_name)
{
    if (!assert_db_open())
        return;
    table_manager* tb = cur_db->get_table(tb_name);
    if (tb == nullptr)
    {
        std::fprintf(stderr, "[Error] table `%s` not exists.\n", tb_name);
        // 日志记录失败
        Logger::get_instance()->log_error(OperationType::INDEX_CREATE,
            Logger::format_create_index_sql(tb_name, col_name),
            std::string("Table '") + tb_name + "' not exists");
    }
    else {
        tb->create_index(col_name);
        // 日志记录成功
        std::string sql = Logger::format_create_index_sql(tb_name, col_name);
        Logger::get_instance()->log(LogLevel::INFO, OperationType::INDEX_CREATE, sql, true,
            std::string("Index created on ") + tb_name + "(" + col_name + ")", tb_name);
    }
}

bool dbms::assert_db_open()
{
    if (cur_db && cur_db->is_opened())
        return true;
    std::fprintf(stderr, "[Error] database is not opened.\n");
    return false;
}

expr_node_t* dbms::get_join_cond(expr_node_t* cond)
{
    if (!cond) return nullptr;
    if (cond->left->term_type == TERM_COLUMN_REF && cond->right->term_type == TERM_COLUMN_REF)
    {
        return cond;
    }
    else {
        return nullptr;
    }
}

bool dbms::find_longest_path(int now, int depth, int* mark, int* path, std::vector<std::vector<int>>& E, int excepted_len, int& max_depth)
{
    mark[now] = 1;
    path[depth] = now;
    if (depth > max_depth)
        max_depth = depth;
    if (depth == excepted_len)
        return true;
    for (int i = 0; i != (int)E.size(); ++i)
    {
        if (!E[now][i] || mark[i]) continue;
        if (find_longest_path(i, depth + 1, mark, path, E, excepted_len, max_depth))
            return true;
    }

    mark[now] = 0;
    return false;
}

bool dbms::value_exists(const char* table, const char* column, const char* data)
{
    if (!assert_db_open())
        return false;
    table_manager* tm = cur_db->get_table(table);
    if (tm == nullptr)
    {
        std::printf("[Error] No table named `%s`\n", table);
        return false;
    }

    return tm->value_exists(column, data);
}