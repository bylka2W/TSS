# TSS — GPU Sharpening Plugin

Плагин для Unreal Engine 5.3. Глобальный compute-шейдер шарпинга через FSceneViewExtension, без патча движка.

### TSS выключен (`r.TSS.Enabled 0`)
![TSS OFF](docs/strength_0.0.png)

### TSS включен (`r.TSS.Enabled 1`, `r.TSS.Strength 2.0`)
![TSS ON](docs/strength_2.0.png)

## Установка

1. Скопируйте папку `TSS` в `Plugins/` вашего проекта
2. Запустите редактор
3. В консоли: `r.TSS.Enabled 1`

## Команды

| Команда | По умолчанию | Описание |
|---------|-------------|----------|
| `r.TSS.Enabled 1` | `0` | Включить шарпинг |
| `r.TSS.Enabled 0` | — | Выключить |
| `r.TSS.Strength <float>` | `1.0` | Сила шарпинга, 0.0–2.0 |
