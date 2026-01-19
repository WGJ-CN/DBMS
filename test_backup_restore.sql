CREATE DATABASE test_db;
USE test_db;

CREATE TABLE students (
    id INT PRIMARY KEY,
    name VARCHAR(50) NOT NULL,
    age INT,
    score FLOAT
);

INSERT INTO students VALUES (1, 'Alice', 20, 95.5);
INSERT INTO students VALUES (2, 'Bob', 21, 88.0);
INSERT INTO students VALUES (3, 'Charlie', 22, 92.5);

SELECT * FROM students;

BACKUP "test_backup";

UPDATE students SET score = 100.0 WHERE id = 1;
DELETE FROM students WHERE id = 2;

SELECT * FROM students;

RESTORE "test_backup";

SELECT * FROM students;

DROP DATABASE test_db;
