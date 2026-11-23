# Máquina de Estados - Jarvis-son V1

El flujo secuencial por turnos está diseñado para evitar el consumo excesivo de RAM. Cada etapa termina antes de iniciar la siguiente.

```mermaid
stateDiagram-v2
    [*] --> IDLE
    IDLE --> RECORDING: Pulsar ENTER
    RECORDING --> TRANSCRIBING: Fin de audio (Termux)
    TRANSCRIBING --> GENERATING: Texto generado (Whisper)
    GENERATING --> SPEAKING: Respuesta lista (Qwen)
    SPEAKING --> IDLE: Fin de reproducción (Piper)
    
    RECORDING --> ERROR: Fallo Micrófono
    TRANSCRIBING --> ERROR: Fallo STT
    GENERATING --> ERROR: Fallo SLM (OOM)
    SPEAKING --> ERROR: Fallo TTS
    ERROR --> IDLE: Reintentar