# Quartz 博客配置

> 目标：笔记仓库 `D:\Basic_learning_record` 推送到 GitHub 后，博客自动更新。
> 博客地址：https://evil0knight.github.io/quartz

---

## 原理

```
本地写笔记 → git push → GitHub Actions 自动构建 Quartz → 博客更新
```

---

## 步骤

### 第一步：Fork Quartz 仓库

1. 打开 https://github.com/jackyzha0/quartz
2. 点击右上角 **Fork**，Fork 到自己的 GitHub 账号
3. 仓库名可改为 `blog` 或保持 `quartz`

---

### 第二步：本地克隆 Quartz

在想要放 Quartz 的地方创建文件夹然后执行，比如 `D:\quartz>`：

```powershell
git clone https://github.com/<你的用户名>/quartz.git D:\quartz
cd D:\quartz
npm install
```

---

### 第三步：配置内容路径

把笔记仓库软链接进 Quartz 的 `content/` 目录（管理员 PowerShell）：

```powershell
cd D:\quartz
Remove-Item content -Recurse -Force   # 删掉自带的 content 文件夹
New-Item -ItemType Junction -Path "content" -Target "D:\Basic_learning_record"
```

---

### 第四步：本地预览

```powershell
npx quartz build --serve
```

浏览器打开 http://localhost:8080 预览效果。

**常见报错：**

1. `style="display:inline" is not a valid HTML tag name`
   — 笔记中有 `<h3 style="display:inline">` 写法，Quartz 不支持，需批量替换为 `<h3>`：
   ```bash
   grep -rl 'style="display:inline"' --include="*.md" . | xargs sed -i 's/<h3 style="display:inline">/<h3>/g'
   ```

2. `codepoint not found in map`（OG 图片 emoji 不支持）
   — 在 `D:\quartz\quartz.config.ts` 中注释掉 `Plugin.CustomOgImages()`：
   ```ts
   // Plugin.CustomOgImages(),
   ```

3. 首页 404
   — 在笔记仓库根目录新建 `index.md` 作为首页。

4. 链接跳转到错误页面
   — 在 `quartz.config.ts` 中将链接解析改为 `relative`：
   ```ts
   Plugin.CrawlLinks({ markdownLinkResolution: "relative" }),
   ```

---

### 第五步：部署到 GitHub Pages

新开一个 PowerShell，推送 Quartz 仓库：

```powershell
cd D:\quartz
git add .
git commit -m "init quartz blog"
git push
```

Quartz fork 的仓库**没有** deploy workflow，需手动创建 `D:\quartz\.github\workflows\deploy.yml`：

```yaml
name: Deploy Quartz site to GitHub Pages

on:
  push:
    branches:
      - v4
  repository_dispatch:
    types: [notes-updated]
  workflow_dispatch:

permissions:
  contents: read
  pages: write
  id-token: write

concurrency:
  group: "pages"
  cancel-in-progress: false

jobs:
  build:
    runs-on: ubuntu-22.04
    steps:
      - uses: actions/checkout@v4
        with:
          fetch-depth: 0
      - uses: actions/setup-node@v4
        with:
          node-version: 22
      - name: Install Dependencies
        run: npm ci
      - name: Build Quartz
        run: npx quartz build
      - name: Upload artifact
        uses: actions/upload-pages-artifact@v3
        with:
          path: public

  deploy:
    needs: build
    environment:
      name: github-pages
      url: ${{ steps.deployment.outputs.page_url }}
    runs-on: ubuntu-latest
    steps:
      - name: Deploy to GitHub Pages
        id: deployment
        uses: actions/deploy-pages@v4
```

然后：
1. 进入仓库 **Settings → Pages**，Source 选择 **GitHub Actions**
2. 推送 deploy.yml 后 Actions 会自动触发构建

---

### 第六步：配置 Token（让笔记 push 自动触发博客更新）

**创建 Token：**

1. 打开 https://github.com/settings/personal-access-tokens/new
2. Token name：`blog-trigger`
3. Repository access：Only select repositories → 选 `quartz`
4. Permissions → **Contents** 设为 `Read and write`
5. Generate token，立刻复制（只显示一次，过期可 Regenerate）

> Token 最长有效期 1 年，到期后在 https://github.com/settings/personal-access-tokens 重新 Regenerate，再更新 Secret 即可。

**存入笔记仓库 Secret：**

1. 打开 https://github.com/evil0knight/Basic_learning_record/settings/secrets/actions
2. 点 **New repository secret**
3. Name：`BLOG_TRIGGER_TOKEN`，Secret：粘贴 token
4. 点 **Add secret**

**在笔记仓库创建触发 workflow**（`D:\Basic_learning_record\.github\workflows\trigger-blog.yml`）：

```yaml
name: Trigger Blog Update

on:
  push:
    branches: [main]

jobs:
  trigger:
    runs-on: ubuntu-latest
    steps:
      - name: Trigger Quartz rebuild
        uses: peter-evans/repository-dispatch@v3
        with:
          token: ${{ secrets.BLOG_TRIGGER_TOKEN }}
          repository: evil0knight/quartz
          event-type: notes-updated
```

推送：

```powershell
cd D:\Basic_learning_record
git add .github/workflows/trigger-blog.yml
git commit -m "add blog trigger workflow"
git push --set-upstream origin main
```

---

## 完成后的效果

每次在本地写完笔记执行：

```bash
git push
```

博客在 1~2 分钟内自动更新，地址：https://evil0knight.github.io/quartz

---

## 注意事项

- Quartz 对文件名中的中文支持良好，但 URL 会被编码
- 不想公开的笔记可在 `quartz.config.ts` 中配置 `ignorePatterns` 排除
- 所有链接中的文件扩展名必须小写（`.md` 不能写成 `.MD`），否则 Linux 环境大小写敏感会 404
