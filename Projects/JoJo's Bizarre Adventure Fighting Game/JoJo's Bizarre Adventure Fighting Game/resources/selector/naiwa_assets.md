# 奶娃选人素材

生成方式：内置 imagegen。所有素材均为带透明通道的 PNG，已按游戏资源尺寸导出；图像尺寸与低透明度边缘清理由确定性导出脚本处理。

|文件|尺寸|用途|
|---|---|---|
|avatar_naiwa.png|84×84|奶娃头像，仅在游戏场景状态栏显示；选角界面不显示此头像|
|1P_naiwa_desc.png|2032×774|A/D 移动，W 跳跃，S 闪现，F 幻影，E 庇护，Q 大招|
|2P_naiwa_desc.png|1983×793|左右键移动，上键跳跃，下键闪现，逗号幻影，句号庇护，斜杠大招|
|naiwa_selector_background.png|560×720|奶娃羽帽头像与豆包伞的透明像素底纹|

生成原图保存于 outputs/naiwa-selector，最终游戏素材保存于本目录。

素材已通过 main.cpp 加载，在 selector_scene.h 中按双方各自的角色选择显示。奶娃头像同时用于选角框和战斗血条。按键图加载为 440×120，背景加载为 560×900，背景以 128/255 不透明度绘制并左右镜像滚动。

## 提示词记录

### avatar_naiwa.png

Use case: stylized-concept. Asset type: pixel-art character selection avatar PNG for an existing 2D fighting game. Input image 1 is the existing Jotaro 84x84 framed portrait, a style/layout reference. Input image 2 is Naiwa's approved sprite, the authoritative identity reference. Create ONLY Naiwa's head-and-upper-shoulders portrait, facing slightly right, cheerful confident expression, large yellow-green face, one bright teal green visible eye, dark navy tricorn-like hat with ornate gold trim and large white curled feather, white cravat/white shirt and dark blue gold cape visible at the bottom. Preserve the exact creature identity of input image 2: no human face, no new hair or different clothing. Square close-up fills the frame with face and hat, feather fully within the frame. Match the first reference's retro pixel sprite style and simple thin gold frame with chamfered corners on a dark navy inner portrait backdrop. Transparent pixels outside the chamfered frame. Native pixel-art aesthetic designed to remain clearly recognizable when reduced to 84x84 pixels, use chunky crisp pixels, limited colors, no blur, no typography, no extra icons, no watermark. Output a single square avatar.

### 1P_naiwa_desc.png

Use case: text-localization. Asset type: 1P Naiwa-specific transparent PNG keyboard legend for a retro 2D fighting game's character selection screen. Input image is the existing game's 1P keyboard guide: use it as exact graphic style and layout reference. Make a new matching TWO-ROW guide with silver gray beveled pixel keycaps, dark navy letter glyphs, large warm ivory Chinese pixel lettering with dark navy heavy outlines. Wide landscape canvas, aspect approximately 2032:774. All background must be truly transparent. Top row has exactly THREE groups from left to right: [A key] [D key] 移动 ; [W key] 跳跃 ; [S key] 闪现. Bottom row exactly THREE groups: [F key] 幻影 ; [E key] 庇护 ; [Q key] 大招. Exact Chinese labels must be rendered as specified: 移动, 跳跃, 闪现, 幻影, 庇护, 大招. Use only these six labels and the six named letter keycaps (plus D, seven keys total). The S action is teleportation, so its label MUST be 闪现, never 防御. F summons a phantom, so label 幻影. E summons Doubao's protective umbrella, so label 庇护. Maintain clean uniform spacing across the two rows, keycap shape consistency and chunky crisp pixel edges; do not add title, character illustration, extra explanatory lines, frame or watermark. Keep margins modest and fill the canvas enough to read when scaled down to 440x120 in game.

### 2P_naiwa_desc.png

Use case: text-localization. Asset type: 2P Naiwa-specific transparent PNG keyboard legend for an existing pixel-art fighting game. Input image 1 is the existing 2P legend and establishes correct arrow and punctuation key shapes. Input image 2 is the approved Naiwa 1P legend and establishes exact new labels and style. Create the matching 2P version ONLY. TWO ROWS and THREE GROUPS per row. Wide landscape canvas, aspect about 1983:793, genuinely transparent background. Top row left-to-right exactly: [LEFT ARROW key] [RIGHT ARROW key] 移动 ; [UP ARROW key] 跳跃 ; [DOWN ARROW key] 闪现. Bottom row exactly: [COMMA ',' key] 幻影 ; [PERIOD '.' key] 庇护 ; [FORWARD SLASH '/' key] 大招. Chinese exact text: 移动 跳跃 闪现 幻影 庇护 大招. Down arrow MUST mean 闪现 not 防御. Do not replace ',' '.' '/' with letters or other punctuation. Match silver-gray beveled chunky pixel keycaps and warm ivory Chinese pixel glyphs with heavy dark navy outlines from the references. Match two-row uniform spacing, no title, no character art, no additional legends or decorative border, no watermark. Keep all content readable when scaled to 440x120.

### naiwa_selector_background.png

Use case: stylized-concept. Asset type: transparent repeat-pattern PNG background overlay named naiwa_selector_background.png for an existing pixel-art fighting game's selection menu. Input image 1 (Jotaro) and image 2 (Kakyoin) are exact layout and technique references: sparse evenly spaced staggered rows of simple white pixel silhouettes on a truly transparent canvas. Input image 3 is Naiwa's identity reference. Create the same kind of restrained menu overlay for Naiwa, target composition 560x720 vertical, horizontally seamless scrolling. Five staggered rows, approximately four emblems across each row with alternate rows offset by half a column. Primary repeated emblem is Naiwa's distinctive plumed navy tricorn hat combined with a round cheerful phantom head silhouette, stylized as a tiny pure-white 8-bit emblem around 65px wide at final size. Alternate a few emblems with a small simple umbrella symbol representing Doubao's protection. All emblems solid white with only transparent cutouts for eyes, hat brim detail or canopy separations; NO colored elements, no shading, no gray, no gradients, no full-size portrait, no title or text, no wallpaper backdrop, no opaque black pixels. All remaining canvas must have zero alpha. Match reference icon scale, spacing and sparse density. Keep the top/bottom comfortable margins. Along the left and right edges, paired half-icons must line up for clean horizontal tiling, as in the reference. Crisp chunky pixel edges, production game texture, not a mockup.

### naiwa_selector_background.png (final cleanup)

Use case: precise-object-edit. Edit target input image 1 is a menu pattern that has unwanted grunge noise. Style reference input image 2 is the clean existing Jotaro pattern. REBUILD the target as a CLEAN SIMPLE sparse pixel pattern matching reference 2. There must be ONLY twenty small flat pure-white icons, arranged as 5 rows by 4 columns, evenly spaced in staggered rows. Alternate a tiny smiling round head wearing a feather hat, and a tiny umbrella silhouette. Each icon consists of contiguous solid opaque white pixels with transparent interior cutouts only. Remove ALL grunge, ALL texture, ALL speckles, ALL glow, ALL shadows, ALL connecting marks, ALL noisy edge outlines. Every pixel between the small icons must be completely transparent, alpha zero, just like reference 2. Flat 1-bit sprite graphics ONLY. Icon bodies around 60x60 pixels in a final 560x720 canvas, keep icons well apart with large clean transparent gaps. No line drawing, no text, no background of any kind. Crisp stair-step square pixel edges. This is a clean game UI mask texture, not an illustration.
