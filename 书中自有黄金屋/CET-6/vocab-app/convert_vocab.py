"""
CET-6 高频词汇 PDF 转换脚本
用法：
  1. pip install pdfplumber
  2. python convert_vocab.py
"""

import re
import os
import sys

PDF_PATH = "../赠-大学英语六级-高频词汇.pdf"
OUTPUT_PATH = "words.js"

# 匹配中文字符
CN_PATTERN = re.compile(r'[\u4e00-\u9fff]')
# 匹配音标（/.../ 或 [...] 形式）
PHONETIC_PATTERN = re.compile(r'(/[^/\n]+/|\[[^\]\n]+\])')
# 匹配英文单词开头（允许连字符、撇号）
WORD_START = re.compile(r'^([a-zA-Z][a-zA-Z\s\-\'\.]*)\s+(.*)', re.DOTALL)


def try_import_pdfplumber():
    try:
        import pdfplumber
        return pdfplumber
    except ImportError:
        print("缺少依赖，请先运行：pip install pdfplumber")
        sys.exit(1)


def parse_line(line):
    """
    尝试解析一行文字为 {en, phonetic, zh}
    支持以下格式：
      - abandon /əˈbændən/ vt. 放弃；遗弃
      - 1. abandon /əˈbændən/ vt. 放弃
      - abandon 放弃；遗弃
      - abandon [əˈbændən] 放弃
    """
    line = line.strip()
    if not line:
        return None

    # 去掉行首的序号（如 "1." "1 " "①"）
    line = re.sub(r'^[\d①②③④⑤⑥⑦⑧⑨⑩]+[.\s、]\s*', '', line)

    # 必须包含中文才是有效词条
    if not CN_PATTERN.search(line):
        return None

    # 必须以英文开头
    if not re.match(r'^[a-zA-Z]', line):
        return None

    # 提取音标
    phonetic = ""
    phonetic_match = PHONETIC_PATTERN.search(line)
    if phonetic_match:
        phonetic = phonetic_match.group(1)
        # 分割：音标前是英文，音标后是中文
        before = line[:phonetic_match.start()].strip()
        after = line[phonetic_match.end():].strip()
        en = before
        zh = after
    else:
        # 无音标：英文部分 + 中文部分
        m = WORD_START.match(line)
        if not m:
            return None
        en = m.group(1).strip()
        zh = m.group(2).strip()

    # 清理英文（只保留第一个单词或词组，去掉词性标注混入的部分）
    en = re.split(r'\s{2,}', en)[0].strip()  # 多个空格分割时取第一段

    # 基本验证
    if not en or not zh:
        return None
    if len(en) > 50:  # 英文部分过长，可能解析错误
        return None
    if not CN_PATTERN.search(zh):
        return None

    return {"en": en, "phonetic": phonetic, "zh": zh}


def extract_words_from_pdf(pdfplumber):
    words = []
    seen = set()

    print(f"正在读取 {PDF_PATH} ...")

    with pdfplumber.open(PDF_PATH) as pdf:
        total_pages = len(pdf.pages)
        print(f"共 {total_pages} 页")

        for i, page in enumerate(pdf.pages):
            # 优先尝试表格提取
            tables = page.extract_tables()
            if tables:
                for table in tables:
                    for row in table:
                        if not row:
                            continue
                        # 表格行：尝试拼成一行解析
                        row_text = " ".join(cell for cell in row if cell)
                        word = parse_line(row_text)
                        if word and word["en"].lower() not in seen:
                            seen.add(word["en"].lower())
                            words.append(word)
            else:
                # 纯文本提取
                text = page.extract_text()
                if not text:
                    continue
                for line in text.split('\n'):
                    word = parse_line(line)
                    if word and word["en"].lower() not in seen:
                        seen.add(word["en"].lower())
                        words.append(word)

    return words


def preview_words(words, n=5):
    print(f"\n===== 预览前 {n} 条解析结果 =====")
    for i, w in enumerate(words[:n]):
        phonetic_str = f" {w['phonetic']}" if w['phonetic'] else ""
        print(f"  {i+1}. {w['en']}{phonetic_str} → {w['zh']}")
    print(f"============================")
    print(f"共提取到 {len(words)} 个词条")


def write_output(words):
    lines = ["const WORDS = ["]
    for w in words:
        en = w["en"].replace("\\", "\\\\").replace('"', '\\"')
        phonetic = w["phonetic"].replace("\\", "\\\\").replace('"', '\\"')
        zh = w["zh"].replace("\\", "\\\\").replace('"', '\\"')
        lines.append(f'  {{ "en": "{en}", "phonetic": "{phonetic}", "zh": "{zh}" }},')
    lines.append("];")

    with open(OUTPUT_PATH, "w", encoding="utf-8") as f:
        f.write("\n".join(lines))

    print(f"\n已生成 {OUTPUT_PATH}（{len(words)} 个词条）")


def main():
    if not os.path.exists(PDF_PATH):
        print(f"错误：找不到文件 {PDF_PATH}")
        print("请确保本脚本和PDF文件在同一目录下运行")
        sys.exit(1)

    pdfplumber = try_import_pdfplumber()
    words = extract_words_from_pdf(pdfplumber)

    if not words:
        print("\n提取失败：未能从PDF中解析出词条。")
        print("PDF格式可能特殊，请告知我PDF中文字的样式（如发给我截图），我来调整解析规则。")
        sys.exit(1)

    preview_words(words)

    print("\n格式看起来正确吗？(y/n)")
    ans = input().strip().lower()
    if ans != 'y':
        print("请告知解析错误的地方，我来调整脚本。")
        sys.exit(0)

    write_output(words)
    print("完成！接下来用浏览器打开 index.html 即可。")


if __name__ == "__main__":
    main()
