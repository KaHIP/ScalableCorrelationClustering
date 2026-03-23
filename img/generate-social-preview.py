#!/usr/bin/env python3
"""Generate the GitHub social preview image (1280x640) for SCC."""

from PIL import Image, ImageDraw, ImageFont
import math
import os

W, H = 1280, 640
OUT_PATH = os.path.join(os.path.dirname(os.path.abspath(__file__)), "social-preview.png")

canvas = Image.new("RGB", (W, H), (26, 26, 46))
draw = ImageDraw.Draw(canvas)

# Gradient background
cx, cy = W // 2, H // 2
for r in range(500, 0, -2):
    frac = r / 500
    c1 = int(26 + (22 - 26) * (1 - frac))
    c2 = int(26 + (33 - 26) * (1 - frac))
    c3 = int(46 + (62 - 46) * (1 - frac))
    draw.ellipse([cx - r * 1.6, cy - r, cx + r * 1.6, cy + r], fill=(c1, c2, c3))

bg = (26, 26, 46)

# Cluster colors (from SVG)
blue = (66, 165, 245)
orange = (255, 138, 101)
purple = (186, 104, 200)
teal_c = (77, 182, 172)

green_pos = (102, 187, 106)   # positive edges
red_neg = (239, 83, 80)       # negative edges

# Cluster definitions: center + nodes
# Cluster 1 (blue) - top left
c1_nodes = [(650, 100), (710, 90), (680, 150), (720, 140)]
# Cluster 2 (orange) - top right
c2_nodes = [(920, 80), (980, 80), (920, 140), (980, 140), (950, 110)]
# Cluster 3 (purple) - bottom left
c3_nodes = [(680, 340), (740, 330), (660, 400), (720, 400)]
# Cluster 4 (teal) - bottom right
c4_nodes = [(940, 330), (1000, 330), (970, 390), (940, 270), (1000, 270)]

clusters = [
    (c1_nodes, blue),
    (c2_nodes, orange),
    (c3_nodes, purple),
    (c4_nodes, teal_c),
]

# Intra-cluster edges (positive, green)
c1_edges = [(0, 1), (0, 2), (1, 3), (2, 3), (1, 2)]
c2_edges = [(0, 1), (0, 2), (1, 3), (2, 3), (0, 4), (1, 4), (2, 4), (3, 4)]
c3_edges = [(0, 1), (0, 2), (1, 3), (2, 3), (0, 3)]
c4_edges = [(0, 1), (0, 2), (1, 2), (3, 4), (0, 3), (1, 4), (3, 2), (4, 2)]

intra_edges_list = [c1_edges, c2_edges, c3_edges, c4_edges]

# Inter-cluster negative edges (pairs of (cluster_i, node_i, cluster_j, node_j))
neg_edges = [
    (0, 1, 1, 0),  # blue to orange
    (0, 3, 1, 2),  # blue to orange
    (0, 2, 2, 0),  # blue to purple
    (1, 2, 3, 3),  # orange to teal
    (1, 3, 3, 4),  # orange to teal
    (2, 1, 3, 0),  # purple to teal
    (2, 3, 3, 2),  # purple to teal
    (0, 3, 3, 3),  # blue to teal
    (1, 3, 2, 1),  # orange to purple
]

# Draw cluster background circles (subtle)
cluster_centers = []
for nodes, color in clusters:
    avg_x = sum(n[0] for n in nodes) / len(nodes)
    avg_y = sum(n[1] for n in nodes) / len(nodes)
    cluster_centers.append((avg_x, avg_y))
    # Dashed circle outline
    radius = 70
    for angle_deg in range(0, 360, 8):
        a1 = math.radians(angle_deg)
        a2 = math.radians(angle_deg + 4)
        x1 = avg_x + radius * math.cos(a1)
        y1 = avg_y + radius * math.sin(a1)
        x2 = avg_x + radius * math.cos(a2)
        y2 = avg_y + radius * math.sin(a2)
        dim_c = tuple(int(v * 0.3) for v in color)
        draw.line([(x1, y1), (x2, y2)], fill=dim_c, width=1)

