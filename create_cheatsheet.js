const fs = require('fs');
const {
  Document, Packer, Paragraph, TextRun, Table, TableRow, TableCell,
  AlignmentType, BorderStyle, WidthType, ShadingType, SectionType
} = require('docx');

// ============================================================
// Font size presets — LEFT column slightly smaller, RIGHT bigger
// ============================================================
const SMALL = {
  heading:    22,   // 11pt
  subheading: 18,   // 9pt
  tableLabel: 17,   // 8.5pt
  formula:    16,   // 8pt
  highlight:  18,   // 9pt
  body:       16,   // 8pt
};

const LARGE = {
  heading:    26,   // 13pt
  subheading: 22,   // 11pt
  tableLabel: 20,   // 10pt
  formula:    19,   // 9.5pt
  highlight:  21,   // 10.5pt
  body:       19,   // 9.5pt
};

// ============================================================
// Shared constants
// ============================================================
const DARK_BLUE = "1B3A5C";
const BODY_GRAY  = "555555";
const CELL_BG    = "F3F6F9";
const FORMULA_BG = "F8F8F8";

const NO_BORDER  = { style: BorderStyle.NONE, size: 0 };
const NO_BORDERS = { top: NO_BORDER, bottom: NO_BORDER, left: NO_BORDER, right: NO_BORDER };
const NO_BORDERS_TBL = { top: NO_BORDER, bottom: NO_BORDER, left: NO_BORDER, right: NO_BORDER, insideH: NO_BORDER, insideV: NO_BORDER };
const THIN_BORDER = { style: BorderStyle.SINGLE, size: 1, color: "CCCCCC" };
const THIN_BORDERS = { top: THIN_BORDER, bottom: THIN_BORDER, left: THIN_BORDER, right: THIN_BORDER };

// inner-table column widths (must sum to table width)
const INNER_TABLE_W  = 4900;
const NAME_COL_W     = 1300;
const FORMULA_COL_W  = 3600;

// ============================================================
// Reusable paragraph builders
// ============================================================

/** Section heading — large bold dark-blue 微软雅黑 */
function secHead(text, fonts) {
  return new Paragraph({
    spacing: { before: 60, after: 40 },
    children: [
      new TextRun({ text, font: "微软雅黑", size: fonts.heading, bold: true, color: DARK_BLUE }),
    ],
  });
}

/** Sub-section heading — bold 微软雅黑 */
function subHead(text, fonts) {
  return new Paragraph({
    spacing: { before: 50, after: 20 },
    children: [
      new TextRun({ text, font: "微软雅黑", size: fonts.subheading, bold: true }),
    ],
  });
}

/** Highlight line: "Label: description" — label is bold dark-blue, desc is gray normal */
function hl(label, desc, fonts) {
  return new Paragraph({
    spacing: { before: 10, after: 10 },
    children: [
      new TextRun({ text: label, font: "微软雅黑", size: fonts.highlight, bold: true, color: DARK_BLUE }),
      new TextRun({ text: desc, font: "微软雅黑", size: fonts.body, color: BODY_GRAY }),
    ],
  });
}

/** Formula line — centered, light-bg, Consolas italic */
function fm(text, fonts) {
  return new Paragraph({
    spacing: { before: 10, after: 10 },
    alignment: AlignmentType.CENTER,
    shading: { fill: FORMULA_BG, type: ShadingType.CLEAR },
    children: [
      new TextRun({ text, font: "Consolas", size: fonts.formula, italics: true }),
    ],
  });
}

/** Spacer paragraph */
function sp(pts) {
  return new Paragraph({ spacing: { before: pts * 10, after: 0 }, children: [] });
}

// ============================================================
// Distribution table (二项/泊松/高斯)
// ============================================================
function distRow(name, formula, fonts) {
  const cellMargins = { top: 16, bottom: 16, left: 50, right: 50 };
  return new TableRow({
    children: [
      new TableCell({
        width: { size: NAME_COL_W, type: WidthType.DXA },
        borders: THIN_BORDERS,
        shading: { fill: CELL_BG, type: ShadingType.CLEAR },
        margins: cellMargins,
        children: [
          new Paragraph({
            spacing: { before: 0, after: 0 },
            children: [new TextRun({ text: name, font: "微软雅黑", size: fonts.tableLabel, bold: true })],
          }),
        ],
      }),
      new TableCell({
        width: { size: FORMULA_COL_W, type: WidthType.DXA },
        borders: THIN_BORDERS,
        margins: cellMargins,
        children: [
          new Paragraph({
            spacing: { before: 0, after: 0 },
            children: [new TextRun({ text: formula, font: "Consolas", size: fonts.formula, italics: true })],
          }),
        ],
      }),
    ],
  });
}

