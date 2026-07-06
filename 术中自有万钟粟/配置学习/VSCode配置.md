# VSCode 配置

[← 技术知识地图](./MOC.md)

---

## 用户设置 (settings.json)

```json
{
    "workbench.colorTheme": "GitHub Light",
    "editor.fontSize": 25,
    "workbench.secondarySideBar.defaultVisibility": "hidden",
    "security.workspace.trust.untrustedFiles": "open",
    "terminal.integrated.fontSize": 17,
    "explorer.confirmDelete": false,
    "workbench.startupEditor": "none",
    "files.autoSave": "afterDelay",

    // ==================== 代码补全设置 ====================
    // "editor.quickSuggestions": {
    //     "other": true,
    //     "comments": false,
    //     "strings": false
    // },
    // "editor.suggestOnTriggerCharacters": true,
    // "editor.wordBasedSuggestions": "matchingDocuments",
    // "editor.acceptSuggestionOnEnter": "on",
    // "editor.inlineSuggest.enabled": true,
    // "editor.suggest.showWords": true,
    // "editor.suggest.showSnippets": true,
    "editor.quickSuggestions": {
    "other": "off",
    "comments": "off",
    "strings": "off"
     },
    "editor.suggestOnTriggerCharacters": false,
     "editor.wordBasedSuggestions": "off",
     "editor.acceptSuggestionOnEnter": "off",
     "editor.inlineSuggest.enabled": false,
     "editor.suggest.showWords": false,
     "editor.suggest.showSnippets": false
// ==================== 代码补全设置 ====================
}
```
