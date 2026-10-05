# AI Agent Terminal / 掌上助手

TRIMUI Smart Pro 原生掌机客户端，通过局域网连接电脑已有的 Codex Desktop 会话，提供对话阅读、草稿、语音输入、成果预览与普通结构化问题答复。

**开发者源码预览，尚非开箱即用安装版。** 完整 1.0 未完成验收；审批、停止原回合、外网连接、首次配对向导和跨电脑安装仍有边界。桌面接入依赖版本相关的本地接口，不能保证升级后持续兼容。本项目是第三方项目，与 OpenAI 或 TRIMUI 无官方隶属关系。

## 从这里开始

- [开发环境与首次配置](docs/DEVELOPER_SETUP.md)
- [本次公开准备的验证结果与未完成项](docs/PUBLIC_READINESS.md)
- [原生客户端说明](product/native/README.md) · [使用说明](docs/handheld-codex-1.0/USER_GUIDE.md)
- [产品目标](docs/handheld-codex-1.0/PRD.md) · [验收要求](docs/handheld-codex-1.0/ACCEPTANCE.md)
- [参与开发](CONTRIBUTING.md) · [安全说明](SECURITY.md)

## 获取与测试

电脑测试需要 Node.js >=20.9（推荐 22）及 Python 3.13。本次在 Windows 的独立源码副本中安装并测试，没有复制原电脑凭据、录音、模型或 node_modules。

```powershell
git clone https://github.com/HigloDev/ai-agent-terminal-public.git
cd ai-agent-terminal-public/product/native
npm ci --ignore-scripts
npm test
python -m unittest discover -p "test_*.py"
```

这些命令只验证电脑端逻辑，不安装到掌机、不发送真实消息，也不证明语音 GPU 环境或真机体验已验收。完整开发需要另行配置 WSL、交叉编译工具链、目标设备库、本地模型和配对。

## 源码范围

- `product/native/`：C / SDL2 界面、Node.js 桥接、Python 本地语音、测试与设备工具。
- `product/hub/sanitize.mjs`：原生客户端共用的脱敏组件。
- `docs/handheld-codex-1.0/`：设计和操作材料；其中图片为设计示意，不代表实机验收。
- `scripts/`、`.github/`：公开文件检查及自动测试。

早期多 Agent Hub、MiMo/DSH 实验工具与历史交接不属于此次公开源码范围。它们与旧 Git 历史均保留在维护者本机，未纳入公开提交。旧文档中的本机验收记录不是公开包内容；最新验证范围以 PUBLIC_READINESS 为准。

## 数据与发布

真实 `.env`、私钥、配对 Token、会话、录音、模型、设备库、缓存、日志和二进制输出均不提交。尚无 `.env` 配置机制，示例只使用空值或占位符。每个用户须建立自己的桌面连接及配对，不能复制维护者的 `.local`。

`product/native/build-release.ps1` 仅从干净、已提交的 Git 源码生成 ZIP，包含许可证；不会扫描并打包本机 evidence、rollback 或可执行文件。当前不提供已经完成公开验收的二进制安装包。

## 许可证

本项目原创代码采用 **GNU General Public License v3.0 only（SPDX: `GPL-3.0-only`）**。完整条款见 [LICENSE](LICENSE)。

允许商业使用、修改和分发。分发本程序或基于本程序的修改版时，须遵守 GPLv3，保留适用的版权及许可声明、标明修改，并按条款向接收者提供相应源码；分发修改版整体须继续采用 GPLv3。分发二进制时也有相应源码提供义务，不限于修改过的版本。

仅私人修改、使用而不分发，无须公开修改；GPLv3 不要求把所有私人修改上传到公共 GitHub。仅通过网络提供服务、不向用户分发程序，通常也不触发 GPL 的源码提供义务。本程序不提供保证，具体以 LICENSE 为准。

## 第三方资料与许可

第三方源码、字体、图标和其他组件保留各自原有许可证，本项目许可不替代其许可。使用或分发时请核对 [第三方说明](product/native/NOTICES.md)、组件目录的许可证和字体的 `OFL.txt`。本许可声明不代表已完成所有第三方内容的公开分发审查。
