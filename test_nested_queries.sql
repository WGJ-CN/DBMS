-- 创建测试数据库
CREATE DATABASE test_nested;
USE test_nested;

-- 创建测试表
CREATE TABLE employees (
    id INT PRIMARY KEY,
    name VARCHAR(50),
    department_id INT,
    salary FLOAT
);

CREATE TABLE departments (
    id INT PRIMARY KEY,
    name VARCHAR(50),
    location VARCHAR(50)
);

-- 插入测试数据
INSERT INTO departments VALUES (1, '研发部', '北京');
INSERT INTO departments VALUES (2, '市场部', '上海');
INSERT INTO departments VALUES (3, '财务部', '广州');

INSERT INTO employees VALUES (1, '张三', 1, 8000);
INSERT INTO employees VALUES (2, '李四', 1, 9000);
INSERT INTO employees VALUES (3, '王五', 2, 7000);
INSERT INTO employees VALUES (4, '赵六', 2, 8500);
INSERT INTO employees VALUES (5, '钱七', 3, 6500);

-- 测试1: FROM子句中的嵌套查询
SELECT e.name, d.avg_salary
FROM employees e,
     (SELECT department_id, AVG(salary) as avg_salary FROM employees GROUP BY department_id) d
WHERE e.department_id = d.department_id;

-- 测试2: WHERE子句中的IN子查询
SELECT name FROM employees
WHERE department_id IN (SELECT id FROM departments WHERE location = '北京');

-- 测试3: 复杂的嵌套查询（嵌套IN子查询）
SELECT name FROM employees
WHERE department_id IN (
    SELECT id FROM departments
    WHERE location IN ('北京', '上海')
);

-- 测试4: 带别名的FROM子查询
SELECT t.name, t.salary
FROM (
    SELECT name, salary FROM employees WHERE salary > 8000
) t;

-- 清理测试数据
DROP DATABASE test_nested;

EXIT;