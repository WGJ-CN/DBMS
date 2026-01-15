#!/usr/bin/env python3
import subprocess
import sys
import os

def run_test():
    # 配置路径
    db_path = os.path.join('build', 'trivial_db')
    sql_file = 'full_functionality_test.sql'
    
    # 检查文件是否存在
    if not os.path.exists(db_path):
        print(f"错误：找不到数据库程序 {db_path}")
        return False
    
    if not os.path.exists(sql_file):
        print(f"错误：找不到测试SQL文件 {sql_file}")
        return False
    
    print("开始执行TrivialDB功能测试...")
    
    try:
        # 使用subprocess运行测试
        result = subprocess.run(
            [db_path],
            input=open(sql_file, 'r').read(),
            text=True,
            capture_output=True,
            timeout=30  # 设置30秒超时
        )
        
        # 输出结果
        print("\n测试输出：")
        print(result.stdout)
        
        if result.returncode != 0:
            print("\n测试失败！")
            print("错误输出：")
            print(result.stderr)
            return False
        
        # 检查关键输出
        if "RENAME COLUMN" in result.stdout and "full_name" in result.stdout:
            print("\nRENAME COLUMN 功能测试通过！")
            return True
        else:
            print("\nRENAME COLUMN 功能测试失败")
            return False
            
    except subprocess.TimeoutExpired:
        print("\n测试超时！程序可能已卡住")
        return False
    except Exception as e:
        print(f"\n测试过程中发生错误：{str(e)}")
        return False

if __name__ == '__main__':
    if run_test():
        print("\n所有测试通过！")
        sys.exit(0)
    else:
        print("\n测试失败")
        sys.exit(1)