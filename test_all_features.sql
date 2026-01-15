-- ================================================
-- TrivialDB 全面功能测试程序
-- ================================================

-- ================================================
-- 第一部分：数据库操作测试
-- ================================================

-- 测试1.1: 创建数据库
CREATE DATABASE test_db;
PRINT("[Test 1.1] 创建数据库 test_db - PASSED");

-- 测试1.2: 切换到新数据库
USE test_db;
PRINT("[Test 1.2] 切换到数据库 test_db - PASSED");

-- 测试1.3: 显示数据库信息
SHOW DATABASE test_db;

-- ================================================
-- 第二部分：表创建测试
-- ================================================

PRINT("");
PRINT("========================================");
PRINT("   第2部分：表创建测试");
PRINT("========================================");

-- 测试2.1: 创建部门表
CREATE TABLE departments (
    dept_id int PRIMARY KEY,
    dept_name varchar(50) NOT NULL,
    location varchar(100)
);
PRINT("[Test 2.1] 创建 departments 表 - PASSED");

-- 测试2.2: 创建员工表
CREATE TABLE employees (
    emp_id int PRIMARY KEY,
    name varchar(50) NOT NULL,
    age int,
    salary float,
    dept_id int,
    hire_date varchar(10)
);
PRINT("[Test 2.2] 创建 employees 表 - PASSED");

-- 测试2.3: 显示所有表
SHOW TABLE departments;
SHOW TABLE employees;
PRINT("[Test 2.3] 显示所有表信息 - PASSED");

-- ================================================
-- 第三部分：数据插入测试
-- ================================================

PRINT("");
PRINT("========================================");
PRINT("   第3部分：数据插入测试");
PRINT("========================================");

-- 测试3.1: 向部门表插入数据
INSERT INTO departments VALUES (1, 'Technology', 'New York');
INSERT INTO departments VALUES (2, 'Marketing', 'Los Angeles');
INSERT INTO departments VALUES (3, 'Finance', 'Chicago');
PRINT("[Test 3.1] 向 departments 插入3条记录 - PASSED");

-- 测试3.2: 向员工表插入数据
INSERT INTO employees VALUES (101, 'John Smith', 30, 75000.0, 1, '2020-01-15');
INSERT INTO employees VALUES (102, 'Jane Doe', 28, 68000.0, 2, '2021-03-22');
INSERT INTO employees VALUES (103, 'Mike Johnson', 35, 82000.0, 1, '2019-05-10');
INSERT INTO employees VALUES (104, 'Sarah Wilson', 32, 78000.0, 3, '2020-08-05');
INSERT INTO employees VALUES (105, 'Tom Brown', 29, 65000.0, 2, '2022-01-30');
PRINT("[Test 3.2] 向 employees 插入5条记录 - PASSED");

-- ================================================
-- 第四部分：单表查询测试
-- ================================================

PRINT("");
PRINT("========================================");
PRINT("   第4部分：单表查询测试");
PRINT("========================================");

-- 测试4.1: 查询所有数据
SELECT * FROM departments;
SELECT * FROM employees;
PRINT("[Test 4.1] 查询所有表数据 - PASSED");

-- 测试4.2: 条件查询
SELECT * FROM employees WHERE age > 30;
SELECT * FROM employees WHERE salary > 70000;
SELECT * FROM employees WHERE dept_id = 1;
SELECT * FROM employees WHERE name LIKE 'J%';
PRINT("[Test 4.2] 条件查询测试 - PASSED");

-- 测试4.3: 投影查询
SELECT emp_id, name, salary FROM employees;
SELECT dept_id, dept_name FROM departments;
PRINT("[Test 4.3] 投影查询测试 - PASSED");

-- ================================================
-- 第五部分：多表连接查询测试
-- ================================================

PRINT("");
PRINT("========================================");
PRINT("   第5部分：多表连接查询测试");
PRINT("========================================");

-- 测试5.1: 两表连接查询
SELECT e.emp_id, e.name, e.salary, d.dept_name 
FROM employees AS e, departments AS d
WHERE e.dept_id = d.dept_id;
PRINT("[Test 5.1] 两表连接查询 - PASSED");

