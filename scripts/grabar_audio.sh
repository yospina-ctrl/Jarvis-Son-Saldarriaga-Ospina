#!/bin/bash

# 1. Eliminar archivos anteriores
rm -f ~/storage/music/raw.wav
rm -f ~/storage/music/audio_normalizado.wav

# 2. Iniciar grabación
termux-microphone-record -f ~/storage/music/raw.wav

# 3. Esperar 5 segundos (habla aquí)
sleep 5

# 4. Detener grabación de forma segura
termux-microphone-record -q

# 5. Pausa para que el sistema cierre el archivo
sleep 1

# 6. Normalizar
ffmpeg -y -loglevel error -i ~/storage/music/raw.wav -ar 16000 -ac 1 ~/storage/music/audio_normalizado.wav

# 7. Limpiar
rm -f ~/storage/music/raw.wav