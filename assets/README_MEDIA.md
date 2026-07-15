# Media Asset Slots

The game uses these media files:

- `assets/bgm/taffy_battle.mp3` - combat music (`sound/肥圣帝君.mp3`)
- `assets/bgm/taffy_menu.mp3` - looping menu music (`sound/开头BGM.mp3`)
- `assets/bgm/taffy_victory_laugh.mp3` - victory result audio (`sound/唐笑(1).mp3`)
- `assets/bgm/taffy_defeat.mp3` - defeat/endless result audio (`sound/失败音效.mp3`)

The combat loop tries `taffy_battle.*` first. If it is missing, it falls back to the older `taffy_yun_genshin.*` slot and then `assets/bgm/combat.ogg`.

- `assets/bgm/taffy_victory_laugh.m4a`
- `assets/bgm/taffy_victory_laugh.mp3`
- `assets/bgm/taffy_victory_laugh.ogg`
- `assets/bgm/taffy_victory_laugh.wav`

The normal-mode EX/victory result screen plays the first existing victory audio file. Other normal results and endless-mode game over use `taffy_defeat.*`.

Bundled generated assets:

- `assets/ui/taffy_tangxiao.gif`
- `assets/ui/taffy_defeat_bg.png` - defeat/endless result background
- `assets/combat_bg.png` - Taffy-themed combat background
