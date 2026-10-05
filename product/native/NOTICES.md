# 第三方组件

本项目原创代码采用 GNU GPLv3 only（`GPL-3.0-only`），完整条款见仓库根目录 [LICENSE](../../LICENSE)。第三方组件仍采用其自身许可证；下列说明不改变第三方的版权与许可，也不代表所有历史交付包已完成公开分发审查。公开二进制包应附项目许可证、适用的第三方声明，并按 GPLv3 提供对应版本的相应源码。

## 随包源码

MD4C，Martin Mitáš，MIT。完整许可见 vendor/md4c/LICENSE.md，原源码与版权声明一并保留。

## 设备提供的运行库

SDL2、SDL_ttf 和 C 运行库使用掌机系统提供的版本。公开源码包不复制设备系统库；Noto Sans SC 字体随源码附带并保留 OFL 许可。掌机程序动态链接这些库；本项目源码及构建步骤随包保留。SDL 的许可为 zlib，SDL_ttf 为 zlib，C 运行库及其余系统依赖以设备原有许可证为准。

## 电脑端依赖

package-lock.json 锁定实际 npm 依赖。安装分发由各 npm 包提供；本交付包不包含 node_modules。THIRD_PARTY_PACKAGES.json 记录锁文件的版本和 license 字段（含可选平台包），完整许可在安装后的对应包中。

主要组件：MDast / micromark（MIT）、MathJax 3.2.2（Apache-2.0）、Sharp（Apache-2.0）及其图像运行库（包含 LGPL-3.0-or-later 等许可）。Sharp 的预编译运行库还包含各自的许可，请保留原安装包 notices。

本机已安装的 Codex app-tools 服务由适配器原样调用；没有复制或改写该服务到交付包。

## 本机语音依赖

本机 FunASR、PyTorch、CUDA 及中文 ASR/VAD 模型依赖各自发行包和模型许可。没有把环境、模型权重或用户音频放入交付包。

UI 0.5.2 uses Feather 4.29.2 (MIT, vendor/feather/LICENSE) and Noto Sans SC (SIL OFL, fonts/OFL.txt). Official distributions: https://github.com/feathericons/feather and https://fonts.google.com/noto/specimen/Noto+Sans+SC . Fonts are bundled unmodified; icons are rasterized from the original SVGs.
