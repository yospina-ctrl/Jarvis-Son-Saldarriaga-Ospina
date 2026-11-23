#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <unistd.h>
#include <sys/wait.h>

enum class State {
    IDLE,
    RECORDING,
    TRANSCRIBING,
    GENERATING,
    SPEAKING,
    ERROR
};

class JarvisOrchestrator {
private:
    State current_state;

    // Rutas absolutas (Termux)
    const std::string HOME_DIR    = "/data/data/com.termux/files/home";
    const std::string PROJECT_DIR = HOME_DIR + "/Jarvis-Son-Saldarriaga-Ospina";
    const std::string TMP_DIR     = PROJECT_DIR + "/tmp";

    const std::string AUDIO_RAW  = HOME_DIR + "/storage/music/raw.wav";
    const std::string AUDIO_FILE = HOME_DIR + "/storage/music/audio_normalizado.wav";

    // Archivos temporales con ruta absoluta (no dependen del directorio de ejecucion)
    const std::string STT_PREFIX   = TMP_DIR + "/temp_stt";
    const std::string TEXT_STT_FILE = TMP_DIR + "/temp_stt.txt";
    const std::string TEXT_SLM_FILE = TMP_DIR + "/temp_slm.txt";
    const std::string PROMPT_FILE   = TMP_DIR + "/prompt.txt";
    const std::string SLM_LOG_FILE  = TMP_DIR + "/slm_log.txt";
    const std::string TTS_INPUT     = TMP_DIR + "/tts_input.txt";

    const std::string LLAMA_BIN = HOME_DIR + "/llama.cpp/build/bin/";
    const std::string MODELO    = HOME_DIR + "/storage/dcim/models/Qwen3.5-2B-Q4_K_M.gguf";

    bool executeCommand(const std::string& cmd) {
        int result = std::system(cmd.c_str());
        return result == 0;
    }

    bool existe(const std::string& ruta) {
        return access(ruta.c_str(), X_OK) == 0;
    }

    std::string leerArchivo(const std::string& ruta) {
        std::ifstream f(ruta);
        std::stringstream b;
        b << f.rdbuf();
        return b.str();
    }

    void escribirArchivo(const std::string& ruta, const std::string& contenido) {
        std::ofstream f(ruta, std::ios::trunc);
        f << contenido;
    }

    void cleanup() {
        std::remove(TEXT_STT_FILE.c_str());
        std::remove(TEXT_SLM_FILE.c_str());
        std::remove(PROMPT_FILE.c_str());
        std::remove(TTS_INPUT.c_str());
        // slm_log.txt se conserva a proposito para depurar
    }

    void logTime(const std::string& phase, std::chrono::steady_clock::time_point start) {
        auto end = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        std::cout << "[METRICA] " << phase << " tomo: " << duration << " ms\n";
    }

    // Lee la salida de whisper y quita timestamps
    std::string leerYLimpiarSTT() {
        std::ifstream file(TEXT_STT_FILE);
        std::string linea, texto_limpio;

        if (file.is_open()) {
            std::regex timestamp_regex("\\[.*?\\]");
            std::regex espacios_regex("^\\s+|\\s+$");
            while (std::getline(file, linea)) {
                linea = std::regex_replace(linea, timestamp_regex, "");
                linea = std::regex_replace(linea, espacios_regex, "");
                if (!linea.empty()) {
                    texto_limpio += linea + " ";
                }
            }
            file.close();
        }
        return texto_limpio;
    }

    // Deja solo la respuesta del modelo: quita banner, estadisticas, eco y marcas
    std::string limpiarSLM(const std::string& crudo) {
        std::stringstream ss(crudo);
        std::string linea, out;

        while (std::getline(ss, linea)) {
            size_t i = linea.find_first_not_of(" \t\r");
            if (i == std::string::npos) continue;
            char c = linea[i];
            if (c == '[' || c == '>') continue;                          // "[ Prompt: ...]", eco "> ..."
            if (linea.find("t/s") != std::string::npos) continue;        // estadisticas
            if (linea.find("Exiting") != std::string::npos) continue;
            if (linea.find("llama_") != std::string::npos) continue;
            if (linea.find("build:") != std::string::npos) continue;
            out += linea + " ";
        }

        for (const char* m : {"[end of text]", "<|im_end|>", "<|endoftext|>", "<think>", "</think>"}) {
            size_t p;
            while ((p = out.find(m)) != std::string::npos) {
                out.erase(p, std::string(m).size());
            }
        }

        out.erase(std::remove_if(out.begin(), out.end(),
                  [](char c) { return c == '*' || c == '#' || c == '`' || c == '_' || c == '"'; }),
                  out.end());
        return out;
    }

public:
    JarvisOrchestrator() : current_state(State::IDLE) {
        executeCommand("mkdir -p " + TMP_DIR);
    }

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

