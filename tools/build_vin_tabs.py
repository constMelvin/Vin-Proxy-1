import os
import struct
import zlib
import json
import shutil
from PIL import Image, ImageDraw, ImageFont

def get_gt_game_dir():
    local_app = os.environ.get('LOCALAPPDATA', '')
    if local_app:
        return os.path.join(local_app, 'Growtopia', 'game')
    return ''

def get_gt_cache_dir():
    local_app = os.environ.get('LOCALAPPDATA', '')
    if local_app:
        return os.path.join(local_app, 'Growtopia', 'cache', 'game')
    return ''

def load_item_database():
    possible_paths = [
        'resources/decoded_items.json',
        'build/src/Release/resources/decoded_items.json',
        'decoded_items.json'
    ]
    for p in possible_paths:
        if os.path.exists(p):
            with open(p, 'r', encoding='utf-8') as f:
                data = json.load(f)
            return {it['id']: it for it in data['items']}
    return {}

def extract_item_sprite(item_id, item_map, target_size=(64, 64)):
    """Extracts authentic, crisp pixel-art item sprite from Growtopia tiles_page rttex files."""
    it = item_map.get(item_id)
    if not it:
        print(f"[WARN] Item ID {item_id} not found in items database!")
        return None
    
    tex_file = it.get('file_name', '')
    tx = it.get('tex_x', 0)
    ty = it.get('tex_y', 0)

    gt_game = get_gt_game_dir()
    gt_cache = get_gt_cache_dir()

    rttex_path = os.path.join(gt_game, tex_file)
    if not os.path.exists(rttex_path):
        rttex_path = os.path.join(gt_cache, tex_file)
    
    if not os.path.exists(rttex_path):
        print(f"[WARN] Texture file {tex_file} not found for item {item_id} ({it.get('name')})")
        return None

    try:
        with open(rttex_path, 'rb') as f:
            data = f.read()
        
        decomp = zlib.decompress(data[32:])
        # RTTXTR header has height at offset 8, width at offset 12
        h, w = struct.unpack('<II', decomp[8:16])
        raw_rgba = decomp[124:124 + (w * h * 4)]
        
        img = Image.frombytes('RGBA', (w, h), raw_rgba).transpose(Image.FLIP_TOP_BOTTOM)
        tile_size = 32
        x0 = tx * tile_size
        y0 = ty * tile_size
        tile = img.crop((x0, y0, x0 + tile_size, y0 + tile_size))
        
        # 2x integer scaling with NEAREST produces razor-sharp Growtopia pixel art
        return tile.resize(target_size, Image.Resampling.NEAREST)
    except Exception as e:
        print(f"[ERROR] Failed to extract item {item_id}: {e}")
        return None

