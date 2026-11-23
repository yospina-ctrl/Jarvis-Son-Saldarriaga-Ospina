#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <sstream>

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
    // Nombres de archivos en el almacenamiento compartido para evitar problemas de permisos
    const std::string AUDIO_RAW = "~/storage/music/raw.wav";
    const std::string AUDIO_FILE = "~/storage/music/audio_normalizado.wav";
    const std::string TEXT_STT_FILE = "temp_stt.txt";
    const std::string TEXT_SLM_FILE = "temp_slm.txt";
    const std::string AUDIO_TTS_FILE = "~/storage/music/temp_tts.wav";

    bool executeCommand(const std::string& cmd) {
        int result = std::system(cmd.c_str());
        return result == 0;
    }

    void cleanup() {
        std::remove(TEXT_STT_FILE.c_str());
        std::remove(TEXT_SLM_FILE.c_str());
    }

    void logTime(const std::string& phase, std::chrono::steady_clock::time_point start) {
        auto end = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        std::cout << "[METRICA] " << phase << " tomo: " << duration << " ms\n";
    }

    // Función para leer el texto del STT y limpiarlo
    std::string leerYLimpiarSTT() {
        // CORRECCIÓN: Whisper genera temp_stt.txt (no .txt.txt)
        std::ifstream file("temp_stt.txt"); 
        std::string linea, texto_limpio;
        if (file.is_open()) {
            while (std::getline(file, linea)) {
                // Eliminar timestamps [00:00:00.000 --> 00:00:00.000]
                size_t pos = linea.find("]");
                if (pos != std::string::npos) {
                    texto_limpio += linea.substr(pos + 1);
                }
            }
            file.close();
        }
        // Limpiar espacios al inicio
        if (!texto_limpio.empty() && texto_limpio[0] == ' ') {
            texto_limpio.erase(0, 1);
        }
        return texto_limpio;
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
            // 1. GRABANDO
            // ---------------------------------------------------------
            current_state = State::RECORDING;
            std::cout << "[Estado: RECORDING] Grabando (5 segundos)...\n";
            auto start_time = std::chrono::steady_clock::now();
            
            // CORRECCIÓN: Borrar el archivo raw anterior para evitar el error de "File already exists"
            executeCommand("rm -f " + AUDIO_RAW);
            
            if (!executeCommand("termux-microphone-record -f " + AUDIO_RAW + " -l 5")) {
                std::cerr << "Error en grabación.\n"; continue;
            }
            executeCommand("ffmpeg -y -loglevel error -i " + AUDIO_RAW + " -ar 16000 -ac 1 " + AUDIO_FILE);
            logTime("Grabacion", start_time);

            // ---------------------------------------------------------
            // 2. TRANSCRIBIENDO
            // ---------------------------------------------------------
            current_state = State::TRANSCRIBING;
            std::cout << "[Estado: TRANSCRIBING] Convirtiendo voz a texto...\n";
            start_time = std::chrono::steady_clock::now();
            
            // CORRECCIÓN: Rutas absolutas para whisper.cpp
            std::string stt_cmd = "~/whisper.cpp/build/bin/whisper-cli -m ~/whisper.cpp/models/ggml-base.bin -f " 
                                  + AUDIO_FILE + " -l es -otxt -of temp_stt > /dev/null 2>&1";
            
            if (!executeCommand(stt_cmd)) {
                std::cerr << "Error en STT.\n"; continue;
            }
            logTime("Transcripcion", start_time);

            // ---------------------------------------------------------
            // 3. GENERANDO
            // ---------------------------------------------------------
            current_state = State::GENERATING;
            std::cout << "[Estado: GENERATING] Pensando respuesta...\n";
            start_time = std::chrono::steady_clock::now();

            // Leer el texto del usuario
            std::string prompt_usuario = leerYLimpiarSTT();
            if(prompt_usuario.empty()) {
                std::cerr << "No se reconoció texto. Saltando turno.\n"; continue;
            }
            std::cout << "   -> Tú dijiste: " << prompt_usuario << std::endl;

            // CORRECCIÓN: Rutas absolutas para llama.cpp
            std::string slm_cmd = "~/llama.cpp/build/bin/llama-cli -m ~/storage/dcim/models/Qwen3.5-2B-Q4_K_M.gguf "
                                  "-c 1024 -n 150 -t 6 -st --chat-template-kwargs '{\"enable_thinking\": false}' "
                                  "-p \"" + prompt_usuario + "\" > " + TEXT_SLM_FILE + " 2>/dev/null";
            
            if (!executeCommand(slm_cmd)) {
                std::cerr << "Error en SLM.\n"; continue;
            }
            logTime("Generacion", start_time);

            // ---------------------------------------------------------
            // 4. HABLANDO
            // ---------------------------------------------------------
            current_state = State::SPEAKING;
            std::cout << "[Estado: SPEAKING] Sintetizando voz...\n";
            start_time = std::chrono::steady_clock::now();

            // Leer la respuesta generada por el SLM
            std::string respuesta_slm;
            std::ifstream file_slm(TEXT_SLM_FILE);
            if (file_slm.is_open()) {
                std::stringstream buffer;
                buffer << file_slm.rdbuf();
                respuesta_slm = buffer.str();
                file_slm.close();
            }

            // CORRECCIÓN: Ruta absoluta para el script TTS
            std::string tts_cmd = "python ~/offline-tts-v1.py "
                                  "--vits-model=~/vits-piper-es_ES-davefx-medium/es_ES-davefx-medium.onnx "
                                  "--vits-tokens=~/vits-piper-es_ES-davefx-medium/tokens.txt "
                                  "--vits-data-dir=~/vits-piper-es_ES-davefx-medium/espeak-ng-data "
                                  "--output-filename=" + AUDIO_TTS_FILE + " "
                                  "\"" + respuesta_slm + "\" > /dev/null 2>&1";
            
            if (!executeCommand(tts_cmd)) {
                std::cerr << "Error en TTS.\n"; continue;
            }
            
            executeCommand("termux-media-player play " + AUDIO_TTS_FILE);
            logTime("TTS", start_time);

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