# 原生掌机客户端与电脑桥接

本目录包含 TRIMUI Smart Pro ARM64 的 C / SDL2 客户端、Node.js 桥接及 Python 本地语音组件。当前是开发者预览，完整 1.0 尚未验收。

先阅读 [开发配置](../../docs/DEVELOPER_SETUP.md) 和 [公开准备状态](../../docs/PUBLIC_READINESS.md)。使用说明见 [用户指南](../../docs/handheld-codex-1.0/USER_GUIDE.md)，产品目标和验收条件分别见同目录 PRD 与 ACCEPTANCE。

## 电脑测试

```powershell
npm ci --ignore-scripts
npm test
python -m unittest discover -p "test_*.py"
```

测试使用合成数据和临时文件，不需要真实会话、Token、模型或设备。桌面发送和创建仅在用户操作时调用；测试不连接真实桌面执行这些动作。

## 构建与设备工具

在 WSL/Linux 配置 ARM64 gcc、SDL2/SDL_ttf 头文件。`prepare-build-libs.ps1 -Serial <自己的设备序列号>` 从本台掌机读取目标库至被忽略的 lib/，然后在 WSL 执行 `sh build.sh`。这些库不随仓库分发。

设备脚本要求显式 `-Serial` 或 `ANDROID_SERIAL`，没有内置维护者设备标识。`install-device.ps1` 用于已有配置的设备更新，会备份并核对文件；它不是完整的首次安装向导。回滚要求自己的本地备份，公开源码不附维护者的 rollback 二进制。实际安装前须确认编译产物、配对和操作路径完整。

## 连接与数据

桥接服务使用 HTTPS/Bearer，掌机固定校验已配对电脑的公钥。发现地址不是信任依据；未发现地址时回环默认值只会失败，不会指向维护者的电脑。

`.local` 存放私钥、配对、桌面上下文、回执和录音。不要上传或复制给其他用户。语音默认使用当前 WSL 用户 `$HOME/funasr-gpu/bin/python`，可通过电脑进程环境变量 `GM_ASR_PYTHON` 指定 WSL 中的绝对路径。模型与 GPU 环境需要自行配置。

桌面适配器依赖已安装的本地工具服务和版本相关的 IPC 协议；普通结构化问题与审批不是同一种操作。审批和停止回合不可用时应保持不可用，不能发送普通文本冒充执行。

## 发布

在干净的 Git 工作树中运行 `build-release.ps1` 生成仅含已提交源码的 ZIP 和校验值。不得直接发布旧 evidence、rollback、录音、凭据或本机生成的安装包。第三方来源及许可见 [NOTICES](NOTICES.md)。
