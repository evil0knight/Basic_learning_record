// roots.js - 英语词根词缀数据库 + 自动分析函数
// 算法：先剥前缀 → 再剥后缀 → 最后在剩余词干中匹配词根

// ---- 前缀表（按长度降序，优先匹配最长前缀）----
const PREFIXES = [
  ["counter", "反/对抗"],
  ["under",   "在下/不足"],
  ["ultra",   "超出/极端"],
  ["trans",   "穿越/转变"],
  ["super",   "超级/上方"],
  ["inter",   "之间/相互"],
  ["intra",   "内部"],
  ["intro",   "向内"],
  ["extra",   "超出/额外"],
  ["contra",  "相反/对抗"],
  ["over",    "超过/过度"],
  ["fore",    "前/预先"],
  ["with",    "反对/背离"],
  ["anti",    "反对/对抗"],
  ["auto",    "自动/自身"],
  ["semi",    "半"],
  ["tele",    "远程"],
  ["micro",   "微小"],
  ["macro",   "宏大"],
  ["mono",    "单一"],
  ["post",    "后/之后"],
  ["multi",   "多"],
  ["omni",    "全/一切"],
  ["pre",     "前/预先"],
  ["pro",     "向前/支持"],
  ["mis",     "错误/不当"],
  ["out",     "超过/外"],
  ["dis",     "否定/分开"],
  ["non",     "否定"],
  ["uni",     "单一"],
  ["bi",      "二/两"],
  ["tri",     "三"],
  ["sub",     "在下/次"],
  ["sup",     "在下(sub变体)"],
  ["suf",     "在下(sub变体)"],
  ["suc",     "在下(sub变体)"],
  ["sug",     "在下(sub变体)"],
  ["sur",     "超过/上面"],
  ["per",     "彻底/贯穿"],
  ["con",     "共同/加强"],
  ["com",     "共同/加强"],
  ["col",     "共同(con变体)"],
  ["cor",     "共同(con变体)"],
  ["syn",     "共同/同时"],
  ["sym",     "共同/同时"],
  ["re",      "再/回"],
  ["ex",      "出/前"],
  ["en",      "使成为/进入"],
  ["em",      "使成为/进入"],
  ["de",      "向下/去除"],
  ["ob",      "对抗/朝向"],
  ["op",      "对抗(ob变体)"],
  ["oc",      "对抗(ob变体)"],
  ["co",      "共同"],
  ["ab",      "离开/背离"],
  ["ad",      "朝向/加强"],
  ["af",      "朝向(ad变体)"],
  ["ag",      "朝向(ad变体)"],
  ["al",      "朝向(ad变体)"],
  ["ap",      "朝向(ad变体)"],
  ["as",      "朝向(ad变体)"],
  ["at",      "朝向(ad变体)"],
  ["in",      "否定/向内"],
  ["im",      "否定/向内"],
  ["ir",      "否定(in变体)"],
  ["il",      "否定(in变体)"],
  ["un",      "否定"],
  ["up",      "向上"],
];

