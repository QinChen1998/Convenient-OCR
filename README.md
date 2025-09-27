# Convenient-OCR 智能文字识别工具 

一个基于Qt和C++17构建的智能光学字符识别(OCR)应用程序。

## 功能特性

### 核心功能
- **多格式支持**: PNG、JPG、BMP、TIFF、WebP、PDF文档
- **多语言识别**: 中文(简体/繁体)、英语等多种语言
- **导出功能**: 复制到剪贴板、保存为文本文件
- **屏幕截图**: 内置屏幕捕获功能

## 系统要求

### 运行环境
- **操作系统**: Windows 10/11
- **Qt版本**: Qt 6.9.2
- **编译器**: 支持C++17的编译器 (LLVM-MinGW/Clang/GCC)

### 依赖组件
- **Tesseract OCR**: v5.5.0+ (文字识别引擎)
- **Poppler**: v25.07.0 (PDF处理)

## 安装指南

### 1. 安装Qt开发环境
```bash
# 下载并安装Qt 6.9.2
# Windows: https://www.qt.io/download
```

### 2. 安装Tesseract OCR
```bash
# Windows: 从 https://github.com/tesseract-ocr/tesseract 下载
```

### 3. Poppler (PDF支持)
```bash
# Windows: 从 https://github.com/oschwartz10612/poppler-windows 下载
```

## 编译构建

### 使用qmake构建
```bash
# 1. 生成Makefile
qmake Convenient-OCR.pro

# 2. 编译项目
# Windows (MinGW):
mingw32-make

# 3. 清理构建
mingw32-make clean  # Windows
```

### 构建配置
- **Debug版本**: 输出到 `build/debug/`
- **Release版本**: 输出到 `build/release/`
- **目标可执行文件**: `ConvenientOCRApplication.exe` (Windows) 

## 项目结构

```
Convenient-OCR/
├── main.cpp                   # 应用程序入口点
├── mainwindow.h/cpp           # 主窗口UI控制器
├── mainwindow.ui              # UI设计文件
├── ocrengine.h/cpp            # OCR引擎抽象基类
├── tesseractocrengine.h/cpp   # Tesseract OCR实现
├── fileprocessor.h/cpp        # 多格式文件处理器
├── screencapture.h/cpp        # 屏幕截图功能
├── licensedialog.h/cpp        # 许可证对话框
├── licensedialog.ui           # 许可证对话框UI
├── Convenient-OCR.pro         # qmake项目配置文件
└── build/                     # 构建输出目录
    ├── debug/                 # Debug构建
    └── release/               # Release构建
```

## 使用说明

### 基本操作流程
1. **启动应用程序**: 运行 `ConvenientOCRApplication.exe`
2. **选择输入方式**:
   - 打开图像文件 (PNG, JPG, BMP, TIFF, WebP)
   - 打开PDF文档
   - 使用屏幕截图功能
3. **配置识别选项**:
   - 选择OCR引擎 (当前支持Tesseract)
   - 选择识别语言
4. **执行识别**: 点击识别按钮开始处理
5. **查看和编辑结果**: 在结果区域查看和编辑识别的文本
6. **导出结果**: 复制到剪贴板或保存为文件

### 高级功能
- **多页PDF处理**: 自动处理多页PDF文档
- **批量处理**: 支持多个文件的批量识别
- **进度显示**: 实时显示处理进度

## 许可证

### 开源协议
- **Qt 6.9.2**: LGPL v3
- **Tesseract OCR 5.5.0**: Apache 2.0 
- **Poppler**: MIT
- **LLVM-MinGW**: Apache 2.0 + LLVM

## 贡献

欢迎提交问题报告和功能请求。在贡献代码时，请：

1. Fork本仓库
2. 创建功能分支
3. 提交更改
4. 推送到分支
5. 创建Pull Request

## 技术支持

如果遇到问题，请检查：
1. **依赖安装**: 确保Tesseract和Poppler正确安装
2. **路径配置**: 验证OCR引擎可执行文件在系统PATH中
3. **语言数据**: 确保安装了所需的Tesseract语言包
4. **权限设置**: 确保应用程序有足够的文件访问权限

## 版本历史

- **v1.0.0**: 初始版本
  - 基础OCR功能
  - Tesseract引擎集成
  - 多格式文件支持
  - PDF处理功能
  - 屏幕截图功能

---

