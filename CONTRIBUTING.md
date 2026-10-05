# 参与开发

当前接受原生客户端与电脑桥接的修复、文档完善和可复现测试。先阅读 README 和 docs/PUBLIC_READINESS.md，区分自动测试、真机验证及设计目标。

提交前在 product/native 运行 `npm ci --ignore-scripts`、`npm test` 和 `python -m unittest discover -p "test_*.py"`，再于根目录运行 `node scripts/check-public.mjs`。不要用真实会话发送、实际录音、审批或设备安装来代替隔离测试。

问题报告请说明操作系统、Node 版本、客户端版本、设备固件、最短复现过程和预期结果。只附人工检查过的脱敏文本；不要提交 Token、私钥、Cookie、完整会话、录音或 `.local` 内容。

提交贡献即表示你有权提供这些内容，并同意原创贡献采用项目的 GPL-3.0-only 许可；第三方内容须单独保留来源和原许可。变更说明应列明验证范围，不能把模拟测试写成真机验收。