// ---- 词根表（按长度降序，优先匹配最长词根）----
const ROOTS = [
  ["termina",  "终止/边界"],
  ["termin",   "终止/边界"],
  ["struct",   "建造/构建"],
  ["script",   "写"],
  ["scrib",    "写"],
  ["nounce",   "宣布/说"],
  ["nounc",    "宣布/说"],
  ["stitut",   "建立/站立"],
  ["volut",    "滚动/演变"],
  ["volv",     "滚动/旋转"],
  ["gress",    "步行/前进"],
  ["press",    "压"],
  ["claim",    "呼喊/宣称"],
  ["clam",     "呼喊"],
  ["tract",    "拉/拖"],
  ["ceive",    "抓取/接受"],
  ["cept",     "抓取/接受"],
  ["clude",    "关闭"],
  ["clus",     "关闭"],
  ["rupt",     "断裂"],
  ["dict",     "说/命令"],
  ["duct",     "引导"],
  ["fect",     "做/制造"],
  ["form",     "形状/形式"],
  ["flect",    "弯曲"],
  ["flex",     "弯曲"],
  ["frag",     "断裂"],
  ["frac",     "断裂"],
  ["grad",     "步走/等级"],
  ["sens",     "感觉"],
  ["sent",     "感觉/情绪"],
  ["sign",     "标记/符号"],
  ["simil",    "相似"],
  ["simul",    "相似/模拟"],
  ["solv",     "松开/解决"],
  ["solu",     "解决"],
  ["spect",    "看/检查"],
  ["spec",     "看/检查"],
  ["tend",     "伸展/倾向"],
  ["tens",     "伸展/紧张"],
  ["terr",     "土地/领土"],
  ["text",     "编织/文本"],
  ["tain",     "持有/保持"],
  ["tort",     "扭曲"],
  ["turb",     "搅动/混乱"],
  ["vers",     "转/变"],
  ["vert",     "转/变"],
  ["miss",     "发送"],
  ["manu",     "手"],
  ["migr",     "迁移/移动"],
  ["nunc",     "宣布"],
  ["path",     "感情/痛苦"],
  ["phon",     "声音"],
  ["port",     "携带/运输"],
  ["pend",     "悬挂/称量"],
  ["ped",      "脚"],
  ["pel",      "驱使/推动"],
  ["puls",     "驱使/冲动"],
  ["plic",     "折叠"],
  ["plex",     "折叠/复杂"],
  ["pon",      "放置"],
  ["pos",      "放置"],
  ["voca",     "声音/呼叫"],
  ["voc",      "声音/呼叫"],
  ["vok",      "呼叫"],
  ["vit",      "生命"],
  ["viv",      "生命/活"],
  ["vis",      "看/视觉"],
  ["vid",      "看/视觉"],
  ["duc",      "引导"],
  ["aud",      "听"],
  ["cred",     "相信"],
  ["gen",      "产生/出生"],
  ["grat",     "感激/高兴"],
  ["hab",      "拥有/居住"],
  ["hib",      "抑制/持有"],
  ["lect",     "选择/读"],
  ["loc",      "地方/位置"],
  ["mit",      "发送"],
  ["mob",      "移动"],
  ["mot",      "移动"],
  ["mov",      "移动"],
  ["nat",      "出生/自然"],
  ["nom",      "名称/规则"],
  ["nov",      "新的"],
  ["secu",     "跟随"],
  ["sequ",     "跟随/顺序"],
  ["son",      "声音"],
  ["sol",      "单独/太阳"],
  ["sist",     "站立/坚持"],
  ["val",      "价值/力量"],
  ["vac",      "空的"],
  ["ven",      "来/到来"],
  ["vent",     "来/到来"],
  ["log",      "言语/逻辑"],
  ["fin",      "结束/边界"],
  ["fac",      "做/制造"],
  ["fer",      "携带/承受"],
  ["fund",     "基础/底部"],
  ["fus",      "倾倒/融合"],
  ["grad",     "步/等级"],
  ["graph",    "写/记录"],
  ["lum",      "光"],
  ["luc",      "光"],
  ["man",      "手"],
  ["mort",     "死亡"],
  ["part",     "部分"],
  ["sta",      "站立/停止"],
  ["stat",     "站立/状态"],
  ["urb",      "城市"],
  ["min",      "小/最小"],
  ["mag",      "大"],
  ["medic",    "医疗/医学"],
  ["medi",     "中间"],
  ["ment",     "心智/思考"],
  ["numer",    "数字"],
  ["or",       "说话/嘴"],
  ["pater",    "父亲"],
  ["phot",     "光"],
  ["psych",    "心理/灵魂"],
  ["rupt",     "断裂"],
  ["sci",      "知道"],
  ["scop",     "看/观察"],
  ["tele",     "远程"],
  ["therm",    "热"],
  ["tom",      "切割"],
];

