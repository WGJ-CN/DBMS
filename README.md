# TrivialDB - 轻量级数据库管理系统

TrivialDB是一个功能完整的轻量级数据库管理系统，支持标准的SQL语法，提供图形化用户界面和命令行界面。

## ✨ 主要特性

- **完整的SQL支持**：支持DDL、DML、DQL等标准SQL操作
- **图形用户界面**：基于Python Tkinter的直观可视化界面
- **跨平台运行**：支持Windows和Linux系统
- **数据完整性**：支持主键、外键、UNIQUE、NOT NULL等约束
- **查询优化**：利用B+树索引优化查询性能
- **复杂查询**：支持多表连接、聚集查询、嵌套表达式

## 🚀 快速开始

### 环境要求
- Python 3.6+ (用于GUI)
- C++11兼容编译器
- CMake 3.10+
- Bison和Flex (用于SQL解析)

### 编译数据库核心

**Linux/WSL环境：**
```bash
mkdir build
cd build
cmake ..
make -j8
```

**Windows环境（交叉编译）：**
```bash
mkdir build-win
cd build-win
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=../mingw-toolchain.cmake \
    -DCMAKE_BUILD_TYPE=Release
make -j8
```

### 运行图形界面

**方式1：使用启动器（推荐）**
```bash
# 在项目根目录运行
python3 run_gui.py
```

**方式2：直接运行GUI**
```bash
cd src/gui
python3 trivialdb_gui.py
```

### 运行命令行界面Linux/WSL环境
```bash
# 进入编译目录
cd build/bin  
./trivial_db   
```

### 运行命令行界面windows环境
```bash
# 进入编译目录
cd build-win/bin  
双击trivial_db.exe   
```


## 📊 功能特性

### 支持的SQL语句
- ✅ `CREATE/DROP DATABASE` - 数据库管理
- ✅ `USE` - 数据库切换  
- ✅ `CREATE/DROP TABLE` - 表管理
- ✅ `INSERT/UPDATE/DELETE` - 数据操作
- ✅ `SELECT` - 数据查询（支持JOIN、WHERE、GROUP BY、HAVING）
- ✅ `ALTER TABLE` - 表结构修改（ADD/DROP/RENAME/MODIFY COLUMN）
- ✅ `RENAME TABLE` - 表重命名
- ✅ `SHOW DATABASE/TABLE` - 信息显示

### 数据类型支持
- **INT** - 整型
- **FLOAT** - 浮点型  
- **VARCHAR** - 变长字符串（任意长度）
- **DATE** - 日期类型（YYYY-MM-dd格式）

### 完整性约束
- 🔑 **PRIMARY KEY** - 主键约束（支持复合主键）
- 🔗 **FOREIGN KEY** - 外键约束
- 🔍 **UNIQUE** - 唯一性约束
- 🚫 **NOT NULL** - 非空约束
- ⚡ **DEFAULT** - 默认值约束
- ✓ **CHECK** - 条件检查约束

### 高级功能
- **多表连接查询** - 支持任意多表的JOIN操作
- **聚集函数** - COUNT、SUM、AVG、MIN、MAX
- **复杂表达式** - 支持嵌套算术和逻辑表达式
- **模糊查询** - LIKE运算符支持正则表达式
- **索引优化** - B+树索引加速查询
- **事务支持** - 基本的ACID特性

## 🖥️ 图形界面功能

### 数据库操作
- 创建/选择/删除数据库
- 实时状态显示

### 表管理  
- 可视化创建表（支持所有约束）
- 显示表结构
- 重命名表
- 修改表结构（添加/删除/重命名列）
- 删除表

### 数据操作
- 插入数据（支持批量插入）
- 查询数据（高级查询界面）
- 更新数据
- 删除数据

### SQL控制台
- 交互式SQL命令行
- 语法高亮和自动完成
- 执行结果显示

## 📁 项目结构

```
TrivialDB/
├── database/          # 数据库文件存储目录
├── src/
│   ├── algo/         # 算法模块（搜索、排序）
│   ├── btree/        # B+树索引实现
│   ├── database/     # 数据库核心模块
│   ├── expression/   # 表达式处理
│   ├── fs/          # 文件系统管理
│   ├── gui/         # 图形用户界面
│   ├── index/       # 索引管理
│   ├── page/        # 页面管理
│   ├── parser/      # SQL解析器
│   ├── table/       # 表管理
│   └── utils/       # 工具函数
├── testcase/        # 测试用例
├── build/          # Linux编译目录
├── build-win/      # Windows编译目录
├── run_gui.py      # GUI启动器
└── README.md       # 项目说明
```

## 🔧 开发指南

### 代码架构
项目采用模块化设计，核心模块包括：
1. **SQL解析器** - 基于Bison/Flex的词法和语法分析
2. **存储引擎** - 基于页面的文件存储管理
3. **索引模块** - B+树索引实现
4. **查询处理** - 表达式计算和查询优化
5. **事务管理** - 基本的并发控制

### 扩展开发
如需添加新功能，建议参考现有模块：
- 新SQL语法：修改`src/parser/`中的语法规则
- 新数据类型：在`src/table/`中添加类型支持
- 新约束类型：在`src/table/constraint`中实现
- GUI新功能：修改`src/gui/trivialdb_gui.py`

## 🧪 测试验证

项目包含完整的测试用例：
```bash
# 运行功能测试
cd testcase
full_functionality_test.sql文件为测试用例SQL语句
```

**TrivialDB** - 让数据库管理变得简单高效！ 🎯