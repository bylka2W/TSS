# TSS — GPU Sharpening Plugin

Плагин для Unreal Engine 5.3. Глобальный compute-шейдер шарпинга через FSceneViewExtension, без патча движка.

## Команды консоли

| Команда | По умолчанию | Описание |
|---------|-------------|----------|
| `r.TSS.Enabled 1` | `0` | Включить шарпинг |
| `r.TSS.Enabled 0` | — | Выключить шарпинг (passthrough) |
| `r.TSS.Strength <float>` | `1.0` | Сила шарпинга. диапазон 0.0–2.0 |

## Примеры

```bash
# Включить шарпинг со стандартной силой
r.TSS.Enabled 1

# Слабый шарпинг (30%)
r.TSS.Strength 0.3
r.TSS.Enabled 1

# Средний шарпинг (50%)
r.TSS.Strength 0.5
r.TSS.Enabled 1

# Максимальный шарпинг
r.TSS.Strength 2.0
r.TSS.Enabled 1

# Выключить (passthrough без изменений)
r.TSS.Enabled 0
```

## Структура плагина

```
C:\TSS\
├── TSS.uplugin
├── Shaders\TSSSharpen.usf        # HLSL compute shader
├── Source\TSS\
│   ├── TSS.h                     # FTSSSharpenShader (GLOBAL_SHADER)
│   ├── TSS.cpp                   # Модуль, AddSharpenPass
│   ├── TSSViewExtension.h        # FSceneViewExtensionBase
│   └── TSSViewExtension.cpp      # SubscribeToPostProcessingPass → Tonemap
```

## Архитектура

```
Tonemap pass (UE5)
    ↓
PostProcessPassAfterTonemap_RenderThread
    ↓ r.TSS.Enabled=0 → passthrough (AddDrawTexturePass → OverrideOutput)
    ↓ r.TSS.Enabled=1 → AddSharpenPass (compute) → AddDrawTexturePass → OverrideOutput
    ↓
FXAA → Backbuffer → Экран
```