// ---- 后缀表（按长度降序，优先匹配最长后缀）----
const SUFFIXES = [
  ["ification", "名词化"],
  ["ization",   "名词化"],
  ["isation",   "名词化"],
  ["ousness",   "名词/多"],
  ["iveness",   "名词/倾向"],
  ["fulness",   "名词/充满"],
  ["itarian",   "人/主义者"],
  ["escence",   "过程/状态"],
  ["ication",   "名词/动作"],
  ["istence",   "名词/存在"],
  ["ational",   "形容词"],
  ["ically",    "副词"],
  ["lessly",    "副词/无"],
  ["ation",     "名词/动作"],
  ["ition",     "名词/动作"],
  ["ical",      "形容词/关于"],
  ["sion",      "名词/动作"],
  ["tion",      "名词/动作"],
  ["ment",      "名词/结果"],
  ["ness",      "名词/状态"],
  ["ance",      "名词/状态"],
  ["ence",      "名词/状态"],
  ["ious",      "形容词/多"],
  ["eous",      "形容词"],
  ["ous",       "形容词/多"],
  ["ify",       "使成/动词"],
  ["ize",       "使成/动词"],
  ["ise",       "使成/动词"],
  ["ate",       "使成/动词"],
  ["ive",       "形容词/倾向"],
  ["ary",       "形容词/名词"],
  ["ery",       "名词/场所"],
  ["ory",       "名词/形容词"],
  ["ist",       "人/信仰者"],
  ["ism",       "主义/行为"],
  ["ity",       "名词/性质"],
  ["ful",       "充满"],
  ["less",      "无/缺乏"],
  ["ship",      "身份/状态"],
  ["hood",      "状态/群体"],
  ["ward",      "方向"],
  ["able",      "可以/能够"],
  ["ible",      "可以/能够"],
  ["al",        "形容词/关于"],
  ["ic",        "形容词/关于"],
  ["ly",        "副词"],
  ["er",        "人/做者"],
  ["or",        "人/做者"],
  ["ant",       "人/形容词"],
  ["ent",       "人/形容词"],
];

/**
 * 分析单词的词根词缀结构
 * @param {string} word - 英文单词
 * @returns {Array|null} 成分数组（每项 {type:'prefix'|'root'|'stem'|'suffix', text, meaning}），无法识别则返回 null
 */
function analyzeWord(word) {
  const lower = word.toLowerCase().replace(/[^a-z]/g, "");
  if (lower.length < 5) return null;

  let stem = lower;
  let foundPrefix = null;
  let foundSuffix = null;
  let foundRoot   = null;

  // Step 1: 匹配前缀（最长优先）
  for (const [pfx, meaning] of PREFIXES) {
    // 前缀匹配后，剩余部分至少3个字母
    if (stem.startsWith(pfx) && stem.length - pfx.length >= 3) {
      foundPrefix = { text: pfx, meaning };
      stem = stem.slice(pfx.length);
      break;
    }
  }

  // Step 2: 匹配后缀（最长优先，剩余部分至少2个字母）
  for (const [sfx, meaning] of SUFFIXES) {
    if (stem.endsWith(sfx) && stem.length - sfx.length >= 2) {
      foundSuffix = { text: sfx, meaning };
      stem = stem.slice(0, stem.length - sfx.length);
      break;
    }
  }

  // Step 3: 在剩余词干中匹配词根（最长优先）
  for (const [root, meaning] of ROOTS) {
    if (stem.includes(root)) {
      foundRoot = { text: root, meaning };
      break;
    }
  }

  // 至少需要识别出2个成分才有意义
  const hits = [foundPrefix, foundSuffix, foundRoot].filter(Boolean).length;
  if (hits < 2) return null;

  // 组装结果
  const parts = [];
  if (foundPrefix) {
    parts.push({ type: "prefix", text: foundPrefix.text + "-", meaning: foundPrefix.meaning });
  }
  if (foundRoot) {
    parts.push({ type: "root", text: foundRoot.text, meaning: foundRoot.meaning });
  } else if (stem && stem.length >= 2) {
    // 有前缀或后缀但没匹配到已知词根，直接展示词干
    parts.push({ type: "stem", text: stem, meaning: "" });
  }
  if (foundSuffix) {
    parts.push({ type: "suffix", text: "-" + foundSuffix.text, meaning: foundSuffix.meaning });
  }

  if (parts.length < 2) return null;
  return parts;
}