# Draw negative inter-cluster edges (red, dashed)
for ci, ni, cj, nj in neg_edges:
    x1, y1 = clusters[ci][0][ni]
    x2, y2 = clusters[cj][0][nj]
    length = math.sqrt((x2 - x1) ** 2 + (y2 - y1) ** 2)
    dash_len = 8
    gap_len = 6
    steps = int(length / (dash_len + gap_len))
    for s in range(steps):
        t1 = s * (dash_len + gap_len) / length
        t2 = min((s * (dash_len + gap_len) + dash_len) / length, 1.0)
        sx = x1 + (x2 - x1) * t1
        sy = y1 + (y2 - y1) * t1
        ex = x1 + (x2 - x1) * t2
        ey = y1 + (y2 - y1) * t2
        draw.line([(sx, sy), (ex, ey)], fill=(red_neg[0] // 2, red_neg[1] // 3, red_neg[2] // 3), width=1)

# Draw positive intra-cluster edges (green)
for idx, (nodes, color) in enumerate(clusters):
    for i, j in intra_edges_list[idx]:
        x1, y1 = nodes[i]
        x2, y2 = nodes[j]
        draw.line([(x1, y1), (x2, y2)], fill=(green_pos[0] // 2, green_pos[1] // 2, green_pos[2] // 2), width=2)

# Glow + draw nodes
for nodes, color in clusters:
    for x, y in nodes:
        for r in range(20, 6, -1):
            af = 1 - (r - 6) / 14
            gc = tuple(int(color[k] * 0.12 * af + (1 - 0.12 * af) * bg[k]) for k in range(3))
            draw.ellipse([x - r, y - r, x + r, y + r], fill=gc)
    for x, y in nodes:
        bright = tuple(min(255, int(v * 1.2)) for v in color)
        draw.ellipse([x - 8, y - 8, x + 8, y + 8], fill=color, outline=bright, width=2)

# Multilevel coarsening hint: small arrows between graph stages
# Draw 3 small ">>>" between the graph area center
arr_y = H // 2 + 30
for ax in [810, 830, 850]:
    draw.text((ax, arr_y), ">", fill=(80, 100, 130), font=ImageFont.load_default())

# Fonts
try:
    font_title = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 80)
    font_sub = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 28)
    font_tag = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 22)
    font_legend = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 18)
except OSError:
    font_title = font_sub = font_tag = font_legend = ImageFont.load_default()

text_x = 60

# Title in light blue
draw.text((text_x, 140), "SCC", fill=(79, 195, 247), font=font_title)

# Separator
draw.line([(text_x, 245), (text_x + 480, 245)], fill=(60, 80, 110), width=2)

# Subtitle
draw.text((text_x, 265), "Scalable Correlation", fill=(180, 195, 220), font=font_sub)
draw.text((text_x, 303), "Clustering", fill=(180, 195, 220), font=font_sub)

# Tagline
draw.text((text_x, 370), "Multilevel & memetic algorithms for", fill=(110, 130, 160), font=font_tag)
draw.text((text_x, 400), "signed graph clustering", fill=(110, 130, 160), font=font_tag)

# Badge: Part of KaHIP
badge_y = 470
badge_text = "Part of KaHIP"
badge_bbox = draw.textbbox((0, 0), badge_text, font=font_legend)
bw = badge_bbox[2] - badge_bbox[0]
draw.rounded_rectangle(
    [text_x, badge_y, text_x + bw + 24, badge_y + 32],
    radius=16, fill=(38, 50, 56)
)
draw.text((text_x + 12, badge_y + 5), badge_text, fill=(128, 203, 196), font=font_legend)

# Legend
legend_y = 540
# Positive
draw.line([(text_x, legend_y), (text_x + 25, legend_y)], fill=green_pos, width=3)
draw.text((text_x + 34, legend_y - 10), "+ (attraction)", fill=(160, 160, 160), font=font_legend)
# Negative
for dx in range(0, 25, 10):
    draw.line([(text_x + 180 + dx, legend_y), (text_x + 180 + dx + 5, legend_y)], fill=red_neg, width=3)
draw.text((text_x + 214, legend_y - 10), "- (repulsion)", fill=(160, 160, 160), font=font_legend)

# Bottom tagline
bottom_text = "multilevel coarsening & refinement"
bt_bbox = draw.textbbox((0, 0), bottom_text, font=font_legend)
bt_w = bt_bbox[2] - bt_bbox[0]
draw.text((W - bt_w - 60, H - 40), bottom_text, fill=(84, 110, 122), font=font_legend)

canvas.save(OUT_PATH, "PNG", quality=95)
print(f"Saved {OUT_PATH}")