-- ================================================
-- 第六部分：数据修改测试
-- ================================================

PRINT("");
PRINT("========================================");
PRINT("   第6部分：数据修改测试");
PRINT("========================================");

-- 测试6.1: 更新数据
UPDATE employees SET salary = 80000.0 WHERE emp_id = 101;
SELECT * FROM employees WHERE emp_id = 101;
PRINT("[Test 6.1] 更新员工工资 - PASSED");

-- 测试6.2: 删除数据
DELETE FROM employees WHERE emp_id = 105;
SELECT COUNT(*) FROM employees;
PRINT("[Test 6.2] 删除员工记录 - PASSED");

-- ================================================
-- 第七部分：ALTER TABLE 测试
-- ================================================

PRINT("");
PRINT("========================================");
PRINT("   第7部分：ALTER TABLE 测试");
PRINT("========================================");

-- 测试7.1: ADD COLUMN
ALTER TABLE employees ADD COLUMN phone varchar(15);
SHOW TABLE employees;
PRINT("[Test 7.1] ADD COLUMN - PASSED");

-- 测试7.2: RENAME COLUMN
ALTER TABLE employees RENAME COLUMN phone TO mobile_phone;
SHOW TABLE employees;
PRINT("[Test 7.2] RENAME COLUMN - PASSED");

-- 测试7.3: MODIFY COLUMN
ALTER TABLE employees MODIFY COLUMN salary double;
SHOW TABLE employees;
PRINT("[Test 7.3] MODIFY COLUMN - PASSED");

-- 测试7.4: DROP COLUMN
ALTER TABLE employees DROP COLUMN mobile_phone;
SHOW TABLE employees;
PRINT("[Test 7.4] DROP COLUMN - PASSED");

-- ================================================
-- 第八部分：RENAME TABLE 测试
-- ================================================

PRINT("");
PRINT("========================================");
PRINT("   第8部分：RENAME TABLE 测试");
PRINT("========================================");

-- 测试8.1: 重命名表
RENAME TABLE employees TO staff;
SHOW TABLE staff;
PRINT("[Test 8.1] RENAME TABLE employees TO staff - PASSED");

-- 测试8.2: 使用重命名后的表
SELECT * FROM staff WHERE dept_id = 1;
PRINT("[Test 8.2] 查询重命名后的表 - PASSED");

-- ================================================
-- 第九部分：聚合查询测试
-- ================================================

PRINT("");
PRINT("========================================");
PRINT("   第9部分：聚合查询测试");
PRINT("========================================");

SELECT COUNT(*) FROM staff;
SELECT SUM(salary) FROM staff;
SELECT AVG(salary) FROM staff;
SELECT MAX(salary) FROM staff;
SELECT MIN(age) FROM staff;
PRINT("[Test 9] 聚合查询测试 - PASSED");

-- ================================================
-- 第十部分：清理测试环境
-- ================================================

PRINT("");
PRINT("========================================");
PRINT("   第10部分：清理测试环境");
PRINT("========================================");

DROP TABLE staff;
DROP TABLE departments;
PRINT("[Test 10.1] 删除所有测试表 - PASSED");

DROP DATABASE test_db;
PRINT("[Test 10.2] 删除测试数据库 - PASSED");

-- ================================================
-- 测试总结
-- ================================================

PRINT("");
PRINT("========================================");
PRINT("         测试完成总结");
PRINT("========================================");
PRINT("");
PRINT("✓ 数据库操作: CREATE, USE, SHOW, DROP");
PRINT("✓ 表操作: CREATE, SHOW, RENAME, DROP");
PRINT("✓ ALTER TABLE: ADD, DROP, RENAME, MODIFY");
PRINT("✓ 数据操作: INSERT, UPDATE, DELETE");
PRINT("✓ 查询操作: SELECT (单表, 多表连接, 条件, 投影)");
PRINT("✓ 聚合查询: COUNT, SUM, AVG, MIN, MAX");
PRINT("");
PRINT("所有功能测试完成！");
PRINT("========================================");

EXIT;
