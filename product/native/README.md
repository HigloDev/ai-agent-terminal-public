# 原生掌机客户端与电脑桥接

本目录包含 TRIMUI Smart Pro ARM64 的 C / SDL2 客户端、Node.js 桥接及 Python 本地语音组件。当前是开发者预览，完整 1.0 尚未验收。

界面采用 1280×720 逻辑画布、64 px 顶栏、36 px 底栏，标准正文 26 px、大字 32 px。全局提供原默认、EVA/NERV、K-ON!、Pokémon 四套皮肤。START 打开外观设置，可独立关闭顶栏、底栏、8% 背景 Logo；选择写入本机 `ui-settings.txt`。问题页 START 仍核对全部答案；草稿中 START 展开/收起长稿。会话操作菜单全幅每页最多六项，主选项至少26 px，13项从实际菜单数组计算。主模块 X 打开七模块菜单，L/R 顺序与原来一致；连接页 X 保留声音操作，看板 X 保留声音、Y 切换常亮、A/START 打开全局外观。

按住 Y 显示 64 px 录音条（最长 600 秒），松开后使用 220 px 草稿抽屉；编辑与键盘使用全幅保护区。发送、创建与原问题提交的未知结果保持锁定，A 只查询原请求回执。审批与停止继续不可用。阅读消息双方全宽左对齐，32 px左右边距、紧凑署名，不使用常态气泡或头像槽。全题核对常显“提交至原请求”和完整会话ID，X可检查完整目标与原请求身份。媒体页面显示文件名/来源和完整媒体ID，明确区分加载、失败与同一身份重试。Logo 使用准确原 SVG，第三方来源与尚未核实的再分发状态见 [skins/SOURCES](skins/SOURCES.md)。

先阅读 [开发配置](../../docs/DEVELOPER_SETUP.md) 和 [公开准备状态](../../docs/PUBLIC_READINESS.md)。使用说明见 [用户指南](../../docs/handheld-codex-1.0/USER_GUIDE.md)，产品目标和验收条件分别见同目录 PRD 与 ACCEPTANCE。

## 电脑测试

```powershell
npm ci --ignore-scripts
npm test
python -m unittest discover -p "test_*.py"
```

测试使用合成数据和临时文件，不需要真实会话、Token、模型或设备。桌面发送和创建仅在用户操作时调用；测试不连接真实桌面执行这些动作。

Linux 云环境安装 SDL2 / SDL_ttf 开发依赖后可运行 `sh build-host.sh`、`sh test-native.sh`。依赖仅解压到工作区时设置 `GM_NATIVE_SYSROOT` 指向其根目录。`sh render-fixtures.sh` 使用真实软件 SDL 渲染 80 个状态 × 四套皮肤，输出 1280×720 PNG（需 Pillow）；所有数据位于隔离临时目录。它验证客户端渲染，不代表已经在掌机运行。`build-arm64-cloud.sh` 支持显式的交叉工具链/目标 libc/SDL 头文件根目录；最终设备兼容性仍待真机验收。

## 构建与设备工具

在 WSL/Linux 配置 ARM64 gcc、SDL2/SDL_ttf 头文件。`prepare-build-libs.ps1 -Serial <自己的设备序列号>` 从本台掌机读取目标库至被忽略的 lib/，然后在 WSL 执行 `sh build.sh`。这些库不随仓库分发。

设备脚本要求显式 `-Serial` 或 `ANDROID_SERIAL`，没有内置维护者设备标识。`install-device.ps1` 用于已有配置的设备更新，会备份并核对文件；它不是完整的首次安装向导。回滚要求自己的本地备份，公开源码不附维护者的 rollback 二进制。实际安装前须确认编译产物、配对和操作路径完整。

## 连接与数据

桥接服务使用 HTTPS/Bearer，掌机固定校验已配对电脑的公钥。发现地址不是信任依据；未发现地址时回环默认值只会失败，不会指向维护者的电脑。

`.local` 存放私钥、配对、桌面上下文、回执和录音。不要上传或复制给其他用户。语音默认使用当前 WSL 用户 `$HOME/funasr-gpu/bin/python`，可通过电脑进程环境变量 `GM_ASR_PYTHON` 指定 WSL 中的绝对路径。模型与 GPU 环境需要自行配置。

桌面适配器依赖已安装的本地工具服务和版本相关的 IPC 协议；普通结构化问题与审批不是同一种操作。审批和停止回合不可用时应保持不可用，不能发送普通文本冒充执行。

## 发布

在干净的 Git 工作树中运行 `build-release.ps1` 生成仅含已提交源码的 ZIP 和校验值。不得直接发布旧 evidence、rollback、录音、凭据或本机生成的安装包。第三方来源及许可见 [NOTICES](NOTICES.md)。
