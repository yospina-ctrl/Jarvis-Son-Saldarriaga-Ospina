# Implementación de Whisper/STT

## Comandos para compilar whisper.cpp
git clone --depth 1 https://github.com/ggerganov/whisper.cpp.git
cd whisper.cpp
cmake -S . -B build -DGGML_NO_OPENMP=ON
cmake --build build -j$(nproc)

![alt text](image-7.png)

cd whisper.cpp
cmake -S . -B build -DGGML_NO_OPENMP=ON

![alt text](image-8.png)


cmake --build build -j$(nproc)
![alt text](image-9.png)
![alt text](image-10.png)

## Descarga del modelo Whisper

bash ./models/download-ggml-model.sh base
![alt text](image-11.png)

## Transcripción del modelo con el audio normalizado con FFmpeg
./build/bin/whisper-cli -m models/ggml-base.bin -f ~/storage/music/audio_normalizado.wav -l es
![alt text](image-12.png)
![alt text](image-13.png)