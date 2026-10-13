# 展示站点资源与发布说明

本页为 AI Agent Terminal / 掌上助手的静态产品展示站点。站点源文件位于 `docs/index.html`、`docs/site.css` 和 `docs/site.js`；所有图片、字体和脚本均从本站本地加载，无登录、后台、统计跟踪或外部字体请求。

## 真实界面图

12 张 WebP 均由原生客户端代码的 `render-fixtures.sh` 在云端隔离环境生成，源代码提交为 `88b878f310e1d17f464507d685b3d31ef5e7bb23`，分支为 `ui/handheld-four-skins-20261012`。原图尺寸为 1280 × 720，WebP 为无损编码，已逐像素核对与渲染 PNG 一致。详见 [资源摘要](site-assets/render-provenance.json) 和 [渲染源码校验](site-assets/render-source-hashes.json)。

展示场景为 `chat-latest`、`draft-drawer` 与 `file-diff`，每个场景分别渲染四种外观。所有会话、文件、请求及内容均为合成夹具；没有连接真实桌面、掌机、录音或凭据，没有发送真实消息。页面的掌机框是原创 CSS 示意，不是真机照片。截图只能证明该代码版本在隔离环境中的界面渲染，不能证明真实硬件、录音 ASR 或桌面联动已经验收。该界面开发分支尚未合并主分支。

### 主题标识：保留原始说明

Logo inputs must be the exact supplied SVGs. No characters or recreated marks.

| 文件 | 原始来源 |
| --- | --- |
| nerv.svg | https://cdn.freebiesupply.com/logos/large/2x/nerv-logo-svg-vector.svg |
| kon.svg | https://upload.wikimedia.org/wikipedia/commons/1/17/K-ON_anime_wordmark.svg |
| pokemon.svg | https://upload.wikimedia.org/wikipedia/commons/9/98/International_Pok%C3%A9mon_logo.svg |

这些 Logo 是独立第三方资源，不由项目的 GPL-3.0-only 许可覆盖；权利人再分发许可尚未核实。不主张官方隶属、合作或背书。原程序仅以 20/255（约 7.84%）透明度展示 Logo，默认皮肤不含 Logo。本网站没有另外复制、重绘或添加主题 Logo，标识只存在于真实程序截图中。

仓库 2026-10-12 的来源记录说明：Wikimedia Commons 将 K-ON! wordmark 与 Pokémon logo 标记为 public-domain text logos，同时提示商标限制；这些标签不是本项目的法律认定。NERV 下载页没有明确的再分发许可，其状态仍未解决。完整原始说明、来源与对应资源见 [skins/SOURCES.md](https://github.com/HigloDev/ai-agent-terminal-public/blob/ui/handheld-four-skins-20261012/product/native/skins/SOURCES.md)，第三方声明见 [NOTICES.md](https://github.com/HigloDev/ai-agent-terminal-public/blob/ui/handheld-four-skins-20261012/product/native/NOTICES.md)。

## 字体与原创素材

本站字体是仓库内 Noto Sans SC Regular 的本站字符子集，采用 SIL Open Font License，完整原始许可保存在 [site-assets/OFL.txt](site-assets/OFL.txt)。字体仅做字符子集和 WOFF2 格式转换。掌机外框、设备线稿、项目站点图标、网站版式与交互由本项目制作。网站原创代码适用仓库 GPL-3.0-only；第三方保留其原许可证与商标权利。

## GitHub Pages 发布来源

网站专用分支：`site/product-showcase-20261013`。选择 GitHub 仓库 Settings → Pages → Deploy from a branch → 此分支 → `/docs`，即可发布静态页面。`docs/.nojekyll` 禁用 Jekyll 转换。不需要新增 GitHub Actions workflow，不合并应用 PR #1，也不修改主程序或认证设置。

请以 GitHub Pages 返回的实际发布 URL 与部署状态为准；只有页面已通过 HTTPS 实际访问验证后，才可将状态记为“已上线”。