            executeCommand("rm -f " + AUDIO_RAW);
            executeCommand("chmod 755 " + PROJECT_DIR + "/scripts/grabar_audio.sh");

            if (!executeCommand(PROJECT_DIR + "/scripts/grabar_audio.sh")) {
                std::cerr << "Error en grabación.\n";
                continue;
            }
            logTime("Grabacion", start_time);

            // ---------------------------------------------------------
            // 2. TRANSCRIBIENDO
            // ---------------------------------------------------------
            current_state = State::TRANSCRIBING;
            std::cout << "[Estado: TRANSCRIBING] Convirtiendo voz a texto...\n";
            start_time = std::chrono::steady_clock::now();

            executeCommand("rm -f " + TEXT_STT_FILE);

            std::string stt_cmd = HOME_DIR + "/whisper.cpp/build/bin/whisper-cli "
                                  "-m " + HOME_DIR + "/whisper.cpp/models/ggml-base.bin "
                                  "-f " + AUDIO_FILE + " -l es -otxt -of " + STT_PREFIX +
                                  " > /dev/null 2>&1";

            if (!executeCommand(stt_cmd)) {
                std::cerr << "Error en STT.\n";
                continue;
            }
            logTime("Transcripcion", start_time);

            // ---------------------------------------------------------
            // 3. GENERANDO
            // ---------------------------------------------------------
            current_state = State::GENERATING;
            std::cout << "[Estado: GENERATING] Pensando respuesta...\n";
            start_time = std::chrono::steady_clock::now();

            std::string prompt_usuario = leerYLimpiarSTT();
            if (prompt_usuario.empty()) {
                std::cerr << "No se reconoció texto. Saltando turno.\n";
                continue;
            }
            std::cout << "   -> Tú dijiste: " << prompt_usuario << std::endl;

            executeCommand("rm -f " + TEXT_SLM_FILE);

            std::string slm_cmd;
            if (existe(LLAMA_BIN + "llama-completion")) {
                // Modo no interactivo: plantilla ChatML escrita a mano
                escribirArchivo(PROMPT_FILE,
                    "<|im_start|>user\n" + prompt_usuario + "<|im_end|>\n"
                    "<|im_start|>assistant\n<think>\n\n</think>\n\n");
                slm_cmd = LLAMA_BIN + "llama-completion -m " + MODELO +
                          " -c 1024 -n 150 -t 6 -no-cnv --no-display-prompt -f " + PROMPT_FILE +
                          " > " + TEXT_SLM_FILE + " 2> " + SLM_LOG_FILE + " < /dev/null";
            } else {
                // El prompt va por archivo: las comillas de Whisper no rompen el comando
                escribirArchivo(PROMPT_FILE, prompt_usuario);
                slm_cmd = LLAMA_BIN + "llama-cli -m " + MODELO +
                          " -c 1024 -n 150 -t 6 -st"
                          " --chat-template-kwargs '{\"enable_thinking\": false}'"
                          " -p \"$(cat " + PROMPT_FILE + ")\""
                          " > " + TEXT_SLM_FILE + " 2> " + SLM_LOG_FILE + " < /dev/null";
            }

            int rc = std::system(slm_cmd.c_str());
            std::cout << "   [debug] llama codigo=" << WEXITSTATUS(rc) << "\n";
            logTime("Generacion", start_time);

            // ---------------------------------------------------------
            // 4. HABLANDO
            // ---------------------------------------------------------
            current_state = State::SPEAKING;
            std::cout << "[Estado: SPEAKING] Sintetizando voz...\n";
            start_time = std::chrono::steady_clock::now();

            std::string crudo = leerArchivo(TEXT_SLM_FILE);
            std::string respuesta_slm = limpiarSLM(crudo);

            std::cout << "   [debug] bytes crudos=" << crudo.size()
                      << ", bytes limpios=" << respuesta_slm.size() << "\n";
            std::cout << "   -> Jarvis: " << respuesta_slm << std::endl;

            if (respuesta_slm.find_first_not_of(' ') == std::string::npos) {
                std::cerr << "[AVISO] Respuesta vacia. Ultimas lineas del log:\n";
                executeCommand("tail -5 " + SLM_LOG_FILE);
                respuesta_slm = "Lo siento, no pude generar una respuesta.";
            }

            // El texto entra por stdin: no pasa por el shell, no hay que escapar nada
            escribirArchivo(TTS_INPUT, respuesta_slm);
            int rc_tts = std::system(("termux-tts-speak -l es -s MUSIC < " + TTS_INPUT).c_str());
            if (rc_tts != 0) {
                std::cerr << "Error en TTS (codigo " << WEXITSTATUS(rc_tts) << ").\n";
            }
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