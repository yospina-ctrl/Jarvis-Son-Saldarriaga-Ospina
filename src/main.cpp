#include <iostream>
#include <string>

// Definición de los estados del asistente Jarvis-son V1
enum class AssistantState
{
    IDLE,
    LISTENING,
    TRANSCRIBING,
    GENERATING,
    SPEAKING,
    ERROR
};

class Orchestrator
{
private:
    AssistantState current_state;

public:
    Orchestrator() : current_state(AssistantState::IDLE) {}

    void runCycle()
    {
        // Simulación del ciclo de estados del proyecto
        set_state(AssistantState::LISTENING);
        // Aquí se integrará la captura de audio (Estudiante A)

        set_state(AssistantState::TRANSCRIBING);
        // Aquí se integrará Whisper.cpp (Estudiante A)

        set_state(AssistantState::GENERATING);
        // Aquí entra tu módulo SLM (llama.cpp / Qwen 2B)

        set_state(AssistantState::SPEAKING);
        // Aquí se integrará Piper TTS

        set_state(AssistantState::IDLE);
    }

    void set_state(AssistantState new_state)
    {
        current_state = new_state;
        switch (current_state)
        {
        case AssistantState::IDLE:
            std::cout << "[Jarvis-son] Estado: Esperando activación (IDLE)..."
                      << std::endl;
            break;
        case AssistantState::LISTENING:
            std::cout << "[Jarvis-son] Estado: Escuchando entorno..." << std::endl;
            break;
        case AssistantState::TRANSCRIBING:
            std::cout
                << "[Jarvis-son] Estado: Transcribiendo audio a texto (STT)..."
                << std::endl;
            break;
        case AssistantState::GENERATING:
            std::cout
                << "[Jarvis-son] Estado: Generando respuesta con SLM (Qwen)..."
                << std::endl;
            break;
        case AssistantState::SPEAKING:
            std::cout
                << "[Jarvis-son] Estado: Sintetizando y reproduciendo voz (TTS)..."
                << std::endl;
            break;
        case AssistantState::ERROR:
            std::cerr
                << "[Jarvis-son] ERROR: Se ha producido una falla en el flujo."
                << std::endl;
            break;
        }
    }
};

int main()
{
    std::cout << "=== Inicializando Jarvis-son V1 (Exynos 1280) ===" << std::endl;
    Orchestrator jarvis;
    jarvis.runCycle();
    return 0;
}