def build_tabs():
    # 1. Load original btn_tabs1.rttex to get authentic tab geometry and dimensions
    orig_path = os.path.expandvars(r'%LOCALAPPDATA%\Growtopia\interface\large\btn_tabs1.rttex')
    with open(orig_path, 'rb') as f:
        orig_data = f.read()

    orig_decomp = zlib.decompress(orig_data[32:])[124:]
    # True Growtopia texture dimensions: 512 width, 1024 height
    base_img = Image.frombytes('RGBA', (512, 1024), orig_decomp).transpose(Image.FLIP_TOP_BOTTOM)

    # 2. Prepare 512x1024 canvas (top-down coordinates)
    canvas = Image.new('RGBA', (512, 1024), (0, 0, 0, 0))

    # Fonts
    font_path = r'C:\Windows\Fonts\GOTHICB.TTF'
    font_2line = ImageFont.truetype(font_path, 23)
    font_1line = ImageFont.truetype(font_path, 27)

    # 3. Load item database for authentic Growtopia icons
    item_map = load_item_database()

    # Tab 0: Custom VinProxy Blue Diamond Lock
    vin_logo_path = 'VinProxy_Premium_Logo.png'
    if os.path.exists(vin_logo_path):
        vin_logo = Image.open(vin_logo_path).convert('RGBA')
        w_v, h_v = vin_logo.size
        lock = vin_logo.crop((int(w_v * 0.22), int(h_v * 0.42), int(w_v * 0.78), int(h_v * 0.96)))
    else:
        lock = Image.open('app.ico').convert('RGBA')
    lock = lock.resize((64, 64), Image.Resampling.LANCZOS)

    # Extract authentic Growtopia item icons directly from game files:
    # Tab 1: Main Features -> Ubisoft (ID 5956)
    ubisoft_icon = extract_item_sprite(5956, item_map, (64, 64))
    
    # Tab 2: Proxy Logs -> Scroll Bulletin (ID 3524)
    scroll_icon = extract_item_sprite(3524, item_map, (64, 64))
    
    # Tab 3: Hidden Mods -> Mini-Mod (ID 4758)
    minimod_icon = extract_item_sprite(4758, item_map, (64, 64))
    
    # Tab 4: Options -> Wrench from Main Features (ID 32)
    wrench_icon = extract_item_sprite(32, item_map, (64, 64))

    # Clean tabs configuration
    tabs = [
        {"icon": lock,         "text": "VIN\nPROXY"},
        {"icon": ubisoft_icon, "text": "MAIN\nFEATURES"},
        {"icon": scroll_icon,  "text": "PROXY\nLOGS"},
        {"icon": minimod_icon, "text": "HIDDEN\nMODS"},
        {"icon": wrench_icon,  "text": "OPTIONS"}
    ]

    for row_idx, tab in enumerate(tabs):
        y_start = row_idx * 92
        y_end = (row_idx + 1) * 92
        icon = tab["icon"]
        txt = tab["text"]
        f = font_1line if '\n' not in txt else font_2line
        ty = 30 if '\n' not in txt else 18

        # Inactive tab (Column 0: x=0..228)
        inact_crop = base_img.crop((0, y_start, 228, y_end))
        draw_inact = ImageDraw.Draw(inact_crop)
        # Clear previous notebook icon and text (preserve 6px outer authentic border)
        draw_inact.rectangle((6, 6, 222, 86), fill=(84, 135, 155, 255))
        # Paste icon (centered vertically: y=14 on 92px height tab)
        if icon:
            inact_crop.paste(icon, (16, 14), icon)
        # Draw text with drop shadow
        draw_inact.text((94, ty + 2), txt, fill=(15, 25, 30, 255), font=f)
        draw_inact.text((92, ty), txt, fill=(255, 255, 255, 255), font=f)
        canvas.paste(inact_crop, (0, y_start))

        # Active tab (Column 1: x=228..456)
        act_crop = base_img.crop((228, y_start, 456, y_end))
        draw_act = ImageDraw.Draw(act_crop)
        # Clear previous notebook icon and text
        draw_act.rectangle((6, 6, 222, 86), fill=(121, 194, 222, 255))
        # Paste icon
        if icon:
            act_crop.paste(icon, (16, 14), icon)
        # Draw text with drop shadow
        draw_act.text((94, ty + 2), txt, fill=(25, 45, 55, 255), font=f)
        draw_act.text((92, ty), txt, fill=(255, 255, 255, 255), font=f)
        canvas.paste(act_crop, (228, y_start))

    # Save preview image
    canvas.save('vin_tabs_preview.png')
    print('vin_tabs_preview.png generated successfully!')

    # OpenGL texture orientation (FLIP_TOP_BOTTOM)
    gl_canvas = canvas.transpose(Image.FLIP_TOP_BOTTOM)
    raw_rgba = gl_canvas.tobytes()

    # Use the EXACT 124-byte authentic RTTXTR header from btn_tabs1.rttex
    # This guarantees exact orig_h=736, orig_w=456, mipmap entries, and UV coordinate calculations in Growtopia!
    header = bytearray(zlib.decompress(orig_data[32:])[:124])

    # Compress payload (authentic header + raw RGBA)
    payload = bytes(header) + raw_rgba
    compressed = zlib.compress(payload)

    # RTPACK file header (32 bytes)
    file_header = bytearray(32)
    file_header[0:6] = b'RTPACK'
    struct.pack_into('<I', file_header, 8, len(compressed))
    struct.pack_into('<I', file_header, 12, len(payload))
    file_header[16] = 1  # is_compressed = 1

    final_rttex = bytes(file_header) + compressed

    # Deploy to repositories and Growtopia interface folder
    dest_paths = [
        'resources/interface/large/vin_tabs.rttex',
        'build/src/Release/resources/interface/large/vin_tabs.rttex',
        os.path.expandvars(r'%LOCALAPPDATA%\Growtopia\interface\large\vin_tabs.rttex')
    ]

    for p in dest_paths:
        os.makedirs(os.path.dirname(p), exist_ok=True)
        with open(p, 'wb') as f:
            f.write(final_rttex)
        print(f"Installed vin_tabs.rttex to: {p}")

if __name__ == '__main__':
    build_tabs()
