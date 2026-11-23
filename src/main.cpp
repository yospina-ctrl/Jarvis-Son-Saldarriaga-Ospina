#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>

// Definición de los estados
enum class State {
    IDLE,          // Listo
    RECORDING,     // Grabando
    TRANSCRIBING,  // Transcribiendo
    GENERATING,    // Generando
    SPEAKING,      // Hablando
    ERROR
};

class JarvisOrchestrator {
private:
    State current_state;
    // Nombres de archivos fijos y validados por seguridad
    const std::string AUDIO_FILE = "temp_audio.wav";
    const std::string TEXT_STT_FILE = "temp_stt.txt";
    const std::string TEXT_SLM_FILE = "temp_slm.txt";
    const std::string AUDIO_TTS_FILE = "temp_tts.wav";

    // Función auxiliar para ejecutar comandos de forma segura
    bool executeCommand(const std::string& cmd) {
        int result = std::system(cmd.c_str());
        return result == 0;
    }

    // Función para limpiar archivos temporales
    void cleanup() {
        std::remove(AUDIO_FILE.c_str());
        std::remove(TEXT_STT_FILE.c_str());
        std::remove(TEXT_SLM_FILE.c_str());
        std::remove(AUDIO_TTS_FILE.c_str());
    }

    void logTime(const std::string& phase, std::chrono::steady_clock::time_point start) {
        auto end = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        std::cout << "[METRICA] " << phase << " tomo: " << duration << " ms\n";
    }

public:
    JarvisOrchestrator() : current_state(State::IDLE) {}

    void run() {
        std::cout << "=== Iniciando Jarvis-son V1 ===\n";
        
        while (true) {
            current_state = State::IDLE;
            std::cout << "\n[Estado: IDLE] Presiona ENTER para hablar (o 'q' para salir)...";
            std::string input;
            std::getline(std::cin, input);
            if (input == "q") break;

            // ---------------------------------------------------------
            // 1. GRABANDO (Etapa 2)
            // ---------------------------------------------------------
            current_state = State::RECORDING;
            std::cout << "[Estado: RECORDING] Grabando...\n";
            auto start_time = std::chrono::steady_clock::now();
            
            // NOTA: Ajusta la ruta a tu script real de Termux
            if (!executeCommand("./scripts/grabar_audio.sh " + AUDIO_FILE)) {
                current_state = State::ERROR;
                std::cerr << "Error en grabación.\n";
                continue;
            }
            logTime("Grabacion", start_time);

            // ---------------------------------------------------------
            // 2. TRANSCRIBIENDO (Etapa 3)
            // ---------------------------------------------------------
            current_state = State::TRANSCRIBING;
            std::cout << "[Estado: TRANSCRIBING] Convirtiendo voz a texto...\n";
            start_time = std::chrono::steady_clock::now();
            
            // Comando de Whisper (redirige la salida al archivo txt)
            std::string stt_cmd = "./whisper.cpp/build/bin/whisper-cli -m ./models/ggml-tiny.bin -f " 
                                  + AUDIO_FILE + " -l es -otxt -of temp_stt > /dev/null 2>&1";
            
            if (!executeCommand(stt_cmd)) {
                current_state = State::ERROR;
                std::cerr << "Error en STT.\n";
                cleanup();
                continue;
            }
            logTime("Transcripcion", start_time);

            // ---------------------------------------------------------
            // 3. GENERANDO (Etapa 4)
            // ---------------------------------------------------------
            current_state = State::GENERATING;
            std::cout << "[Estado: GENERATING] Pensando respuesta...\n";
            start_time = std::chrono::steady_clock::now();

            // Leemos el texto transcrito (Seguridad: no lo concatenamos al comando)
            // Usamos llama.cpp pasando el prompt por archivo o argumento controlado
            // Para simplificar en esta demo, usamos un prompt fijo con el archivo de texto
            std::string slm_cmd = "./llama.cpp/build/bin/llama-cli -m ./models/qwen3.5-0.8b-q4_0.gguf "
                                  "-c 1024 -n 96 -t 6 -f " + TEXT_STT_FILE + " -o " + TEXT_SLM_FILE + " > /dev/null 2>&1";
            
            if (!executeCommand(slm_cmd)) {
                current_state = State::ERROR;
                std::cerr << "Error en SLM.\n";
                cleanup();
                continue;
            }
            logTime("Generacion", start_time);

            // ---------------------------------------------------------
            // 4. HABLANDO (Etapa 5)
            // ---------------------------------------------------------
            current_state = State::SPEAKING;
            std::cout << "[Estado: SPEAKING] Sintetizando voz...\n";
            start_time = std::chrono::steady_clock::now();

            // Comando de Piper (lee el texto y genera audio)
            // (Ajusta esto al binario real que uses en Termux)
            std::string tts_cmd = "cat " + TEXT_SLM_FILE + " | ./piper -m ./models/es_ES-voz.onnx -f " + AUDIO_TTS_FILE;
            
            if (!executeCommand(tts_cmd)) {
                current_state = State::ERROR;
                std::cerr << "Error en TTS.\n";
                cleanup();
                continue;
            }
            
            // Reproducir el audio (En Termux: termux-media-player play archivo.wav)
            executeCommand("termux-media-player play " + AUDIO_TTS_FILE);
            logTime("TTS", start_time);

            // Limpieza final de la iteración
            cleanup();
            std::cout << "[Estado: IDLE] Turno completado.\n";
        }
    }
};

int main() {
    JarvisOrchestrator jarvis;
    jarvis.run();
    return 0;
}