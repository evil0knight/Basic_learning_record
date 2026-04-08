# 博客首页美化 — MOC

[← 返回技术总览](../MOC.md)

> 记录 evil0knight's Blog 首页的设计方案与三个核心文件，方便后续扩展维护。

---

## 文件索引

| 文件 | 路径 | 说明 |
| --- | --- | --- |
| 首页内容 | [index.md](./index.md) | 博客首页，改这里加卡片/友链 |
| 样式文件 | [custom.scss](./custom.scss) | 所有自定义 CSS，改这里调样式 |
| 站点配置 | [quartz.config.md](./quartz.config.md) | 站点标题、主题色等配置说明 |
| Quartz 博客配置 | [Quartz博客配置.md](./Quartz博客配置.md) | Quartz 整体配置踩坑记录 |
| 首页美化方案 | [Quartz首页美化方案.md](./Quartz首页美化方案.md) | 美化方案完整步骤文档 |

---

## 如何添加 GitHub 项目卡片

> 不需要懂 HTML，照着下面步骤改就行。

**操作位置**：打开 `D:\Basic_learning_record\index.md`，找到这一行注释：
```
<!-- 在这里追加项目卡片，参考 MOC.md -->
```
在它**上面**粘贴以下代码块，把两处`仓库名`和`一句话描述`换成真实内容：

```html
<a class="card" href="https://github.com/evil0knight/仓库名" target="_blank">
  <div class="card-icon">🛠️</div>
  <div class="card-body">
    <div class="card-title">仓库名</div>
    <div class="card-desc">一句话描述</div>
  </div>
</a>
```

想加几个项目就粘贴几次，每次换不同的仓库名和描述。

---

## 如何添加友链

**操作位置**：同一个 `index.md`，找到这一行注释：
```
<!-- 在这里追加友链，参考 MOC.md -->
```
在它**上面**粘贴以下代码块，替换三处内容：

```html
<a class="friend-card" href="https://对方博客URL" target="_blank">
  <img class="friend-avatar" src="https://avatars.githubusercontent.com/对方GitHub用户名" alt="名字" />
  <div class="friend-info">
    <div class="friend-name">名字</div>
    <div class="friend-desc">一句话介绍</div>
  </div>
</a>
```

| 替换位置 | 填什么 |
| --- | --- |
| `对方博客URL` | 对方博客的网址 |
| `对方GitHub用户名` | 对方的 GitHub ID（头像会自动拉取） |
| `名字` / `一句话介绍` | 显示的名字和描述 |

如果对方没有 GitHub，把整个 `<img .../>` 那行换成：
```html
<div class="friend-avatar friend-avatar-placeholder">😊</div>
```

---

## 后续可扩展功能

| 功能 | 操作 |
| --- | --- |
| 评论区（giscus） | GitHub 仓库开启 Discussions → 去 giscus.app 获取配置 → 改 `quartz.layout.ts` 的 `afterBody` |
| 公告栏 | `index.md` 顶部加 `<div class="banner">` + `custom.scss` 追加样式 |
| 最近更新列表 | `quartz.layout.ts` 的 `right` 数组加 `Component.RecentNotes()` |
| 修改主题配色 | `quartz.config.ts` 的 `theme.colors` 块 |
| 新卡片区块 | `index.md` 加 `<div class="card-grid">` + 若干 `<a class="card">` |

---

## 实际文件位置

| 文件 | 实际路径 |
| --- | --- |
| index.md | `D:\Basic_learning_record\index.md` |
| custom.scss | `D:\quartz\quartz\styles\custom.scss` |
| quartz.config.ts | `D:\quartz\quartz.config.ts` |
| quartz.layout.ts | `D:\quartz\quartz.layout.ts` |
