# Compilación y prueba de orquestador

## Clonar repositorio de github de proyecto Jarvis-Son para el main.cpp y cmake
![alt text](image-27.png)

## Compilar y probar orquestador
### 1. Configurar (esto creará la carpeta build)
cmake -B build

### 2. Compilar
cmake --build build
![alt text](<Captura de pantalla 2026-11-22 225609.png>)

### 3. Dar permisos para permitir la captura de audio
chmod +x scripts/grabar_audio.sh
![alt text](image-28.png)

### 4. Ejecutar
./build/jarvis_orchestrator
![alt text](<WhatsApp Image 2026-11-23 at 8.51.25 AM.jpeg>)








