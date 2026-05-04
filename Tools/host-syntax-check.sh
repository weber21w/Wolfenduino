#!/bin/sh
set -eu

CC=${HOST_CC:-${CC:-gcc}}
CFLAGS="-std=gnu99 -Wall -Wextra -Wno-unused-parameter -DHOST_BUILD=1 -I./Game -I./DataHeaders"

for src in \
	Game/Actor.c \
	Game/Engine.c \
	Game/FixedMath.c \
	Game/Map.c \
	Game/Menu.c \
	Game/Data_GUI.c \
	Game/Platform.c \
	Game/Player.c \
	Game/Renderer.c \
	Game/WolfenduinoUzebox.c
 do
	echo "syntax: $src"
	$CC $CFLAGS -fsyntax-only "$src"
done
