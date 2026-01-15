-- 简单功能测试
CREATE DATABASE test_db;
USE test_db;

-- 创建表测试
CREATE TABLE test (id int, name varchar(20));
SHOW TABLE test;

-- 插入和查询测试
INSERT INTO test VALUES (1, 'Hello');
INSERT INTO test VALUES (2, 'World');
SELECT * FROM test;

-- RENAME COLUMN 测试
ALTER TABLE test RENAME COLUMN name TO new_name;
SHOW TABLE test;
SELECT * FROM test;

DROP TABLE test;
DROP DATABASE test_db;
EXIT;