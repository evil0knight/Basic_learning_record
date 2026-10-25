#!/usr/bin/env python
"""本地语音转写：视频或音频 -> 带时间戳的纯文本。

用途：视频拿不到字幕时的兜底。不下载视频、不处理字幕，只做转写。

用法：
    python transcribe.py 输入文件 [--model small] [--language zh] [--out 结果.txt]

依赖：faster-whisper（pip install faster-whisper）、ffmpeg 在 PATH。
"""

from __future__ import annotations

import argparse
import os
import shutil
import subprocess
import sys
import wave

import numpy as np
from faster_whisper import WhisperModel

# Windows 控制台默认 GBK，转写结果里的中文会报 UnicodeEncodeError
for stream in (sys.stdout, sys.stderr):
    if hasattr(stream, "reconfigure"):
        stream.reconfigure(encoding="utf-8", errors="replace")

FFMPEG_FALLBACK = (
    r"C:\Users\17443\AppData\Local\Microsoft\WinGet\Packages"
    r"\Gyan.FFmpeg_Microsoft.Winget.Source_8wekyb3d8bbwe"
    r"\ffmpeg-9.0-full_build\bin\ffmpeg.EXE"
)


def find_ffmpeg() -> str:
    path = shutil.which("ffmpeg")
    if path:
        return path
    if os.path.exists(FFMPEG_FALLBACK):
        return FFMPEG_FALLBACK
    sys.exit("找不到 ffmpeg，请先安装并加入 PATH")


def to_pcm16k(src: str, work_dir: str) -> str:
    """统一转成 16kHz 单声道 16bit WAV。

    必须走这条路的两个原因：
    1. faster-whisper 的 model.transcribe(path) 依赖 av 解码，av 版本一变就会报
       TypeError: open() got an unexpected keyword argument 'metadata_errors'。
    2. 直接把 numpy 数组喂给 transcribe 可以完全绕过 av。
    """
    stem = os.path.splitext(os.path.basename(src))[0]
    dst = os.path.join(work_dir, f"{stem}.16k.wav")
    if os.path.exists(dst) and os.path.getmtime(dst) >= os.path.getmtime(src):
        return dst
    subprocess.run(
        [find_ffmpeg(), "-y", "-loglevel", "error", "-i", src,
         "-vn", "-ac", "1", "-ar", "16000", "-c:a", "pcm_s16le", dst],
        check=True,
    )
    return dst


def read_wav(path: str) -> np.ndarray:
    with wave.open(path, "rb") as w:
        if w.getsampwidth() != 2:
            sys.exit(f"{path} 不是 16bit PCM，转换步骤可能失败")
        frames = w.readframes(w.getnframes())
    return np.frombuffer(frames, dtype=np.int16).astype(np.float32) / 32768.0


def format_ts(seconds: float) -> str:
    seconds = int(seconds)
    return f"[{seconds // 60:02d}:{seconds % 60:02d}]"


def main() -> int:
    ap = argparse.ArgumentParser(description="本地语音转写")
    ap.add_argument("input", help="音频或视频文件")
    ap.add_argument("-o", "--out", help="输出 txt 路径，默认同目录下 <文件名>.txt")
    ap.add_argument("-m", "--model", default="small",
                    help="模型大小：tiny/base/small/medium/large-v3，默认 small")
    ap.add_argument("-l", "--language", default="zh", help="语言代码，默认 zh")
    ap.add_argument("--device", default="cpu", help="cpu 或 cuda")
    ap.add_argument("--compute-type", default="int8",
                    help="int8 / float16 / float32，CPU 用 int8")
    ap.add_argument("--force", action="store_true", help="覆盖已有输出")
    args = ap.parse_args()

    if not os.path.exists(args.input):
        sys.exit(f"找不到输入文件：{args.input}")

    work_dir = os.path.dirname(os.path.abspath(args.input))
    out_path = args.out or os.path.join(
        work_dir, os.path.splitext(os.path.basename(args.input))[0] + ".txt"
    )
    if os.path.exists(out_path) and not args.force:
        print(f"输出已存在，跳过：{out_path}（加 --force 覆盖）")
        return 0

    wav = to_pcm16k(args.input, work_dir)
    audio = read_wav(wav)
    print(f"音频时长 {len(audio) / 16000 / 60:.1f} 分钟，模型 {args.model}", flush=True)

    model = WhisperModel(args.model, device=args.device, compute_type=args.compute_type)
    segments, _ = model.transcribe(
        audio,
        language=args.language,
        task="transcribe",
        vad_filter=True,
        beam_size=5,
        # 关掉上下文条件，否则长音频容易陷入重复幻觉（同一句话循环）。
        condition_on_previous_text=False,
        compression_ratio_threshold=2.4,
        no_speech_threshold=0.6,
    )

    lines: list[str] = []
    with open(out_path, "w", encoding="utf-8") as fh:
        for seg in segments:
            line = f"{format_ts(seg.start)} {seg.text.strip()}"
            lines.append(line)
            fh.write(line + "\n")
            fh.flush()
            print(line, flush=True)

    print(f"DONE {len(lines)} 段 -> {out_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
