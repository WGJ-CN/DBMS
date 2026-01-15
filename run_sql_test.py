#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
TrivialDB SQL 测试执行程序
用于执行 SQL 测试文件并输出结果
"""

import subprocess
import sys
import os
import time

# 配置
TRIVIAL_DB_PATH = os.path.join(os.path.dirname(__file__), "build", "trivial_db")
SQL_TEST_FILE = os.path.join(os.path.dirname(__file__), "test_all_features.sql")


def print_header(title):
    """打印测试标题"""
    print("\n" + "=" * 60)
    print(f"  {title}")
    print("=" * 60)


def print_section(title):
    """打印章节标题"""
    print("\n" + "-" * 60)
    print(f"  {title}")
    print("-" * 60)


def run_sql_test():
    """运行 SQL 测试"""
    
    # 检查文件是否存在
    if not os.path.exists(TRIVIAL_DB_PATH):
        print(f"错误: 找不到 TrivialDB 可执行文件: {TRIVIAL_DB_PATH}")
        print("请先编译项目: cd build && make -j8")
        return False
    
    if not os.path.exists(SQL_TEST_FILE):
        print(f"错误: 找不到测试文件: {SQL_TEST_FILE}")
        return False
    
    print_header("TrivialDB SQL 测试执行程序")
    
    # 读取 SQL 文件
    print(f"读取测试文件: {SQL_TEST_FILE}")
    try:
        with open(SQL_TEST_FILE, 'r', encoding='utf-8') as f:
            sql_content = f.read()
        print(f"测试文件大小: {len(sql_content)} 字节")
    except Exception as e:
        print(f"读取测试文件失败: {e}")
        return False
    
    # 执行测试
    print_section("开始执行测试")
    
    try:
        # 启动 TrivialDB 进程
        process = subprocess.Popen(
            [TRIVIAL_DB_PATH],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            bufsize=1,
            cwd=os.path.dirname(__file__)
        )
        
        # 发送 SQL 命令
        stdout_lines = []
        start_time = time.time()
        
        # 逐行发送 SQL 命令
        for line in sql_content.split('\n'):
            line = line.strip()
            
            # 跳过空行和纯注释行
            if not line or line.startswith('--'):
                continue
            
            # 发送命令
            process.stdin.write(line + '\n')
            process.stdin.flush()
            
            # 读取输出
            while True:
                line = process.stdout.readline()
                if not line:
                    break
                stdout_lines.append(line.strip())
                print(line.strip())
                
                # 检查是否回到提示符
                if 'trivial_db>' in line or 'exit' in line.lower():
                    break
            
            # 检查是否已退出
            if 'good bye' in line.lower():
                break
        
        end_time = time.time()
        
        # 等待进程结束
        try:
            process.wait(timeout=5)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait()
        
        # 统计结果
        print_section("测试执行结果")
        
        # 分析输出
        error_count = stdout_lines.count('[Error]')
        passed_count = sum(1 for line in stdout_lines if 'PASSED' in line)
        
        print(f"总执行时间: {end_time - start_time:.2f} 秒")
        print(f"通过测试: {passed_count}")
        print(f"错误数量: {error_count}")
        
        # 显示统计信息
        print_section("输出统计")
        print(f"总输出行数: {len(stdout_lines)}")
        
        # 显示最后几行输出
        print_section("最后输出")
        for line in stdout_lines[-20:]:
            print(line)
        
        return error_count == 0
        
    except Exception as e:
        print(f"执行测试时出错: {e}")
        import traceback
        traceback.print_exc()
        return False


def run_single_command(command):
    """运行单个 SQL 命令"""
    
    if not os.path.exists(TRIVIAL_DB_PATH):
        print(f"错误: 找不到 TrivialDB: {TRIVIAL_DB_PATH}")
        return False
    
    try:
        process = subprocess.Popen(
            [TRIVIAL_DB_PATH],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            bufsize=1
        )
        
        # 发送命令
        process.stdin.write(command + '\n')
        process.stdin.write('EXIT;\n')
        process.stdin.flush()
        
        # 读取输出
        output = process.stdout.read()
        print(output)
        
        process.wait()
        return True
        
    except Exception as e:
        print(f"执行命令时出错: {e}")
        return False


def interactive_mode():
    """交互模式"""
    
    if not os.path.exists(TRIVIAL_DB_PATH):
        print(f"错误: 找不到 TrivialDB: {TRIVIAL_DB_PATH}")
        return
    
    print("\n" + "=" * 60)
    print("  TrivialDB 交互模式")
    print("=" * 60)
    print("输入 SQL 命令，按回车执行。")
    print("输入 'EXIT' 退出。")
    print("-" * 60)
    
    try:
        process = subprocess.Popen(
            [TRIVIAL_DB_PATH],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            bufsize=1
        )
        
        while True:
            cmd = input("SQL> ").strip()
            if not cmd:
                continue
            
            if cmd.upper() == 'EXIT':
                break
            
            # 发送命令
            process.stdin.write(cmd + '\n')
            process.stdin.flush()
            
            # 读取输出
            while True:
                line = process.stdout.readline()
                if not line:
                    break
                print(line.strip())
                
                if 'trivial_db>' in line or 'exit' in line.lower():
                    break
            
            if 'good bye' in line.lower():
                break
        
        process.wait()
        
    except KeyboardInterrupt:
        print("\n正在退出...")
        process.kill()
        process.wait()
    except Exception as e:
        print(f"错误: {e}")


def main():
    """主函数"""
    
    if len(sys.argv) > 1:
        # 命令行参数处理
        if sys.argv[1] == '--interactive' or sys.argv[1] == '-i':
            # 交互模式
            interactive_mode()
        elif sys.argv[1] == '--help' or sys.argv[1] == '-h':
            # 帮助信息
            print("""
TrivialDB SQL 测试执行程序

用法:
    python3 run_sql_test.py           # 执行完整测试
    python3 run_sql_test.py -i        # 交互模式
    python3 run_sql_test.py -h        # 显示帮助
    python3 run_sql_test.py "SQL命令" # 执行单个命令

选项:
    --interactive, -i    启动交互模式
    --help, -h           显示此帮助信息
            """)
        else:
            # 执行单个命令
            command = ' '.join(sys.argv[1:])
            run_single_command(command)
    else:
        # 默认执行完整测试
        run_sql_test()


if __name__ == '__main__':
    main()