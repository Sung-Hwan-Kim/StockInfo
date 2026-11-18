"""StockInfo 아이콘 생성 스크립트.

사용법:  python tools/make_icon.py
결과물:  resources/icons/StockInfo.ico, StockInfo.icns, icon_<크기>.png
필요:    Pillow
"""
from pathlib import Path

from PIL import Image, ImageDraw, ImageFilter

S = 1024  # 작업 해상도 (축소하면서 안티앨리어싱)
OUT = Path(__file__).resolve().parent.parent / "resources" / "icons"

RED = (255, 77, 79)
BLUE = (64, 128, 255)


def lerp(a, b, t):
    return tuple(int(a[i] + (b[i] - a[i]) * t) for i in range(len(a)))


def render(small: bool = False) -> Image.Image:
    """small=True: 16~24px용 (선을 굵게, 격자 생략)"""
    img = Image.new("RGBA", (S, S), (0, 0, 0, 0))

    # 둥근 사각형 배경 (세로 그라데이션)
    margin, radius = 56, 210
    grad = Image.new("RGBA", (S, S))
    gd = ImageDraw.Draw(grad)
    top, bottom = (40, 50, 72), (14, 18, 28)
    for y in range(S):
        gd.line([(0, y), (S, y)], fill=lerp(top, bottom, y / S) + (255,))
    mask = Image.new("L", (S, S), 0)
    ImageDraw.Draw(mask).rounded_rectangle(
        [margin, margin, S - margin, S - margin], radius=radius, fill=255)
    img.paste(grad, (0, 0), mask)

    # 반투명 요소는 별도 레이어에 그린 뒤 합성 (직접 그리면 알파가 덮어써져 구멍이 남)
    overlay = Image.new("RGBA", (S, S), (0, 0, 0, 0))
    od = ImageDraw.Draw(overlay)
    od.rounded_rectangle([margin, margin, S - margin, S - margin], radius=radius,
                         outline=(255, 255, 255, 46), width=10)
    if not small:
        for y in (330, 500, 670):
            od.line([(margin + 90, y), (S - margin - 90, y)], fill=(255, 255, 255, 26), width=6)
    img.alpha_composite(overlay)

    d = ImageDraw.Draw(img)

    # 캔들 (하락 파랑 / 상승 빨강) - 오른쪽으로 갈수록 상승
    candles = [
        # x중심, 몸통위, 몸통아래, 꼬리위, 꼬리아래, 색
        (250, 600, 760, 560, 800, BLUE),
        (420, 520, 700, 470, 740, RED),
        (590, 560, 660, 510, 700, BLUE),
        (760, 330, 600, 280, 640, RED),
    ]
    body_w = 104
    for x, bt, bb, wt, wb, col in candles:
        d.line([(x, wt), (x, wb)], fill=col + (255,), width=22)
        d.rounded_rectangle([x - body_w // 2, bt, x + body_w // 2, bb], radius=14, fill=col + (255,))

    # 상승 추세선 + 화살표 (흰색, 그림자)
    pts = [(170, 690), (400, 520), (560, 600), (800, 300)]
    shadow = Image.new("RGBA", (S, S), (0, 0, 0, 0))
    sd = ImageDraw.Draw(shadow)
    line_w = 92 if small else 46
    sd.line([(x + 10, y + 14) for x, y in pts], fill=(0, 0, 0, 150), width=line_w + 12, joint="curve")
    shadow = shadow.filter(ImageFilter.GaussianBlur(14))
    img.alpha_composite(shadow)

    d = ImageDraw.Draw(img)
    white = (255, 255, 255, 255)
    d.line(pts, fill=white, width=line_w, joint="curve")
    r = line_w // 2
    for x, y in pts[:-1]:
        d.ellipse([x - r, y - r, x + r, y + r], fill=white)
    # 화살촉 (마지막 선분 방향)
    import math
    (x1, y1), (x2, y2) = pts[-2], pts[-1]
    ang = math.atan2(y2 - y1, x2 - x1)
    L, W = (190, 140) if small else (150, 100)
    tip = (x2 + math.cos(ang) * 40, y2 + math.sin(ang) * 40)
    back = (tip[0] - math.cos(ang) * L, tip[1] - math.sin(ang) * L)
    left = (back[0] + math.cos(ang + math.pi / 2) * W, back[1] + math.sin(ang + math.pi / 2) * W)
    right = (back[0] - math.cos(ang + math.pi / 2) * W, back[1] - math.sin(ang + math.pi / 2) * W)
    d.polygon([tip, left, right], fill=white)
    return img


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    big = render()
    small = render(small=True)

    def at(size):
        return (small if size <= 24 else big).resize((size, size), Image.LANCZOS)

    for size in (16, 24, 32, 48, 64, 128, 256, 512):
        at(size).save(OUT / f"icon_{size}.png")

    ico_sizes = [16, 20, 24, 32, 40, 48, 64, 128, 256]
    frames = [at(sz) for sz in ico_sizes]
    frames[-1].save(OUT / "StockInfo.ico", format="ICO",
                    sizes=[(sz, sz) for sz in ico_sizes], append_images=frames[:-1])
    big.save(OUT / "StockInfo.icns", format="ICNS")
    print("saved to", OUT)


if __name__ == "__main__":
    main()
