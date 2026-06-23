# TSS — Texture Scaling Suite

FSR 2 аналог, написанный исключительно на **B⁺** → HLSL.

## Сборка

```cmd
bpc hlsl src\tss_shader.b+ -o src\tss_shader.hlsl
dxc -T cs_6_0 -E TSS_EASU src\tss_shader.hlsl -Fo tss_easu.cso
```

## Структура

- `src/*.b+` — исходники шейдеров на B⁺
- `src/*.hlsl` — сгенерированный HLSL
