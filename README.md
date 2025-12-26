# TCPingGUI for Windows

一个用C语言和Win32 API编写的、带GUI界面的Windows平台TCP端口批量测试工具。

<!-- 
    你可以在这里添加截图。
    将截图文件命名为 "screenshot.png" 并放在项目根目录，然后取消下面这行的注释：
    ![TCPingGUI Screenshot](screenshot.png)
-->

## 简介

本工具提供了一个直观的图形用户界面，用于测试到指定IP/域名的TCP端口连通性及测量延迟。它模仿了Windows原生`ping`工具的输出格式。

## ✨ 功能特性

- **图形用户界面**：所有操作均在GUI窗口中完成，易于使用。
- **高DPI兼容**：界面在高分屏（如2K, 4K）上也能清晰显示。
- **现代化外观**：UI控件采用Windows现代视觉样式。
- **批量Ping**：可自定义Ping的次数，输入`0`为无限次Ping。
- **实时日志**：模仿Windows `ping`命令的格式，实时显示每个连接请求的结果。
- **统计报告**：在任务结束或手动停止后，自动统计并显示延迟（最大/最小/平均），以及成功/失败次数和丢包率。

## 编译运行

本项目使用`gcc` (MinGW-w64)进行编译。

### 依赖

- [MinGW-w64](https://www.mingw-w64.org/) (确保`gcc.exe`和`windres.exe`在你的系统`PATH`环境变量中)。

### 编译步骤

项目内已包含一个批处理脚本 `build.bat`，它会自动完成所有编译步骤。

只需在项目根目录下，通过命令行运行：
```bash
.\build.bat
```
脚本会自动执行以下操作：
1.  使用`windres.exe`编译资源文件 (`.rc`)。
2.  使用`gcc.exe`编译C语言源代码 (`main.c`) 并将其与资源文件链接。
3.  在根目录下生成最终的可执行文件 `TCPing_GUI.exe`。

## 许可证

本项目采用 [MIT License](LICENSE)。