<div align="center">
<img width="198" height="193" alt="Emerging-Logo" src="https://github.com/user-attachments/assets/00a3dae5-8d76-422b-8c6c-bbbcc4f5f9d6" />

  <h1>Emerging 编程语言</h1>
  <h3>使用 Microsoft Visual C++ 与 NASM 汇编开发的新兴编程语言</h3>

  <p align="center">
    <a href="https://deeplearning-corporation.github.io">
      <img src="https://img.shields.io/badge/Deep%20Learning%20Corporation-官方网站-2ea44f?style=flat-square" alt="Deep Learning Corporation 官网">
    </a>
  </p>
</div>

## .emg 编译 .exe
可使用 emerging.exe 一步到位，编译为 .exe Windows PE 格式可执行文件
可使用 linker.cpp 使用 .obj 编译为 .exe Windows PE 格式可执行文件

## 自带函数（仅展示部分函数）
1.file （操作文件函数）
2.time （时间函数）

## 贡献
- BiliBili UP:杜雨本雨
- BiliBili UP:OmegeUOS
- BiliBili UP:一片里脊肉
- BiliBili UP:长庚hc

## 我们的网站
Deep Learning Corporation 官网：https://deeplearning-corporation.github.io
Deep Learning Corporation 中的 Emerging 内容：https://deeplearning-corporation.github.io\emerging\frontend\index.html

## 安装
下载适用于 Windows x64 的安装程序：可在此 GitHub 仓库点击 Releases → 下载 emerging-win32-installer.exe
下载 Emerging 安装后的内容（.zip 压缩包）：可以在此 GitHub 仓库点击 Releases → 下载 emerging-win32.zip

## 编译
需要在 Windows 编译
1.下载 Microsoft Visual C++ 编译器
2.下载 NASM 以编译 运行时库

编译 emerging.cpp 与 linker.cpp：
```batch
cl /EHsc /Fe:emerging.exe emerging.cpp
cl /EHsc /Fe:linker.exe linker.cpp
```

编译运行时库：
```batch
nasm -f win32 -o runtime.obj runtime.asm
cd lib\runtime
nasm -f win32 -o runtime_win.obj runtime_win.asm
nasm -f elf32 -o runtime_linux.obj runtime_linux.asm
nasm -f macho32 -o runtime_macos.obj runtime_macos.asm
```

## 致谢
感谢阅读我们 Emerging 编程语言的 README！

## 加入我们
您如果想加入我们，可以搜索 QQ 群号：1026732566（群名：Omege UOS）

