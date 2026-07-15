# NewGame - 拯救塔菲喵

[English Version](README_EN.md) | GitHub Repository: `https://github.com/CrowdLug/galagame`

一款基于 **C++20 + Qt 6 Widgets + CMake** 开发的 2D 俯视角射击生存游戏。玩家需要在限定场景中控制角色移动、射击敌人、躲避陷阱，并尽可能拯救塔菲喵，或在无尽模式中坚持更久。

## 项目简介

本项目采用 Qt Graphics View Framework 实现游戏场景、角色、敌人、子弹、掉落物和陷阱等对象的绘制与交互。游戏逻辑被拆分为场景管理、战斗循环、敌人生成、碰撞检测、资源加载、反馈效果等模块，便于后续扩展关卡、武器和敌人类型。

## 核心玩法

- **普通模式**：玩家进入固定关卡，在 60 秒内对抗分批刷新的敌人，战斗结束后进入结算页面。
- **无尽模式**：敌人按波次持续生成，随着波次推进敌人压力逐步提升，目标是尽可能长时间生存并获得更高击杀数。
- **实时射击战斗**：玩家使用鼠标左键向目标方向射击，子弹与敌人发生碰撞后造成伤害。
- **敌人 AI**：包含近战敌人和远程敌人两类。近战敌人会追击玩家，远程敌人会保持距离并发射子弹。
- **拾取与恢复**：击败敌人后有机会生成回血道具，玩家碰到道具后恢复生命值。
- **陷阱机制**：战斗过程中会周期性生成陷阱，玩家触碰后受到伤害并产生控制效果。
- **战斗反馈**：包含受伤闪烁、击杀反馈、屏幕震动、音效/背景音乐等效果，增强游戏打击感。
- **数据驱动配置**：武器、敌人和关卡刷新节奏使用 JSON 文件配置，方便调参和扩展。

## 操作说明

| 操作 | 按键 / 鼠标 |
| --- | --- |
| 角色移动 | `W` `A` `S` `D` |
| 射击 | 鼠标左键单击 / 按住连续射击 |
| 步枪 | `1` |
| 手枪 | `2` |
| 霰弹枪 | `3` |
| 开关屏幕震动 | `V` |
| 暂停 / 恢复敌人刷新 | `P` |
| 菜单选择 | 鼠标点击 |

### 武器配置

| 槽位 | 武器 | 特点 | 配置文件 |
| --- | --- | --- | --- |
| `1` | Rifle | 高射速、稳定输出、单发消耗 1 点弹药 | `data/weapons/rifle.json` |
| `2` | Pistol | 较高单发伤害、精度高、射速较低 | `data/weapons/pistol.json` |
| `3` | Shotgun | 一次发射 7 颗弹丸、近距离爆发、单发消耗 3 点弹药 | `data/weapons/shotgun.json` |

三种武器共享当前弹药池。射速、伤害、弹速、散布、后坐力、弹药消耗和弹丸数量均由 JSON 配置控制。

## 技术栈

- **开发语言**：C++20
- **图形界面**：Qt 6 Widgets
- **多媒体支持**：Qt 6 Multimedia
- **构建系统**：CMake 3.21+
- **推荐编译器**：MinGW 64-bit 或 MSVC 2022 x64
- **推荐开发工具**：Qt Creator / Visual Studio Code + CMake Tools

## 运行环境要求

### 基本环境

- Windows 10 / Windows 11
- Qt 6.x，推荐使用项目脚本中配置的 `C:\Qt\6.11.0\mingw_64`
- CMake 3.21 或更高版本
- MinGW 64-bit，推荐路径为 `C:\mingw64`

### 可选环境

- Visual Studio 2022 或 Build Tools
- Ninja
- Qt Creator

如果本机 Qt 或 MinGW 安装路径不同，需要同步修改：

- `CMakePresets.json`
- `build_mingw_release.bat`
- `run_game.bat`

## 构建与运行

### 方式一：使用脚本构建

在项目根目录双击或运行：

```bat
build_mingw_release.bat
```

构建成功后运行：

```bat
run_game.bat
```

### 方式二：使用 CMake 命令

```bat
cmake --preset mingw-release
cmake --build --preset mingw-release -j 2
```

构建输出位置：

```text
build/mingw-release/NewGame.exe
```

如需打包 Qt 运行库，可使用：

```bat
windeployqt6 --release --compiler-runtime --dir build/mingw-release build/mingw-release/NewGame.exe
```

## 项目目录结构

```text
newgame/
├── assets/                  # 图片、背景、音乐等资源
│   ├── bgm/                 # 背景音乐
│   └── ui/                  # 菜单界面资源
├── data/                    # JSON 配置数据
│   ├── enemies/             # 敌人参数
│   ├── stages/              # 关卡刷新配置
│   └── weapons/             # 武器参数
├── include/                 # 头文件
├── src/                     # 源代码
├── build_mingw_release.bat  # Release 构建脚本
├── run_game.bat             # 游戏启动脚本
├── CMakeLists.txt           # CMake 项目配置
└── CMakePresets.json        # CMake 预设配置
```

## 主要模块说明

- `GameManager`：负责游戏状态流转，包括菜单、战斗和结算页面。
- `SceneManager`：负责创建和切换不同的 Qt 图形场景。
- `CombatScene`：负责接收键鼠输入、刷新 HUD、管理战斗场景显示。
- `CombatLoop`：负责战斗主循环，包括敌人生成、子弹更新、陷阱更新和胜负判定。
- `CollisionManager`：负责玩家、敌人、子弹、掉落物和陷阱之间的碰撞检测。
- `EnemyDirector` / `LightDirector`：负责敌人刷新节奏和战斗强度调度。
- `WeaponSystem`：负责武器开火逻辑、子弹生成和武器参数读取。
- `ResourceManager`：负责图片、音频等资源加载。

## 数据配置示例

敌人、武器和关卡数据均可通过 JSON 调整。例如：

- `data/weapons/rifle.json`：步枪配置，高射速稳定输出。
- `data/weapons/pistol.json`：手枪配置，高精度和较高单发伤害。
- `data/weapons/shotgun.json`：霰弹枪配置，多弹丸和大范围散布。
- `data/enemies/melee.json`：配置近战敌人的移动速度、攻击范围和攻击伤害。
- `data/enemies/ranged.json`：配置远程敌人的射程、偏好距离和攻击间隔。
- `data/stages/stage01.json`：配置不同时间点生成的近战/远程敌人数量。

这种设计让游戏参数和代码逻辑分离，便于平衡性调整和后续内容扩展。

## 项目特点

- 使用 Qt Graphics View 实现 2D 游戏对象管理。
- 基于 `QTimer` 的实时战斗循环，默认约 60 FPS 刷新。
- 采用模块化设计，降低场景、实体、碰撞和资源管理之间的耦合。
- 使用 JSON 作为外部配置，提升项目可维护性。
- 包含普通模式和无尽模式，具有较完整的开始、战斗、结算流程。
- 支持背景音乐、屏幕震动、受击闪烁等反馈效果。

## 后续可扩展方向

- 增加激光武器或范围攻击武器，并加入独立弹药与换弹机制。
- 增加 Boss 关卡和特殊敌人。
- 增加升级选择界面，使无尽模式具有更强成长性。
- 增加存档系统和最高分记录。
- 优化 UI 字体编码与多语言显示。
- 增加单元测试或自动化冒烟测试，提升项目稳定性。

## 说明

本项目用于课程学习和游戏开发实践，重点展示 C++ 面向对象设计、Qt 图形界面开发、CMake 工程管理、JSON 数据驱动配置以及基本 2D 游戏循环实现。
