---
title: Quartz 首页美化方案
---

# Quartz 首页美化方案

> 目标：简洁现代白风格，蓝紫色调，带 Hero 区、卡片导航、GitHub 项目入口、友链、评论区。

---

## 第一步：替换 `index.md`

将 `D:\Basic_learning_record\index.md` 的全部内容替换为以下内容：

```markdown
---
title: 首页
---

<div class="home-hero">
  <img class="home-avatar" src="https://avatars.githubusercontent.com/evil0knight" alt="avatar" />
  <div class="home-hero-text">
    <h1 class="home-title">Ao Yang 的知识库</h1>
    <p class="home-subtitle">嵌入式 · 计算机基础 · 硬件模块 · 持续更新中</p>
    <div class="home-badges">
      <a class="badge badge-github" href="https://github.com/evil0knight" target="_blank">⭐ GitHub</a>
      <a class="badge badge-outline" href="书中自有黄金屋/MOC.md">📚 书</a>
      <a class="badge badge-outline" href="术中自有万钟粟/MOC.md">⚙️ 术</a>
      <a class="badge badge-outline" href="库中车马多如簇/MOC.md">🔩 库</a>
    </div>
  </div>
</div>

---

## 知识分区

<div class="card-grid">
  <a class="card" href="书中自有黄金屋/MOC.md">
    <div class="card-icon">📚</div>
    <div class="card-body">
      <div class="card-title">书中自有黄金屋</div>
      <div class="card-desc">教材学科笔记 · 计算机基础 · 电子电路</div>
    </div>
  </a>
  <a class="card" href="术中自有万钟粟/MOC.md">
    <div class="card-icon">⚙️</div>
    <div class="card-body">
      <div class="card-title">术中自有万钟粟</div>
      <div class="card-desc">实用技术笔记 · Git · Modbus · IC设计</div>
    </div>
  </a>
  <a class="card" href="库中车马多如簇/MOC.md">
    <div class="card-icon">🔩</div>
    <div class="card-body">
      <div class="card-title">库中车马多如簇</div>
      <div class="card-desc">硬件模块笔记 · WiFi · UART · 软件架构</div>
    </div>
  </a>
</div>

---

## GitHub 项目

<div class="card-grid">
  <a class="card card-github-project" href="https://github.com/evil0knight" target="_blank">
    <div class="card-icon">🐙</div>
    <div class="card-body">
      <div class="card-title">evil0knight</div>
      <div class="card-desc">查看我的所有开源项目 →</div>
    </div>
  </a>
</div>

> 后续在这里补充具体项目卡片，格式同上，把 href 改成具体 repo 链接即可。

---

## 友链

<div class="friend-links">
  <a class="friend-card" href="https://quartz.jzhao.xyz" target="_blank">
    <img class="friend-avatar" src="https://avatars.githubusercontent.com/jackyzha0" alt="Jacky Zhao" />
    <div class="friend-info">
      <div class="friend-name">Jacky Zhao</div>
      <div class="friend-desc">Quartz 作者</div>
    </div>
  </a>
  <a class="friend-card" href="填入友链URL" target="_blank">
    <img class="friend-avatar" src="填入头像URL" alt="友链名" />
    <div class="friend-info">
      <div class="friend-name">友链名称</div>
      <div class="friend-desc">一句话介绍</div>
    </div>
  </a>
</div>
```

---

## 第二步：修改 `custom.scss`

文件路径：`D:\quartz\quartz\styles\custom.scss`

将文件内容替换为：