function distTable(fonts) {
  return new Table({
    width: { size: INNER_TABLE_W, type: WidthType.DXA },
    columnWidths: [NAME_COL_W, FORMULA_COL_W],
    borders: THIN_BORDERS,
    rows: [
      distRow("二项分布", "P(X=k)=C(n,k)·pᵏ·(1−p)ⁿ⁻ᵏ  E=np  Var=np(1−p)", fonts),
      distRow("泊松分布", "P(X=k)=λᵏe⁻λ/k!  E=λ  Var=λ (n→∞,np→λ)", fonts),
      distRow("高斯分布", "f(x)=1/[σ√(2π)]·exp(−(x−μ)²/(2σ²))", fonts),
    ],
  });
}

// ============================================================
// Content blocks
// ============================================================

/** Page 1 content: probability dist + network metrics */
function page1Content(fonts) {
  return [
    // ── 概率分布 ──
    secHead("概率分布", fonts),
    distTable(fonts),
    hl("中心极限: ", "大量独立同分布之和近似正态", fonts),
    hl("泊松近似: ", "二项(n大p小) → λ=np 泊松", fonts),

    // ── 网络指标 ──
    secHead("网络指标", fonts),

    // 聚类系数
    subHead("聚类系数", fonts),
    fm("Cᵢ = 2Eᵢ/[kᵢ(kᵢ−1)]  C = 3N△/N₃", fonts),
    hl("局部: ", "节点 i 的邻居间实际边数/可能边数", fonts),
    hl("全局: ", "三角形数×3 / 三元组数", fonts),

    // PageRank
    subHead("PageRank", fonts),
    fm("PR(u) = (1−d)/N + d·Σ_{v∈B(u)} PR(v)/L(v)", fonts),
    hl("d=0.85: ", "阻尼系数; B(u)=入链集; L(v)=v出度", fonts),
    hl("", "初始化 PR=1/N, 迭代至收敛", fonts),
  ];
}

/** Page 2 content: algorithms + concepts + BA model */
function page2Content(fonts) {
  return [
    // ── 算法 ──
    secHead("算法", fonts),
    subHead("最小生成树 (MST)", fonts),
    hl("Prim: ", "任意点出发, 每步连最小权重边 O(|V|²)", fonts),
    hl("Kruskal: ", "边升序+并查集判环 O(|E|log|E|)", fonts),

    // ── 概念速查 ──
    secHead("概念速查", fonts),
    hl("平均度: ", "⟨k⟩ = 2M/N, 衡量连接稀疏度", fonts),
    hl("二分图: ", "节点分两互不相交子集, 边仅跨子集", fonts),
    hl("最邻近耦合: ", "节点成环, 仅连左右 K/2 邻居(规则网)", fonts),
    hl("WS 小世界: ", "最近邻耦合 + 以 p 概率随机重连边", fonts),
    hl("NW 小世界: ", "最近邻耦合 + 以 p 概率随机加边(不断开)", fonts),

    // ── BA 无标度模型 ──
    secHead("BA 无标度模型", fonts),
    hl("总度数: ", "Σk = 2mt,  m=每步新增边数", fonts),
    fm("∂kᵢ/∂t = m·kᵢ/Σk = kᵢ/(2t)", fonts),
    fm("∫∂kᵢ/kᵢ = ∫∂t/(2t)  ⟹  kᵢ(t) = m√(t/tᵢ)", fonts),
    fm("P(kᵢ<k) = 1 − m²/k²  ⟹  P(k) = 2m²k⁻³", fonts),
    fm("⟹  P(k) ~ k⁻³  (幂律, 无标度, 方差发散)", fonts),
  ];
}

// ============================================================
// Page builder — 2-column table: left=small, right=large
// ============================================================
function buildPage(contentFn) {
  const colW = 5400;
  const tblW = colW * 2;
  const cellMargins = { top: 40, bottom: 40, left: 60, right: 60 };

  return {
    properties: {
      page: {
        size: { width: 12240, height: 15840 },
        margin: { top: 720, right: 720, bottom: 720, left: 720 },
      },
    },
    children: [
      new Table({
        width: { size: tblW, type: WidthType.DXA },
        columnWidths: [colW, colW],
        borders: NO_BORDERS_TBL,
        rows: [
          new TableRow({
            children: [
              // Left cell — small font
              new TableCell({
                width: { size: colW, type: WidthType.DXA },
                borders: NO_BORDERS,
                margins: cellMargins,
                children: contentFn(SMALL),
              }),
              // Right cell — large font
              new TableCell({
                width: { size: colW, type: WidthType.DXA },
                borders: NO_BORDERS,
                margins: cellMargins,
                children: contentFn(LARGE),
              }),
            ],
          }),
        ],
      }),
    ],
  };
}

// ============================================================
// Assemble document
// ============================================================
const doc = new Document({
  styles: {
    default: {
      document: {
        run: { font: "Microsoft YaHei", size: 22 },
      },
    },
  },
  sections: [
    buildPage(page1Content),
    buildPage(page2Content),
  ],
});

const OUT = "D:/水/小抄_网络科学_新版.docx";
Packer.toBuffer(doc).then(buf => {
  fs.writeFileSync(OUT, buf);
  console.log("Done: " + OUT);
});
