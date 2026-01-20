#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
TrivialDB GUI启动器
从根目录运行GUI程序，自动切换到GUI文件所在目录
"""

import os
import sys
import subprocess

def main():
    # 获取GUI文件所在目录
    gui_dir = os.path.join(os.path.dirname(__file__), 'src', 'gui')
    gui_file = os.path.join(gui_dir, 'trivialdb_gui.py')
    
    # 检查GUI文件是否存在
    if not os.path.exists(gui_file):
        print(f"错误：找不到GUI文件 {gui_file}")
        input("按Enter键退出...")
        return
    
    # 切换到GUI文件所在目录
    original_dir = os.getcwd()
    try:
        os.chdir(gui_dir)
        print(f"切换到目录: {gui_dir}")
        print("正在启动TrivialDB GUI...")
        
        # 运行GUI程序
        result = subprocess.run([sys.executable, "trivialdb_gui.py"])
        
        if result.returncode != 0:
            print(f"GUI程序异常退出，返回码: {result.returncode}")
            
    except Exception as e:
        print(f"运行错误: {e}")
        import traceback
        traceback.print_exc()
    finally:
        # 恢复原始目录
        os.chdir(original_dir)

if __name__ == "__main__":
    main()