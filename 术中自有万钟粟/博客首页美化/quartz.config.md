# quartz.config.ts 配置说明

[← 返回 MOC](./MOC.md)

> 实际文件路径：`D:\quartz\quartz.config.ts`
> 这里只记录需要手动维护的配置项，其余保持默认。

---

## 站点标题

```ts
pageTitle: "evil0knight's Blog",
```

修改这里改左上角和浏览器标签页标题。

---

## 主题配色

```ts
colors: {
  lightMode: {
    secondary: "#284b63",   // 链接色
    tertiary: "#84a59d",    // 次要强调色
  },
  darkMode: {
    secondary: "#7b97aa",
    tertiary: "#84a59d",
  },
},
```

想换蓝紫色调可以把 `secondary` 改成 `#7c6af7`（和 custom.scss 保持一致）。

---

## 字体

```ts
typography: {
  header: "Schibsted Grotesk",
  body: "Source Sans Pro",
  code: "IBM Plex Mono",
},
```

换字体直接改这里，填 Google Fonts 上的字体名即可。

---

## 评论区（giscus）配置位置

评论区不在这个文件，在 `D:\quartz\quartz.layout.ts` 的 `afterBody`，参考 MOC.md 的扩展说明。