```scss
@use "./base.scss";

/* ===== 首页 Hero ===== */
.home-hero {
  display: flex;
  align-items: center;
  gap: 1.5rem;
  padding: 2rem 0 1.5rem;
  flex-wrap: wrap;
}

.home-avatar {
  width: 90px;
  height: 90px;
  border-radius: 50%;
  border: 3px solid #7c6af7;
  object-fit: cover;
  flex-shrink: 0;
}

.home-title {
  font-size: 1.8rem;
  font-weight: 700;
  margin: 0 0 0.3rem;
  color: var(--dark);
}

.home-subtitle {
  color: var(--darkgray);
  margin: 0 0 0.8rem;
  font-size: 0.95rem;
}

.home-badges {
  display: flex;
  gap: 0.5rem;
  flex-wrap: wrap;
}

/* ===== 徽章 ===== */
.badge {
  display: inline-block;
  padding: 0.3rem 0.8rem;
  border-radius: 999px;
  font-size: 0.85rem;
  font-weight: 600;
  text-decoration: none !important;
  transition: transform 0.15s, box-shadow 0.15s;

  &:hover {
    transform: translateY(-2px);
    box-shadow: 0 4px 12px rgba(124, 106, 247, 0.25);
  }
}

.badge-github {
  background: #7c6af7;
  color: #fff !important;
}

.badge-outline {
  border: 1.5px solid #7c6af7;
  color: #7c6af7 !important;
  background: transparent;
}

/* ===== 卡片网格 ===== */
.card-grid {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(220px, 1fr));
  gap: 1rem;
  margin: 1rem 0;
}

.card {
  display: flex;
  align-items: center;
  gap: 0.9rem;
  padding: 1rem 1.2rem;
  border-radius: 12px;
  background: var(--light);
  border: 1.5px solid var(--lightgray);
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.06);
  text-decoration: none !important;
  transition: transform 0.15s, box-shadow 0.15s, border-color 0.15s;

  &:hover {
    transform: translateY(-3px);
    box-shadow: 0 6px 20px rgba(124, 106, 247, 0.15);
    border-color: #7c6af7;
  }
}

.card-icon {
  font-size: 1.8rem;
  flex-shrink: 0;
}

.card-title {
  font-weight: 600;
  font-size: 0.95rem;
  color: var(--dark);
  margin-bottom: 0.2rem;
}

.card-desc {
  font-size: 0.8rem;
  color: var(--darkgray);
}

/* ===== 友链 ===== */
.friend-links {
  display: flex;
  flex-wrap: wrap;
  gap: 1rem;
  margin: 1rem 0;
}

.friend-card {
  display: flex;
  align-items: center;
  gap: 0.75rem;
  padding: 0.75rem 1rem;
  border-radius: 12px;
  background: var(--light);
  border: 1.5px solid var(--lightgray);
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.06);
  text-decoration: none !important;
  min-width: 180px;
  transition: transform 0.15s, box-shadow 0.15s, border-color 0.15s;

  &:hover {
    transform: translateY(-3px);
    box-shadow: 0 6px 20px rgba(124, 106, 247, 0.15);
    border-color: #7c6af7;
  }
}

.friend-avatar {
  width: 44px;
  height: 44px;
  border-radius: 50%;
  object-fit: cover;
  border: 2px solid var(--lightgray);
  flex-shrink: 0;
}

.friend-avatar-placeholder {
  display: flex;
  align-items: center;
  justify-content: center;
  background: var(--lightgray);
  color: var(--darkgray);
  font-size: 1.2rem;
  font-weight: bold;
}

.friend-name {
  font-weight: 600;
  font-size: 0.9rem;
  color: var(--dark);
}

.friend-desc {
  font-size: 0.78rem;
  color: var(--darkgray);
}
```

---

## 第三步：改站点标题（可选）

文件：`D:\quartz\quartz.config.ts`，第 11 行：

```ts
pageTitle: "Ao Yang 的知识库",
```

---

## 第四步：加评论区（giscus）

评论区需要 GitHub Discussions，步骤：

### 4.1 开启 GitHub Discussions

1. 打开你的笔记仓库（GitHub 页面）
2. Settings → Features → 勾选 **Discussions**

### 4.2 安装 giscus App

访问 [https://github.com/apps/giscus](https://github.com/apps/giscus)，点 Install，选你的仓库。

### 4.3 获取配置参数

访问 [https://giscus.app](https://giscus.app)，填入你的仓库名，它会生成如下参数：

- `repo`：如 `evil0knight/Basic_learning_record`
- `repoId`：一串 base64 ID
- `category`：选 `Announcements`
- `categoryId`：一串 base64 ID

### 4.4 修改 `quartz.layout.ts`

在 `D:\quartz\quartz.layout.ts` 中：

**第 9 行附近，`afterBody` 改为：**

```ts
afterBody: [
  Component.Comments({
    provider: "giscus",
    options: {
      repo: "evil0knight/你的仓库名",
      repoId: "填入repoId",
      category: "Announcements",
      categoryId: "填入categoryId",
      mapping: "pathname",
      strict: false,
      reactionsEnabled: true,
      inputPosition: "top",
    },
  }),
],
```

> 注意：`repoId` 和 `categoryId` 必须从 giscus.app 获取，不能手填。

---

## 后续：添加具体 GitHub 项目卡片

在 `index.md` 的 GitHub 项目区，复制以下模板，每个项目一个卡片：

```html
<a class="card card-github-project" href="https://github.com/evil0knight/项目名" target="_blank">
  <div class="card-icon">🛠️</div>
  <div class="card-body">
    <div class="card-title">项目名</div>
    <div class="card-desc">一句话描述这个项目</div>
  </div>
</a>
```

---

## 效果预览（文字描述）

- **Hero 区**：左侧圆形头像（GitHub 头像自动拉取），右侧名字 + 副标题 + 三个徽章按钮
- **知识分区**：三列卡片，hover 时上浮 + 紫色边框
- **GitHub 项目**：同款卡片，点击跳转 GitHub
- **友链**：横排小卡片，带头像
- **评论区**：页面底部，GitHub Discussions 驱动，支持 emoji 反应
