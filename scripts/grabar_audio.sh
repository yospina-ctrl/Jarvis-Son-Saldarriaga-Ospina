#!/bin/bash
# Grabar directamente en la carpeta de música del teléfono (visible en el administrador de archivos)
termux-microphone-record -f ~/storage/music/raw.wav -l 8

# Usar ffmpeg para normalizar el formato
ffmpeg -y -loglevel error -i ~/storage/music/raw.wav -ar 16000 -ac 1 ~/storage/music/audio_normalizado.wav

# Eliminar el archivo original sin procesar (opcional)
rm ~/storage/music/raw.wav