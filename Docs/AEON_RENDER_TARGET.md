# Aeon render target -- honest numbers

Hardware: AMD RX 9060 XT 16GB, Ryzen 5700X, Linux / Vulkan (RADV).

## What we run
- Software Lumen GI + reflections (Lumen Lite once enabled in 5.8's project settings is the 60fps path)
- Nanite, Virtual Shadow Maps, MegaLights (production-ready in 5.8)
- TSR: 1440p internal (dynamic 60-100%), upscaled to the display
- Hardware ray tracing: OFF. Opt-in experiment only. It is the one Unreal path that
  is still uneven on RADV and it would make the platform look slower than it is.

## Targets, not promises
| Profile     | Output | Internal        | GI              | Frame  |
|-------------|--------|-----------------|-----------------|--------|
| Performance | 4K     | 1440p dyn (TSR) | Lumen Lite / SW | 60 fps |
| Quality     | 4K     | 1620p dyn (TSR) | Software Lumen  | 40 fps |

Native 4K60 with full Lumen is not a target for this card. PS5 does not do it either.

## Why 16GB VRAM matters here
Nanite + virtual textures + 4K materials fit without streaming stalls. That is the
Steam Machine's most-cited weakness (8GB) and Aeon's clearest hardware win.
