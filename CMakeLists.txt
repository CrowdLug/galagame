cmake_minimum_required(VERSION 3.21)

project(NewGame LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Multimedia)

add_executable(NewGame
    src/main.cpp
    src/MainWindow.cpp
    include/MainWindow.h
    src/GameManager.cpp
    include/GameManager.h
    src/SceneManager.cpp
    include/SceneManager.h
    src/TransitionController.cpp
    include/TransitionController.h
    src/CombatLoop.cpp
    include/CombatLoop.h
    src/RunContext.cpp
    include/RunContext.h
    src/ResourceManager.cpp
    include/ResourceManager.h
    src/PlayerEntity.cpp
    include/PlayerEntity.h
    src/WeaponSystem.cpp
    include/WeaponSystem.h
    src/CombatScene.cpp
    include/CombatScene.h
    src/LightDirector.cpp
    include/LightDirector.h
    src/EnemyDirector.cpp
    include/EnemyDirector.h
    src/EnemyEntity.cpp
    include/EnemyEntity.h
    src/FeedbackSystem.cpp
    include/FeedbackSystem.h
    src/CollisionManager.cpp
    include/CollisionManager.h
    src/BulletItem.cpp
    include/BulletItem.h
    src/EnemyBulletItem.cpp
    include/EnemyBulletItem.h
    src/PickupItem.cpp
    include/PickupItem.h
    src/TrapItem.cpp
    include/TrapItem.h
)

target_include_directories(NewGame PRIVATE include)

target_link_libraries(NewGame
    PRIVATE
        Qt6::Widgets
        Qt6::Multimedia
)
