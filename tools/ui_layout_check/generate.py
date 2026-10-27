import ast
from pathlib import Path
import re
import sys

quote = r'"(?:[^"\\]|\\.)*"'
arg = '(?:' + quote + r'\s*)+'
pattern = re.compile(r'android_text\(\s*(' + arg + r'),\s*(' + arg + r')\)')
translations = {}
for path in [Path('src/dusk/language.cpp'), *Path('src/dusk/ui').glob('*.cpp')]:
    for match in pattern.finditer(path.read_text(encoding='utf-8')):
        en, fr = [''.join(ast.literal_eval(s) for s in re.findall(quote, group)) for group in match.groups()]
        translations[en] = fr
keys = ['Disc Image', 'Data Folder', 'Language', 'Graphics Backend', 'Save File Type',
        'Show FPS Counter', 'Pause on Focus Lost', 'Remember Window Size', 'Internal Resolution',
        'Shadow Resolution', 'Output Resampling', 'Enable Texture Replacements', 'Unlock Framerate',
        'Framerate Cap', 'Disable Cutscene Pillarboxing', 'Disable Letterboxing', 'Allow Background Inputs',
        'Touch Controls', 'Touch Targeting', 'Touch Camera X Sensitivity', 'Touch Camera Y Sensitivity',
        'Free Camera X Sensitivity', 'Free Camera Y Sensitivity', 'Invert First Person X Axis',
        'Invert First Person Y Axis', 'Gyro Pitch Sensitivity', 'Gyro Yaw Sensitivity',
        'Mouse Aim Sensitivity', 'Mouse Camera Sensitivity', 'Mouse/Touch in Menus',
        'No Low HP Sound', "Non-Stop Midna's Lament", 'Minimal HUD', 'HUD Scale',
        'Restore Wii 1.0 Glitches', 'Hide Owl Statue Markers', 'Disable Rupee Cutscenes',
        'Hold B for Instant Text', 'Hold Button to Mash', 'No Climbing Miss Animation',
        'Show Poe Count on Map', 'Magic Armor Behavior', 'Skip Dusklight Main Menu',
        'Show Shader Compilation', 'Check for Dusklight Updates', 'Enable Discord Rich Presence',
        'Enable Advanced Settings', 'Show Input Viewer', 'Show Gyro Input Viewer',
        'Menu Scaling Mode', 'Skip TV Settings Screen', 'Enable LED Status',
        'Emulate Triggers', 'Custom Action Bindings']
values = {'Disc Image': 'The Legend of Zelda - Twilight Princess (Europe).rvz',
          'Data Folder': '/storage/emulated/0/Android/data/dev.twilitrealm.dusk/files',
          'Language': translations['French'], 'Graphics Backend': 'OpenGL ES',
          'Save File Type': translations['Card Image'],
          'Show FPS Counter': translations['Top Left'],
          'Internal Resolution': 'Auto (1920×1080)',
          'Output Resampling': translations['Bilinear'],
          'Disable Letterboxing': translations['Only During Cutscenes'],
          'Magic Armor Behavior': translations['Double Defense'],
          'Custom Action Bindings': translations['Press a Key or Button...']}
output = Path(sys.argv[1] if len(sys.argv) > 1 else 'build/ui-layout-labels.tsv')
output.parent.mkdir(parents=True, exist_ok=True)
output.write_text('\n'.join(translations[key] + '\t' + values.get(key, translations['Off']) for key in keys), encoding='utf-8')
print(len(keys), 'settings scenarios generated from actual source translations')
