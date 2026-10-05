# 开发环境与首次配置

当前源码预览支持维护者/开发者复现电脑逻辑，尚未完成陌生电脑的完整安装验收。请勿把测试通过理解为掌机已完成配对和安装。

## 1. 电脑端依赖与测试

Windows、Node.js >=20.9（本次验证使用 Node 22）及 Python 3.13。Python 单元测试不需要下载语音模型。

在 `product/native` 运行 `npm ci --ignore-scripts`、`npm test`、`python -m unittest discover -p "test_*.py"`。锁文件固定 npm 依赖，安装后的目录留在本地。`--ignore-scripts` 避免运行依赖安装脚本；本次在 Windows 已验证预编译 Sharp 包可加载并处理测试图像。

## 2. 掌机编译

目标为 TRIMUI Smart Pro ARM64，开发环境需要 WSL/Linux 的 `gcc-aarch64-linux-gnu`、SDL2、SDL_ttf 开发头文件。先在 Windows 连接自己的 USB/ADB 设备，用 `adb devices` 确认目标，然后运行：

```powershell
./prepare-build-libs.ps1 -Serial <自己的设备序列号>
```

在 WSL 的本目录执行 `sh build.sh`。该脚本依赖目标设备库，运行时使用设备原有系统库。仓库不分发这些库，也不保证任意固件兼容。

设备安装、截屏和壁纸脚本同样要求 `-Serial` 或 `ANDROID_SERIAL`。公开版本已移除维护者的固定设备标识。更新前先退出掌机应用；备份自己的数据，不要使用别人的配对文件。

## 3. 本地语音

默认使用 WSL 当前用户的 `$HOME/funasr-gpu/bin/python`；这是约定路径，不是自动安装环境。若使用其他虚拟环境，在启动桥接进程前配置 `GM_ASR_PYTHON` 为 WSL 内的 Python 绝对路径。路径作为独立参数传入，不作为 shell 代码执行。

运行环境涉及 FunASR、PyTorch、CUDA、NumPy、语音模型与词表。GPU、驱动及完整 Python 版本组合尚未形成经过独立验证的锁定安装方案。`setup-nano-model.py` 和 `setup-stream-model.py` 会下载较大的模型，使用前核对磁盘空间、来源和模型许可。下载脚本不代表允许把模型重新分发。

## 4. 桌面连接与配对：仍需手动配置

桥接需要本机 `.local/cert.pem`、`.local/key.pem`，以及本机已授权的桌面连接上下文。`setup-local.mjs` 使用已有证书建立 Token 和公钥指纹配置，它不会自动生成 HTTPS 证书或创建桌面授权。

当前接入读取安装环境中的 app-tools 服务和桌面监听记录，普通结构化答复还依赖版本相关的本地 IPC。它不是通用、稳定的第三方 SDK；缺少合法调用上下文时应停止配置，不能伪造会话身份或复制其他人的上下文。

每位用户应自行生成证书和凭据，通过自己的受控 USB 路径完成配对。首次配对向导、全新账号接入和跨版本适配尚未完成独立验证，因此本次不提供“一键安装”承诺。不要把桥接端口转发到公网。

## 5. 当前验证不覆盖

- 新电脑上的证书生成、桌面授权、首次配对和实际掌机安装全流程。
- 本次依赖及默认地址改动后的 ARM64 编译、真机显示和真人语音回归。
- 外网、不同固件、不同 GPU、完整电脑/桌面重启恢复。

上述项目通过前，发布定位保持开发者源码预览。具体检查记录见 [PUBLIC_READINESS](PUBLIC_READINESS.md)。
