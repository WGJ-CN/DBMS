#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
TrivialDB 图形用户界面
基于Python Tkinter的数据库管理GUI
"""

import tkinter as tk
from tkinter import ttk, messagebox, scrolledtext, simpledialog
import subprocess
import os
import json
import platform
from pathlib import Path
import ctypes

class TrivialDBGUI:
    def __init__(self, root):
        self.root = root
        self.root.title("📦 TrivialDB 数据库管理系统")
        self.root.geometry("1200x900")
        self.root.configure(bg="#ecf0f1")
        
        # 设置窗口图标（如果有）
        try:
            self.root.iconbitmap("icon.ico")  # Windows
        except:
            pass
        
                # 数据库配置
        self.current_db = None
        self.username = None
        self.password = None
        
        # 自动识别平台并设置可执行文件路径
        self.trivial_db_path = self._detect_db_path()
        
        # 初始化样式
        self.setup_styles()
                # 初始化界面
        self.setup_ui()
        # 启动时显示登录框
        self.root.after(100, self.show_login_dialog)
    
    def _detect_db_path(self):
        """自动检测平台并返回正确的数据库可执行文件路径"""
        # 判断当前操作系统
        system = platform.system()
        
        # 获取 GUI 脚本所在目录，用于计算相对路径
        gui_dir = os.path.dirname(os.path.abspath(__file__))
        
        if system == "Windows":
            # Windows 平台
            default_rel_path = "../../build-win/bin/trivial_db.exe"
        else:
            # Linux/WSL 平台
            default_rel_path = "../../build/bin/trivial_db"
        
        # 默认绝对路径
        default_path = os.path.normpath(os.path.join(gui_dir, default_rel_path))
        
        # 尝试读取配置文件覆盖默认值
        config_path = os.path.join(gui_dir, "config.json")
        if os.path.exists(config_path):
            try:
                with open(config_path, 'r') as f:
                    config = json.load(f)
                # 根据平台选择配置
                if system == "Windows":
                    rel_path = config.get("db_path_win", config.get("db_path", default_rel_path))
                else:
                    rel_path = config.get("db_path_linux", config.get("db_path", default_rel_path))
                # 转换为绝对路径
                return os.path.normpath(os.path.join(gui_dir, rel_path))
            except:
                pass
        
        return default_path
    
    def setup_styles(self):
        """设置界面样式"""
        style = ttk.Style()
        
        # 配置不同样式
        style.configure("TFrame", background="#f0f0f0")
        style.configure("TLabelFrame", background="#ffffff", bordercolor="#cccccc")
        style.configure("TLabelFrame.Label", background="#ffffff", foreground="#333333")
        
        # 按钮样式
        style.configure("Primary.TButton", 
                       background="#007acc", 
                       foreground="white",
                       padding=(10, 5),
                       font=("Arial", 10, "bold"))
        
        style.configure("Secondary.TButton",
                       background="#6c757d",
                       foreground="white",
                       padding=(8, 4))
        
        style.configure("Danger.TButton",
                       background="#dc3545",
                       foreground="white",
                       padding=(8, 4))
        
        style.configure("Success.TButton",
                       background="#28a745",
                       foreground="white",
                       padding=(8, 4))
        
        # 标签样式
        style.configure("Title.TLabel",
                       font=("Arial", 12, "bold"),
                       foreground="#2c3e50",
                       background="#f0f0f0")
        
        style.configure("Subtitle.TLabel",
                       font=("Arial", 10, "bold"),
                       foreground="#34495e",
                       background="#f0f0f0")
    
    def show_login_dialog(self):
        """显示登录对话框"""
        dialog = tk.Toplevel(self.root)
        dialog.title("用户登录")
        dialog.geometry("300x200")
        self.center_dialog(dialog, 300, 200)
        
        ttk.Label(dialog, text="用户名:").pack(pady=5)
        user_entry = ttk.Entry(dialog, width=20)
        user_entry.pack(pady=5)
        if self.username: user_entry.insert(0, self.username)
        
        ttk.Label(dialog, text="密码:").pack(pady=5)
        pass_entry = ttk.Entry(dialog, width=20, show="*")
        pass_entry.pack(pady=5)
        
        def on_login():
            user = user_entry.get().strip()
            pwd = pass_entry.get().strip()
            if not user or not pwd:
                messagebox.showerror("错误", "请输入用户名和密码")
                return
            
            # 保存凭证
            self.username = user
            self.password = pwd
            self.status_var.set(f"当前用户: {user}")
            dialog.destroy()
            
        btn_frame = ttk.Frame(dialog)
        btn_frame.pack(pady=20)
        ttk.Button(btn_frame, text="登录", command=on_login).pack(side=tk.LEFT, padx=10)
        ttk.Button(btn_frame, text="取消", command=dialog.destroy).pack(side=tk.LEFT, padx=10)

    def center_dialog(self, dialog, width=400, height=300):
        """将对话框居中显示在主窗口中心"""
        dialog.update_idletasks()
        # 获取主窗口位置和大小
        x = self.root.winfo_x() + (self.root.winfo_width() - width) // 2
        y = self.root.winfo_y() + (self.root.winfo_height() - height) // 2
        dialog.geometry(f"{width}x{height}+{x}+{y}")
        
    def setup_ui(self):
        """设置主界面布局"""
        # 创建标题栏
        self.create_header()
        
        # 创建主框架
        main_frame = ttk.Frame(self.root, padding="10", style="Main.TFrame")
        main_frame.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        
        # 配置网格权重
        self.root.columnconfigure(0, weight=1)
        self.root.rowconfigure(0, weight=1)
        main_frame.columnconfigure(1, weight=1)
        main_frame.rowconfigure(1, weight=1)
        
        # 左侧导航栏 - 使用淡蓝色背景
        nav_frame = ttk.LabelFrame(main_frame, text="数据库操作", padding="10", style="Nav.TLabelframe")
        nav_frame.grid(row=0, column=0, rowspan=2, sticky=(tk.N, tk.S, tk.W), padx=(0, 10))
        
        # 统一按钮颜色 - 淡蓝色
        button_color = "#e3f2fd"
        hover_color = "#bbdefb"
        active_color = "#90caf9"
        text_color = "#1565c0"
        
        # 数据库选择/创建
        self.create_nav_button(nav_frame, "创建数据库", self.create_database, 0, button_color, hover_color, active_color, text_color)
        self.create_nav_button(nav_frame, "选择数据库", self.select_database, 1, button_color, hover_color, active_color, text_color)
        self.create_nav_button(nav_frame, "删除数据库", self.drop_database, 2, button_color, hover_color, active_color, text_color)
        
        # 表操作分隔线
        self.create_section_separator(nav_frame, "表操作", 3, 4)
        
        self.create_nav_button(nav_frame, "创建表", self.create_table, 5, button_color, hover_color, active_color, text_color)
        self.create_nav_button(nav_frame, "显示表结构", self.show_table_structure, 6, button_color, hover_color, active_color, text_color)
        self.create_nav_button(nav_frame, "重命名表", self.rename_table, 7, button_color, hover_color, active_color, text_color)
        self.create_nav_button(nav_frame, "修改表结构", self.alter_table, 8, button_color, hover_color, active_color, text_color)
        self.create_nav_button(nav_frame, "删除表", self.drop_table, 9, button_color, hover_color, active_color, text_color)
        
        # 数据操作分隔线
        self.create_section_separator(nav_frame, "数据操作", 10, 11)
        
        self.create_nav_button(nav_frame, "插入数据", self.insert_data, 12, button_color, hover_color, active_color, text_color)
        self.create_nav_button(nav_frame, "查询数据", self.query_data, 13, button_color, hover_color, active_color, text_color)
        self.create_nav_button(nav_frame, "更新数据", self.update_data, 14, button_color, hover_color, active_color, text_color)
        self.create_nav_button(nav_frame, "删除数据", self.delete_data, 15, button_color, hover_color, active_color, text_color)
        
        # 用户管理
        self.create_nav_button(nav_frame, "登录/切换用户", self.show_login_dialog, 16, button_color, hover_color, active_color, text_color)

        # 备份恢复操作分隔线
        self.create_section_separator(nav_frame, "备份恢复", 17, 18)
        
        self.create_nav_button(nav_frame, "备份数据库", self.backup_database, 19, button_color, hover_color, active_color, text_color)
        self.create_nav_button(nav_frame, "恢复数据库", self.restore_database, 20, button_color, hover_color, active_color, text_color)
        
        # SQL命令行和退出
        self.create_nav_button(nav_frame, "SQL命令行", self.open_sql_console, 21, button_color, hover_color, active_color, text_color)
        self.create_nav_button(nav_frame, "退出程序", self.quit_app, 22, button_color, hover_color, active_color, text_color)
        
        # 状态栏
        self.status_var = tk.StringVar(value="就绪")
        status_bar = ttk.Label(main_frame, textvariable=self.status_var, relief=tk.SUNKEN, 
                            font=("Arial", 10), foreground="#1976d2", background="#e3f2fd")
        status_bar.grid(row=2, column=0, columnspan=2, sticky=(tk.W, tk.E), pady=(10, 0))
        
        # 主显示区域 - 白色背景
        display_frame = ttk.Frame(main_frame, style="Display.TFrame")
        display_frame.grid(row=0, column=1, rowspan=2, sticky=(tk.W, tk.E, tk.N, tk.S))
        display_frame.columnconfigure(0, weight=1)
        display_frame.rowconfigure(0, weight=1)
        
        # 结果显示区域 - 白色背景，黑色文字，增大字体
        self.result_text = scrolledtext.ScrolledText(display_frame, width=80, height=30,
                                                  font=("Consolas", 12),
                                                  bg="white", fg="black")
        self.result_text.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        
        # 数据库信息显示
        self.db_info_var = tk.StringVar(value="未选择数据库")
        db_info_label = ttk.Label(main_frame, textvariable=self.db_info_var, 
                                 font=("Arial", 11, "bold"), foreground="#1976d2",
                                 background="#f5f5f5")
        db_info_label.grid(row=2, column=1, sticky=tk.SE, padx=10, pady=10)
    
    def create_header(self):
        """创建标题栏"""
        # 标题栏框架
        header_frame = ttk.Frame(self.root, padding=(0, 5, 0, 5), style="Header.TFrame")
        header_frame.grid(row=0, column=0, sticky=(tk.W, tk.E))
        
        # 标题标签
        title_label = tk.Label(header_frame, text="📦 TrivialDB 数据库管理系统", 
                             font=("Arial", 18, "bold"), fg="#2c3e50", bg="#ecf0f1")
        title_label.pack(side=tk.LEFT, padx=15)
        
        # 副标题
        subtitle_label = tk.Label(header_frame, text="轻量级数据库管理专家", 
                               font=("Arial", 10), fg="#7f8c8d", bg="#ecf0f1")
        subtitle_label.pack(side=tk.LEFT, padx=10)
        
        # 分隔线
        separator = ttk.Separator(self.root, orient='horizontal')
        separator.grid(row=1, column=0, sticky=(tk.W, tk.E), pady=(0, 5))
    
    def create_nav_button(self, parent, text, command, row, button_color, hover_color, active_color, text_color):
        """创建带颜色的导航按钮"""
        btn = tk.Button(parent, text=text, command=command,
                      font=("Arial", 10, "bold"), bg=button_color, fg=text_color,
                      activebackground=active_color, activeforeground=text_color,
                      relief="flat", padx=10, pady=5, cursor="hand2")
        btn.grid(row=row, column=0, pady=3, sticky=tk.EW)
        
        # 添加悬停效果
        def on_enter(e):
            btn['background'] = hover_color
            
        def on_leave(e):
            btn['background'] = button_color
            
        btn.bind("<Enter>", on_enter)
        btn.bind("<Leave>", on_leave)
        
        return btn
    
    def create_section_separator(self, parent, label_text, sep_row, label_row):
        """创建分区分隔线"""
        separator = ttk.Separator(parent, orient='horizontal')
        separator.grid(row=sep_row, column=0, pady=8, sticky=tk.EW)
        label = tk.Label(parent, text=label_text, font=("Arial", 10, "bold"), 
                       fg="black", bg="#f0f0f0")
        label.grid(row=label_row, column=0, pady=3)
    
    def execute_sql(self, sql_command, require_db=True):
        """执行SQL命令并返回结果"""
        if require_db and not self.current_db:
            messagebox.showerror("错误", "请先选择或创建数据库")
            return None
        
        try:
            # 构建完整的SQL命令
            if require_db:
                full_command = f"USE {self.current_db};\n{sql_command}\nEXIT;"
            else:
                full_command = f"{sql_command}\nEXIT;"
            
                        # 执行命令 - 设置工作目录为可执行文件所在目录
            exe_dir = os.path.dirname(os.path.abspath(self.trivial_db_path))
            
            args = [self.trivial_db_path]
            if self.username and self.password:
                args.extend(["-u", self.username, "-p", self.password])

            process = subprocess.Popen(
                args,
                stdin=subprocess.PIPE,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                cwd=exe_dir
            )
            
            # 使用二进制模式读取，然后手动解码避免编码问题
            stdout, stderr = process.communicate(input=full_command.encode('utf-8'))
            
            # 尝试多种编码方式解码输出
            def safe_decode(data):
                if not data:
                    return ""
                try:
                    # 首先尝试UTF-8
                    return data.decode('utf-8')
                except UnicodeDecodeError:
                    try:
                        # 如果UTF-8失败，尝试GBK
                        return data.decode('gbk')
                    except UnicodeDecodeError:
                        # 如果都失败，使用错误替换
                        return data.decode('utf-8', errors='replace')
            
            stdout = safe_decode(stdout)
            stderr = safe_decode(stderr)
            
            if stderr:
                messagebox.showerror("执行错误", stderr)
                return None
                
            return stdout
            
        except Exception as e:
            messagebox.showerror("错误", f"执行命令时出错: {str(e)}")
            return None
    
    def create_database(self):
        """创建数据库对话框"""
        def on_create():
            db_name = name_entry.get().strip()
            if not db_name:
                messagebox.showerror("错误", "数据库名称不能为空")
                return
                
            sql = f"CREATE DATABASE {db_name};"
            result = self.execute_sql(sql, require_db=False)
            
            if result and "Error" not in result:
                self.current_db = db_name
                self.db_info_var.set(f"当前数据库: {db_name}")
                self.status_var.set(f"数据库 {db_name} 创建成功")
                dialog.destroy()
        
        dialog = tk.Toplevel(self.root)
        dialog.title("创建数据库")
        dialog.geometry("300x150")
        self.center_dialog(dialog, 300, 150)
        
        ttk.Label(dialog, text="数据库名称:").pack(pady=10)
        name_entry = ttk.Entry(dialog, width=20)
        name_entry.pack(pady=5)
        
        btn_frame = ttk.Frame(dialog)
        btn_frame.pack(pady=10)
        
        ttk.Button(btn_frame, text="创建", command=on_create).pack(side=tk.LEFT, padx=5)
        ttk.Button(btn_frame, text="取消", command=dialog.destroy).pack(side=tk.LEFT, padx=5)
    
    def select_database(self):
        """选择数据库对话框"""
        def on_select():
            db_name = name_entry.get().strip()
            if not db_name:
                messagebox.showerror("错误", "数据库名称不能为空")
                return
                
            # 检查数据库是否存在
            sql = f"SHOW DATABASE {db_name};"
            result = self.execute_sql(sql, require_db=False)
            
            if result and "Error" not in result:
                self.current_db = db_name
                self.db_info_var.set(f"当前数据库: {db_name}")
                self.status_var.set(f"已选择数据库: {db_name}")
                dialog.destroy()
            else:
                messagebox.showerror("错误", f"数据库 {db_name} 不存在")
        
        dialog = tk.Toplevel(self.root)
        dialog.title("选择数据库")
        dialog.geometry("300x150")
        self.center_dialog(dialog, 300, 150)
        
        ttk.Label(dialog, text="数据库名称:").pack(pady=10)
        name_entry = ttk.Entry(dialog, width=20)
        name_entry.pack(pady=5)
        
        btn_frame = ttk.Frame(dialog)
        btn_frame.pack(pady=10)
        
        ttk.Button(btn_frame, text="选择", command=on_select).pack(side=tk.LEFT, padx=5)
        ttk.Button(btn_frame, text="取消", command=dialog.destroy).pack(side=tk.LEFT, padx=5)
    
    def create_table(self):
        """创建表对话框"""
        def on_create():
            table_name = name_entry.get().strip()
            columns = columns_text.get("1.0", tk.END).strip()
            
            if not table_name or not columns:
                messagebox.showerror("错误", "表名和列定义不能为空")
                return
                
            sql = f"CREATE TABLE {table_name} ({columns});"
            result = self.execute_sql(sql)
            
            if result and "Error" not in result:
                self.status_var.set(f"表 {table_name} 创建成功")
                dialog.destroy()
        
        dialog = tk.Toplevel(self.root)
        dialog.title("创建表")
        dialog.geometry("400x300")
        self.center_dialog(dialog, 400, 300)
        
        ttk.Label(dialog, text="表名:").pack(pady=5)
        name_entry = ttk.Entry(dialog, width=30)
        name_entry.pack(pady=5)
        
        ttk.Label(dialog, text="列定义 (用逗号分隔):").pack(pady=5)
        columns_text = scrolledtext.ScrolledText(dialog, width=40, height=8)
        columns_text.pack(pady=5)
        columns_text.insert("1.0", "id INT PRIMARY KEY, name VARCHAR(50), age INT")
        
        btn_frame = ttk.Frame(dialog)
        btn_frame.pack(pady=10)
        
        ttk.Button(btn_frame, text="创建", command=on_create).pack(side=tk.LEFT, padx=5)
        ttk.Button(btn_frame, text="取消", command=dialog.destroy).pack(side=tk.LEFT, padx=5)
    
    def show_table_structure(self):
        """显示表结构对话框"""
        # 创建自定义对话框
        dialog = tk.Toplevel(self.root)
        dialog.title("显示表结构")
        dialog.geometry("400x200")
        self.center_dialog(dialog, 400, 200)
        
        ttk.Label(dialog, text="请输入表名:").pack(pady=20)
        table_entry = ttk.Entry(dialog, width=30)
        table_entry.pack(pady=10)
        
        def on_show():
            table_name = table_entry.get().strip()
            if table_name:
                sql = f"SHOW TABLE {table_name};"
                result = self.execute_sql(sql)
                if result:
                    self.display_result(result)
                dialog.destroy()
            else:
                messagebox.showwarning("警告", "请输入表名")
        
        btn_frame = ttk.Frame(dialog)
        btn_frame.pack(pady=20)
        ttk.Button(btn_frame, text="显示", command=on_show).pack(side=tk.LEFT, padx=10)
        ttk.Button(btn_frame, text="取消", command=dialog.destroy).pack(side=tk.LEFT, padx=10)
    
    def rename_table(self):
        """重命名表"""
        def on_rename():
            old_name = old_entry.get().strip()
            new_name = new_entry.get().strip()
            
            if not old_name or not new_name:
                messagebox.showerror("错误", "表名不能为空")
                return
                
            sql = f"RENAME TABLE {old_name} TO {new_name};"
            result = self.execute_sql(sql)
            
            if result and "Error" not in result:
                self.status_var.set(f"表 {old_name} 重命名为 {new_name}")
                dialog.destroy()
        
        dialog = tk.Toplevel(self.root)
        dialog.title("重命名表")
        dialog.geometry("300x200")
        self.center_dialog(dialog, 300, 200)
        
        ttk.Label(dialog, text="原表名:").pack(pady=5)
        old_entry = ttk.Entry(dialog, width=20)
        old_entry.pack(pady=5)
        
        ttk.Label(dialog, text="新表名:").pack(pady=5)
        new_entry = ttk.Entry(dialog, width=20)
        new_entry.pack(pady=5)
        
        btn_frame = ttk.Frame(dialog)
        btn_frame.pack(pady=10)
        
        ttk.Button(btn_frame, text="重命名", command=on_rename).pack(side=tk.LEFT, padx=5)
        ttk.Button(btn_frame, text="取消", command=dialog.destroy).pack(side=tk.LEFT, padx=5)
    
    def display_result(self, result):
        """在结果显示区域显示结果"""
        self.result_text.delete("1.0", tk.END)
        self.result_text.insert("1.0", result)
    
    # 其他方法需要继续实现...
    def alter_table(self):
        """修改表结构对话框"""
        self.alter_dialog = tk.Toplevel(self.root)
        self.alter_dialog.title("修改表结构")
        self.alter_dialog.geometry("400x400")
        self.center_dialog(self.alter_dialog, 400, 400)
        
        # 表名输入
        ttk.Label(self.alter_dialog, text="表名:").pack(pady=5)
        table_entry = ttk.Entry(self.alter_dialog, width=30)
        table_entry.pack(pady=5)
        
        # 操作选择
        ttk.Label(self.alter_dialog, text="操作类型:").pack(pady=5)
        operation_var = tk.StringVar(value="ADD")
        op_frame = ttk.Frame(self.alter_dialog)
        op_frame.pack(pady=5)
        
        ttk.Radiobutton(op_frame, text="添加列", variable=operation_var, value="ADD").pack(side=tk.LEFT)
        ttk.Radiobutton(op_frame, text="删除列", variable=operation_var, value="DROP").pack(side=tk.LEFT)
        ttk.Radiobutton(op_frame, text="重命名列", variable=operation_var, value="RENAME").pack(side=tk.LEFT)
        ttk.Radiobutton(op_frame, text="修改类型", variable=operation_var, value="MODIFY").pack(side=tk.LEFT)
        
        # 列名输入
        ttk.Label(self.alter_dialog, text="列名:").pack(pady=5)
        column_entry = ttk.Entry(self.alter_dialog, width=30)
        column_entry.pack(pady=5)
        
        # 新列名输入（重命名时使用）
        ttk.Label(self.alter_dialog, text="新列名 (重命名时):").pack(pady=5)
        new_column_entry = ttk.Entry(self.alter_dialog, width=30)
        new_column_entry.pack(pady=5)
        
        # 类型输入（添加/修改时使用）
        ttk.Label(self.alter_dialog, text="数据类型 (添加/修改时):").pack(pady=5)
        type_entry = ttk.Entry(self.alter_dialog, width=30)
        type_entry.pack(pady=5)
        type_entry.insert(0, "INT")
        
        def on_alter():
            table_name = table_entry.get().strip()
            operation = operation_var.get()
            column_name = column_entry.get().strip()
            new_column_name = new_column_entry.get().strip()
            new_type = type_entry.get().strip()
            
            if not table_name:
                messagebox.showerror("错误", "表名不能为空")
                return
                
            sql = ""
            if operation == "ADD":
                if not column_name or not new_type:
                    messagebox.showerror("错误", "列名和类型不能为空")
                    return
                sql = f"ALTER TABLE {table_name} ADD COLUMN {column_name} {new_type};"
            elif operation == "DROP":
                if not column_name:
                    messagebox.showerror("错误", "列名不能为空")
                    return
                sql = f"ALTER TABLE {table_name} DROP COLUMN {column_name};"
            elif operation == "RENAME":
                if not column_name or not new_column_name:
                    messagebox.showerror("错误", "原列名和新列名不能为空")
                    return
                sql = f"ALTER TABLE {table_name} RENAME COLUMN {column_name} TO {new_column_name};"
            elif operation == "MODIFY":
                if not column_name or not new_type:
                    messagebox.showerror("错误", "列名和类型不能为空")
                    return
                sql = f"ALTER TABLE {table_name} MODIFY COLUMN {column_name} {new_type};"
            
            try:
                result = self.execute_sql(sql)
                if result is None:
                    return  # execute_sql已经显示了错误信息
                
                if "Error" in result:
                    messagebox.showerror("错误", result)
                    return
                
                self.status_var.set(f"表 {table_name} 修改成功")
                # 直接关闭对话框，不再使用after方法
                self.alter_dialog.destroy()
            except Exception as e:
                messagebox.showerror("系统错误", f"执行命令时出错: {str(e)}")
                # 直接关闭对话框，不再使用after方法
                self.alter_dialog.destroy()
        
        btn_frame = ttk.Frame(self.alter_dialog)
        btn_frame.pack(pady=10)
        
        ttk.Button(btn_frame, text="执行", command=on_alter).pack(side=tk.LEFT, padx=5)
        ttk.Button(btn_frame, text="取消", command=self.alter_dialog.destroy).pack(side=tk.LEFT, padx=5)
    
    def drop_database(self):
        """删除数据库对话框"""
        def on_drop():
            db_name = name_entry.get().strip()
            if not db_name:
                messagebox.showerror("错误", "数据库名称不能为空")
                return
                
            # 确认对话框
            if not messagebox.askyesno("确认", f"确定要删除数据库 {db_name} 吗？此操作不可恢复！"):
                return
                
            sql = f"DROP DATABASE {db_name};"
            result = self.execute_sql(sql, require_db=False)
            
            if result and "Error" not in result:
                if self.current_db == db_name:
                    self.current_db = None
                    self.db_info_var.set("未选择数据库")
                self.status_var.set(f"数据库 {db_name} 删除成功")
                dialog.destroy()
        
        dialog = tk.Toplevel(self.root)
        dialog.title("删除数据库")
        dialog.geometry("300x150")
        self.center_dialog(dialog, 300, 150)
        
        ttk.Label(dialog, text="数据库名称:").pack(pady=10)
        name_entry = ttk.Entry(dialog, width=20)
        name_entry.pack(pady=5)
        
        btn_frame = ttk.Frame(dialog)
        btn_frame.pack(pady=10)
        
        ttk.Button(btn_frame, text="删除", command=on_drop).pack(side=tk.LEFT, padx=5)
        ttk.Button(btn_frame, text="取消", command=dialog.destroy).pack(side=tk.LEFT, padx=5)
    
    def quit_app(self):
        """退出应用程序"""
        if messagebox.askyesno("退出", "确定要退出TrivialDB吗？"):
            self.root.destroy()
    
    def backup_database(self):
        """备份数据库功能"""
        if not self.current_db:
            messagebox.showerror("错误", "请先选择或创建数据库")
            return
        
        # 确认备份操作
        if messagebox.askyesno("确认备份", f"确定要备份数据库 {self.current_db} 吗？"):
            try:
                import os
                import shutil
                
                # 构建路径 - 直接使用相对于当前项目根目录的路径
                # 获取当前脚本所在目录
                script_dir = os.path.dirname(os.path.abspath(__file__))
                # 项目根目录是script_dir的上两级（src/gui -> src -> root）
                project_root = os.path.abspath(os.path.join(script_dir, "..", ".."))
                db_root = os.path.join(project_root, "database")
                # 将备份目录放在项目根目录下，而不是database目录下
                backup_root = os.path.join(project_root, "backup")
                
                # 确保备份目录存在
                os.makedirs(backup_root, exist_ok=True)
                
                # 关闭数据库（如果打开）- 必须先关闭数据库，确保.dabase文件包含最新的表信息
                if self.current_db:
                    # 执行EXIT命令，让数据库引擎自动调用close()方法保存所有信息
                    sql = "EXIT;"
                    self.execute_sql(sql, require_db=False)
                
                # 复制数据库元信息文件
                db_file = os.path.join(db_root, f"{self.current_db}.database")
                backup_db_file = os.path.join(backup_root, f"{self.current_db}.database")
                
                # 检查数据库文件是否存在
                if not os.path.exists(db_file):
                    messagebox.showerror("错误", f"未找到数据库文件: {db_file}")
                    return
                
                shutil.copy2(db_file, backup_db_file)
                
                # 获取表名列表 - 直接从数据库文件中读取表信息
                tables = []
                
                try:
                    import struct
                    # 定义数据库信息结构（与database.h中的定义一致）
                    # 结构包含：
                    # - int table_num; (4字节)
                    # - char db_name[MAX_NAME_LEN]; (64字节)
                    # - char table_name[MAX_TABLE_NUM][MAX_NAME_LEN]; (16 * 64字节)
                    
                    with open(db_file, 'rb') as f:
                        # 读取表数量（第一个字段是table_num）
                        table_num = struct.unpack('i', f.read(4))[0]
                        # 读取数据库名称
                        db_name_bytes = f.read(64)  # 跳过数据库名称
                        # 读取表名列表
                        for i in range(table_num):
                            table_name = f.read(64).decode('utf-8').strip('\x00')
                            if table_name:
                                tables.append(table_name)
                except Exception as e:
                    messagebox.showwarning("警告", f"无法获取表信息: {str(e)}")
                
                # 复制每个表的.thead和.tdata文件
                if tables:
                    for table_name in tables:
                        # 复制表结构文件
                        head_file = os.path.join(db_root, f"{table_name}.thead")
                        backup_head_file = os.path.join(backup_root, f"{table_name}.thead")
                        
                        if os.path.exists(head_file):
                            shutil.copy2(head_file, backup_head_file)
                        else:
                            messagebox.showwarning("警告", f"未找到表 {table_name} 的结构文件 {head_file}")
                        
                        # 复制表数据文件
                        data_file = os.path.join(db_root, f"{table_name}.tdata")
                        backup_data_file = os.path.join(backup_root, f"{table_name}.tdata")
                        
                        if os.path.exists(data_file):
                            shutil.copy2(data_file, backup_data_file)
                        else:
                            messagebox.showwarning("警告", f"未找到表 {table_name} 的数据文件 {data_file}")
                
                messagebox.showinfo("成功", f"数据库 {self.current_db} 备份成功")
                self.status_var.set(f"数据库 {self.current_db} 备份成功")
                
            except Exception as e:
                messagebox.showerror("错误", f"备份时出错: {str(e)}")
    
    def restore_database(self):
        """恢复数据库功能"""
        # 打开对话框让用户输入要恢复的数据库名称
        db_name = simpledialog.askstring("恢复数据库", "请输入要恢复的数据库名称:")
        
        if not db_name:
            return
        
        # 确认恢复操作
        if messagebox.askyesno("确认恢复", f"确定要从备份中恢复数据库 {db_name} 吗？"):
            try:
                import os
                import shutil
                
                # 构建路径 - 直接使用相对于当前项目根目录的路径
                # 获取当前脚本所在目录
                script_dir = os.path.dirname(os.path.abspath(__file__))
                # 项目根目录是script_dir的上两级（src/gui -> src -> root）
                project_root = os.path.abspath(os.path.join(script_dir, "..", ".."))
                db_root = os.path.join(project_root, "database")
                # 将备份目录放在项目根目录下，而不是database目录下
                backup_root = os.path.join(project_root, "backup")
                
                # 检查备份文件是否存在
                backup_db_file = os.path.join(backup_root, f"{db_name}.database")
                if not os.path.exists(backup_db_file):
                    messagebox.showerror("错误", f"未找到数据库 {db_name} 的备份文件")
                    return
                
                # 关闭当前数据库（如果打开）
                # 不需要手动调用EXIT，execute_sql方法会自动添加
                
                # 恢复数据库元信息文件
                db_file = os.path.join(db_root, f"{db_name}.database")
                shutil.copy2(backup_db_file, db_file)
                
                # 读取数据库表信息
                import struct
                db_info = {}
                with open(db_file, 'rb') as f:
                    db_info = {}
                    # 假设数据库信息结构中包含表数量和表名列表
                    # 这里需要根据实际的database.h中的结构来读取
                    # 暂时使用简单的方法，通过备份文件获取表信息
                    
                # 从备份的数据库文件中读取表名列表
                tables = []
                
                try:
                    import struct
                    # 从备份的数据库文件中读取表信息（与database.h结构一致）
                    with open(backup_db_file, 'rb') as f:
                        # 读取表数量（第一个字段是table_num）
                        table_num = struct.unpack('i', f.read(4))[0]
                        # 读取数据库名称
                        db_name_bytes = f.read(64)  # 跳过数据库名称
                        # 读取表名列表
                        for i in range(table_num):
                            table_name = f.read(64).decode('utf-8').strip('\x00')
                            if table_name:
                                tables.append(table_name)
                except Exception as e:
                    messagebox.showwarning("警告", f"无法从备份文件中读取表信息: {str(e)}")
                    # 如果无法从备份文件中读取表信息，尝试通过备份目录中的文件获取
                    messagebox.showinfo("信息", "将尝试通过备份目录中的文件恢复表")
                    
                # 如果从备份文件中没有读取到表信息，尝试通过备份目录中的文件获取
                if not tables:
                    # 获取备份目录中所有.thead文件的名称
                    table_files = [f[:-6] for f in os.listdir(backup_root) if f.endswith('.thead')]
                    tables = table_files
                
                # 恢复表文件
                if tables:
                    for table_name in tables:
                        # 恢复表结构文件
                        backup_head_file = os.path.join(backup_root, f"{table_name}.thead")
                        head_file = os.path.join(db_root, f"{table_name}.thead")
                        
                        if os.path.exists(backup_head_file):
                            shutil.copy2(backup_head_file, head_file)
                        else:
                            messagebox.showwarning("警告", f"未找到表 {table_name} 的备份结构文件 {backup_head_file}")
                        
                        # 恢复表数据文件
                        backup_data_file = os.path.join(backup_root, f"{table_name}.tdata")
                        data_file = os.path.join(db_root, f"{table_name}.tdata")
                        
                        if os.path.exists(backup_data_file):
                            shutil.copy2(backup_data_file, data_file)
                        else:
                            messagebox.showwarning("警告", f"未找到表 {table_name} 的备份数据文件 {backup_data_file}")
                
                # 更新当前数据库信息
                self.current_db = db_name
                self.db_info_var.set(f"当前数据库: {db_name}")
                messagebox.showinfo("成功", f"数据库 {db_name} 恢复成功")
                self.status_var.set(f"数据库 {db_name} 恢复成功")
                
            except Exception as e:
                messagebox.showerror("错误", f"恢复时出错: {str(e)}")
    
    def drop_table(self):
        """删除表对话框"""
        def on_drop():
            table_name = name_entry.get().strip()
            if not table_name:
                messagebox.showerror("错误", "表名不能为空")
                return
                
            # 确认对话框
            if not messagebox.askyesno("确认", f"确定要删除表 {table_name} 吗？此操作不可恢复！"):
                return
                
            sql = f"DROP TABLE {table_name};"
            result = self.execute_sql(sql)
            
            if result and "Error" not in result:
                self.status_var.set(f"表 {table_name} 删除成功")
                dialog.destroy()
        
        dialog = tk.Toplevel(self.root)
        dialog.title("删除表")
        dialog.geometry("300x150")
        self.center_dialog(dialog, 300, 150)
        
        ttk.Label(dialog, text="表名:").pack(pady=10)
        name_entry = ttk.Entry(dialog, width=20)
        name_entry.pack(pady=5)
        
        btn_frame = ttk.Frame(dialog)
        btn_frame.pack(pady=10)
        
        ttk.Button(btn_frame, text="删除", command=on_drop).pack(side=tk.LEFT, padx=5)
        ttk.Button(btn_frame, text="取消", command=dialog.destroy).pack(side=tk.LEFT, padx=5)
    
    def insert_data(self):
        """插入数据对话框"""
        def on_insert():
            table_name = table_entry.get().strip()
            values = values_text.get("1.0", tk.END).strip()
            
            if not table_name or not values:
                messagebox.showerror("错误", "表名和值不能为空")
                return
            
            # 解析多行值
            value_lines = [v.strip() for v in values.split('\n') if v.strip()]
            
            for val_line in value_lines:
                sql = f"INSERT INTO {table_name} VALUES ({val_line});"
                result = self.execute_sql(sql)
                if result and "Error" in result:
                    messagebox.showerror("插入错误", f"插入失败: {result}")
                    return
            
            self.status_var.set(f"成功插入 {len(value_lines)} 条数据到表 {table_name}")
            dialog.destroy()
        
        dialog = tk.Toplevel(self.root)
        dialog.title("插入数据")
        dialog.geometry("400x300")
        self.center_dialog(dialog, 400, 300)
        
        ttk.Label(dialog, text="表名:").pack(pady=5)
        table_entry = ttk.Entry(dialog, width=30)
        table_entry.pack(pady=5)
        
        ttk.Label(dialog, text="值 (每行一条记录):").pack(pady=5)
        values_text = scrolledtext.ScrolledText(dialog, width=40, height=8)
        values_text.pack(pady=5)
        values_text.insert("1.0", "1, 'Alice', 25\n2, 'Bob', 30")
        
        btn_frame = ttk.Frame(dialog)
        btn_frame.pack(pady=10)
        
        ttk.Button(btn_frame, text="插入", command=on_insert).pack(side=tk.LEFT, padx=5)
        ttk.Button(btn_frame, text="取消", command=dialog.destroy).pack(side=tk.LEFT, padx=5)
    
    def query_data(self):
        """查询数据对话框"""
        def on_query():
            tables = tables_entry.get().strip()
            columns = columns_entry.get().strip()
            condition = condition_entry.get().strip()
            group_by = group_by_entry.get().strip()
            having = having_entry.get().strip()
            order_by = order_by_entry.get().strip()
            order_direction = order_direction_var.get()
            
            if not tables:
                messagebox.showerror("错误", "表名不能为空")
                return
                
            sql = f"SELECT {columns if columns else '*'} FROM {tables}"
            if condition:
                sql += f" WHERE {condition}"
            if group_by:
                sql += f" GROUP BY {group_by}"
            if having:
                sql += f" HAVING {having}"
            if order_by:
                sql += f" ORDER BY {order_by} {order_direction}"
            sql += ";"
            
            result = self.execute_sql(sql)
            if result:
                self.display_result(result)
                dialog.destroy()
        
        dialog = tk.Toplevel(self.root)
        dialog.title("高级查询")
        dialog.geometry("500x500")
        self.center_dialog(dialog, 500, 500)
        
        ttk.Label(dialog, text="表名 (多表用逗号分隔):").pack(pady=5)
        tables_entry = ttk.Entry(dialog, width=40)
        tables_entry.pack(pady=5)
        
        ttk.Label(dialog, text="列名 (可选，用逗号分隔，支持COUNT(), AVG(), MAX(), MIN()等):").pack(pady=5)
        columns_entry = ttk.Entry(dialog, width=40)
        columns_entry.pack(pady=5)
        
        ttk.Label(dialog, text="条件 (WHERE子句):").pack(pady=5)
        condition_entry = ttk.Entry(dialog, width=40)
        condition_entry.pack(pady=5)
        
        ttk.Label(dialog, text="分组依据 (GROUP BY子句):").pack(pady=5)
        group_by_entry = ttk.Entry(dialog, width=40)
        group_by_entry.pack(pady=5)
        
        ttk.Label(dialog, text="分组条件 (HAVING子句):").pack(pady=5)
        having_entry = ttk.Entry(dialog, width=40)
        having_entry.pack(pady=5)
        
        ttk.Label(dialog, text="排序依据 (ORDER BY子句):").pack(pady=5)
        order_by_entry = ttk.Entry(dialog, width=40)
        order_by_entry.pack(pady=5)
        
        # 排序方向选择
        order_frame = ttk.Frame(dialog)
        order_frame.pack(pady=5)
        ttk.Label(order_frame, text="排序方向:").pack(side=tk.LEFT)
        order_direction_var = tk.StringVar(value="ASC")
        ttk.Radiobutton(order_frame, text="升序", variable=order_direction_var, value="ASC").pack(side=tk.LEFT, padx=5)
        ttk.Radiobutton(order_frame, text="降序", variable=order_direction_var, value="DESC").pack(side=tk.LEFT, padx=5)
        
        btn_frame = ttk.Frame(dialog)
        btn_frame.pack(pady=15)
        
        ttk.Button(btn_frame, text="查询", command=on_query).pack(side=tk.LEFT, padx=5)
        ttk.Button(btn_frame, text="取消", command=dialog.destroy).pack(side=tk.LEFT, padx=5)
    
    def update_data(self):
        """更新数据对话框"""
        def on_update():
            table_name = table_entry.get().strip()
            set_clause = set_entry.get().strip()
            condition = condition_entry.get().strip()
            
            if not table_name or not set_clause:
                messagebox.showerror("错误", "表名和SET子句不能为空")
                return
                
            sql = f"UPDATE {table_name} SET {set_clause}"
            if condition:
                sql += f" WHERE {condition}"
            sql += ";"
            
            result = self.execute_sql(sql)
            if result and "Error" not in result:
                self.status_var.set(f"表 {table_name} 数据更新成功")
                dialog.destroy()
        
        dialog = tk.Toplevel(self.root)
        dialog.title("更新数据")
        dialog.geometry("400x250")
        self.center_dialog(dialog, 400, 250)
        
        ttk.Label(dialog, text="表名:").pack(pady=5)
        table_entry = ttk.Entry(dialog, width=30)
        table_entry.pack(pady=5)
        
        ttk.Label(dialog, text="SET子句 (例: age = 30):").pack(pady=5)
        set_entry = ttk.Entry(dialog, width=30)
        set_entry.pack(pady=5)
        
        ttk.Label(dialog, text="条件 (可选):").pack(pady=5)
        condition_entry = ttk.Entry(dialog, width=30)
        condition_entry.pack(pady=5)
        
        btn_frame = ttk.Frame(dialog)
        btn_frame.pack(pady=10)
        
        ttk.Button(btn_frame, text="更新", command=on_update).pack(side=tk.LEFT, padx=5)
        ttk.Button(btn_frame, text="取消", command=dialog.destroy).pack(side=tk.LEFT, padx=5)
    
    def delete_data(self):
        """删除数据对话框"""
        def on_delete():
            table_name = table_entry.get().strip()
            condition = condition_entry.get().strip()
            
            if not table_name:
                messagebox.showerror("错误", "表名不能为空")
                return
                
            if not condition:
                if not messagebox.askyesno("警告", "没有指定条件，将删除表中所有数据！确定要继续吗？"):
                    return
            
            sql = f"DELETE FROM {table_name}"
            if condition:
                sql += f" WHERE {condition}"
            sql += ";"
            
            result = self.execute_sql(sql)
            if result and "Error" not in result:
                self.status_var.set(f"表 {table_name} 数据删除成功")
                dialog.destroy()
        
        dialog = tk.Toplevel(self.root)
        dialog.title("删除数据")
        dialog.geometry("400x200")
        self.center_dialog(dialog, 400, 200)
        
        ttk.Label(dialog, text="表名:").pack(pady=5)
        table_entry = ttk.Entry(dialog, width=30)
        table_entry.pack(pady=5)
        
        ttk.Label(dialog, text="条件 (可选，为空则删除所有数据):").pack(pady=5)
        condition_entry = ttk.Entry(dialog, width=30)
        condition_entry.pack(pady=5)
        
        btn_frame = ttk.Frame(dialog)
        btn_frame.pack(pady=10)
        
        ttk.Button(btn_frame, text="删除", command=on_delete).pack(side=tk.LEFT, padx=5)
        ttk.Button(btn_frame, text="取消", command=dialog.destroy).pack(side=tk.LEFT, padx=5)
    
    def open_sql_console(self):
        """打开SQL命令行界面"""
        console = SQLConsole(self.root, self)
        console.grab_set()

class SQLConsole(tk.Toplevel):
    """SQL命令行控制台"""
    def __init__(self, parent, gui):
        super().__init__(parent)
        self.gui = gui
        self.title("SQL控制台")
        self.geometry("600x400")
        
        # 居中显示
        parent_geo = parent.geometry()
        pw, ph = map(int, parent_geo.split('+')[0].split('x'))
        px, py = map(int, parent_geo.split('+')[1:])
        self.geometry(f"600x400+{px + (pw-600)//2}+{py + (ph-400)//2}")
        
        # SQL输入区域
        ttk.Label(self, text="输入SQL命令:").pack(pady=5)
        self.sql_text = scrolledtext.ScrolledText(self, height=10)
        self.sql_text.pack(pady=5, fill=tk.BOTH, expand=True)
        
        # 按钮区域
        btn_frame = ttk.Frame(self)
        btn_frame.pack(pady=10)
        
        ttk.Button(btn_frame, text="执行", command=self.execute).pack(side=tk.LEFT, padx=5)
        ttk.Button(btn_frame, text="清除", command=self.clear).pack(side=tk.LEFT, padx=5)
        ttk.Button(btn_frame, text="关闭", command=self.destroy).pack(side=tk.LEFT, padx=5)
    
    def execute(self):
        """执行SQL命令"""
        sql = self.sql_text.get("1.0", tk.END).strip()
        if sql:
            result = self.gui.execute_sql(sql)
            if result:
                # 在新的对话框中显示结果
                result_dialog = tk.Toplevel(self)
                result_dialog.title("执行结果")
                result_dialog.geometry("500x300")
                
                result_text = scrolledtext.ScrolledText(result_dialog)
                result_text.pack(fill=tk.BOTH, expand=True, padx=10, pady=10)
                result_text.insert("1.0", result)
                
                ttk.Button(result_dialog, text="关闭", command=result_dialog.destroy).pack(pady=10)
    
    def clear(self):
        """清除输入"""
        self.sql_text.delete("1.0", tk.END)

def main():
    """主函数"""
    # 解决 Windows 下界面模糊/分辨率低的问题
    if platform.system() == "Windows":
        try:
            # Windows 8.1+
            ctypes.windll.shcore.SetProcessDpiAwareness(1)
        except Exception:
            try:
                # Windows Vista/7/8
                ctypes.windll.user32.SetProcessDPIAware()
            except Exception:
                pass

    root = tk.Tk()
    app = TrivialDBGUI(root)
    root.mainloop()

if __name__ == "__main__":
    main()