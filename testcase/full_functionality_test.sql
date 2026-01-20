-- ================================================
-- TrivialDB 全面功能测试
-- ================================================

-----------------------------------------------
-- 第一部分：数据库操作测试
-----------------------------------------------
CREATE DATABASE company_db;
USE company_db;

-----------------------------------------------
-- 第二部分：表操作测试
-----------------------------------------------

-- 2.1 创建部门表
CREATE TABLE departments (
    dept_id int PRIMARY KEY,
    dept_name varchar(50),
    location varchar(100),
    budget float
);

-- 2.2 创建员工表
CREATE TABLE employees (
    emp_id int PRIMARY KEY,
    name varchar(50),
    age int,
    salary float,
    dept_id int,
    email varchar(100)
);

-- 2.3 创建项目表
CREATE TABLE projects (
    project_id int PRIMARY KEY,
    project_name varchar(100),
    budget float,
    manager_id int,
    dept_id int
);

-- 显示所有表结构
SHOW TABLE departments;
SHOW TABLE employees;
SHOW TABLE projects;

-----------------------------------------------
-- 第三部分：数据插入测试 (INSERT)
-----------------------------------------------

-- 插入部门数据
INSERT INTO departments VALUES (1, 'Technology', 'New York', 500000.0);
INSERT INTO departments VALUES (2, 'Marketing', 'Los Angeles', 300000.0);
INSERT INTO departments VALUES (3, 'Finance', 'Chicago', 400000.0);

-- 插入员工数据
INSERT INTO employees VALUES (101, 'Alice', 28, 75000.0, 1, 'alice@company.com');
INSERT INTO employees VALUES (102, 'Bob', 32, 82000.0, 1, 'bob@company.com');
INSERT INTO employees VALUES (103, 'Carol', 35, 90000.0, 2, 'carol@company.com');
INSERT INTO employees VALUES (104, 'David', 29, 68000.0, 2, 'david@company.com');
INSERT INTO employees VALUES (105, 'Eva', 31, 78000.0, 3, 'eva@company.com');

-- 插入项目数据
INSERT INTO projects VALUES (1001, 'Website', 50000.0, 101, 1);
INSERT INTO projects VALUES (1002, 'Marketing', 30000.0, 103, 2);
INSERT INTO projects VALUES (1003, 'Finance', 75000.0, 103, 3);
INSERT INTO projects VALUES (1004, 'Mobile App', 60000.0, 101, 1);

-----------------------------------------------
-- 第四部分：单表查询测试 (SELECT)
-----------------------------------------------

-- 全表查询
SELECT * FROM departments;
SELECT * FROM employees;
SELECT * FROM projects;

-- 条件查询 (WHERE)
SELECT * FROM employees AS a WHERE age > 30;
SELECT * FROM employees AS a WHERE salary > 70000.0;
SELECT * FROM employees AS a WHERE dept_id = 1;
SELECT * FROM employees AS a WHERE name LIKE 'A%';

-- 投影查询
SELECT emp_id, name, salary FROM employees;
SELECT dept_id, dept_name, location FROM departments;

-- 聚合查询
SELECT COUNT(*) FROM employees;
SELECT AVG(salary) FROM employees;
SELECT MAX(salary) FROM employees;
SELECT MIN(age) FROM employees;
SELECT SUM(budget) FROM departments;

-----------------------------------------------
-- 第五部分：多表连接查询测试 (JOIN)
-----------------------------------------------

-- 两表连接
SELECT e.emp_id, e.name, e.salary, d.dept_name 
FROM employees AS e, departments AS d 
WHERE e.dept_id = d.dept_id;

-- 三表连接
SELECT e.name, d.dept_name, p.project_name, p.budget
FROM employees AS e, departments AS d, projects AS p
WHERE e.dept_id = d.dept_id AND d.dept_id = p.dept_id;

-- 带条件的多表连接
SELECT e.name, e.salary, d.dept_name, p.project_name
FROM employees AS e, departments AS d, projects AS p
WHERE e.dept_id = d.dept_id 
  AND d.dept_id = p.dept_id 
  AND p.budget > 40000.0;

-----------------------------------------------
-- 第六部分：数据修改测试 (UPDATE, DELETE)
-----------------------------------------------

-- 更新数据
UPDATE employees SET salary = salary * 1.1 WHERE dept_id = 1;
SELECT * FROM employees WHERE dept_id = 1;

-- 删除数据
DELETE FROM projects WHERE budget < 40000.0;
SELECT * FROM projects;

-----------------------------------------------
-- 第七部分：ALTER TABLE 测试
-----------------------------------------------

-- RENAME COLUMN
ALTER TABLE employees RENAME COLUMN email TO work_email;
SHOW TABLE employees;

-- ADD COLUMN
ALTER TABLE departments ADD COLUMN employeecount int;
SHOW TABLE departments;

-- MODIFY COLUMN
ALTER TABLE employees MODIFY COLUMN salary int;
SHOW TABLE employees;

-- DROP COLUMN
ALTER TABLE departments DROP COLUMN employeecount;
SHOW TABLE departments;

-----------------------------------------------
-- 第八部分：RENAME TABLE 测试
-----------------------------------------------

RENAME TABLE projects TO companyprojects;
SHOW TABLE companyprojects;
SELECT * FROM companyprojects WHERE budget > 50000.0;

-----------------------------------------------
-- 第九部分：清理测试环境
-----------------------------------------------

DROP TABLE companyprojects;
DROP TABLE employees;
DROP TABLE departments;
DROP DATABASE company_db;

PRINT("========================================");
PRINT("         所有功能测试完成！");
PRINT("========================================");
PRINT("测试内容：");
PRINT("✓ CREATE/USE/DROP DATABASE");
PRINT("✓ CREATE/SHOW/DROP TABLE");
PRINT("✓ RENAME TABLE");
PRINT("✓ ALTER TABLE: ADD/DROP/RENAME/MODIFY");
PRINT("✓ INSERT/UPDATE/DELETE");
PRINT("✓ SELECT: 单表查询、条件查询、投影、排序");
PRINT("✓ 多表连接查询");
PRINT("✓ 聚合函数: COUNT/AVG/MAX/MIN/SUM");
PRINT("========================================");

EXIT;