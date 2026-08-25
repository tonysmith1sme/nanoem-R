# fx9-next 接手说明

下一 session **看不到** 规划对话。先读本文，再读代码，不要重开「删掉旧转译」的空讨论。

仓库：`/Users/seele/Documents/GitRepo/nanoem-R`
分支：`main`（已 push）
最新相关提交：`2279a07 [fx9-next] Flatten struct entry I/O and convert mixed int/float`
日期：2026-08-25

---

## 1. 已锁定的决策（不要改，除非用户明确改口）

| 项 | 选择 |
|---|---|
| 长期范围 | 编译器 + 运行时全重写，最后连 sokol/ImGui 一起换 |
| 验收 | 先对齐 D3D9/MME **语义**；像素对图以后再补（本机 macOS，没有原版 MME） |
| 效率 | 兼容优先，同时要编译速度和运行时 |
| IR / 后端 | **自研** HLSL+Effect 前端 → SPIR-V → SPIRV-Cross。MME 路径 **不链接 glslang** |
| 分期 | **第一期只交编译器**；运行时仍接现有 `Effect::load` / sokol |
| 第一期效果 | 合成语料库 + `MME/` 滤镜必须编过；ray-mmd 只作进度，不卡关 |
| 挂接 | 保持 `plugin_effect` ABI + `emapp/resources/protobuf/effect.proto` |
| 旧代码 | `fx9` 与 `fx9-next` **并行**；过线再删 Lemon/glslang-HLSL 和 `obsolete/` |
| DX9 缺口 | 第一期就要做 `texM3x3*` 和 >16 sampler 重映射（不要再标 manifest fail） |
| 新代码位置 | `dependencies/fx9-next` |
| 二期 | 新运行时（换 Effect/Technique/Pass/dx9rt），仍共享宿主 device |
| 三期 | 拆 sokol（含 ImGui） |
| 默认编译器 | **仍是 `fx9`**，直到用户同意切 `NANOEM_EFFECT_COMPILER=fx9-next` |

不要做：

- 不改 `Effect.cc` 加载/Script 语义（除非 ABI 对不上的最小修补）
- 不换 sokol / ImGui / 默认材质
- 不删 `dependencies/glslang`（内置着色器构建仍要用）
- 不做与原版 MME 的像素 diff
- **没有路径键源码 shim**（不要把 `EffectSourcePipeline` 的 ray-mmd 改写迁过来）
- 不切默认编译器，除非用户明确说切

冻结的 ABI 约定（运行时已写死）：

- UBO 名：`vs_uniforms_vec4` / `ps_uniforms_vec4`
- Metal entry：`fx9_metal_main`
- 宏：`NANOEM`、`NANOEM_OUTPUT_SHADER_LANGUAGE_{GLSL,ESSL,HLSL,MSL,SPIRV}`、`MME_MIPMAP`
- `effect.proto` 字段与 `Effect::load` 一致
- 整效果 all-or-nothing：任一 pass 失败则整效果失败

---

## 2. 现在做到哪了（第一期，未切默认）

新编译器已经能：

- 编过 `dependencies/fx9/test/effects/corpus/` **全部 19 个**（含原 fx9 标 fail 的 `03_texM3x3`、`05_many_samplers`）
- 编过 `MME/` 滤镜 8/8：Diffusion7、AnimeScreenTex、LightBloom（含空格文件名 + `.fxsub`）、msUnsharp、ikBokeh、ikDiffusion1/2、PostAdultShader
- 编过 ray-mmd 进度样本 5/5：`Main/main.fx`、`ray.fx`、FXAA、Toon、`material_glass.fx`（`FOG_ENABLE=0`）
- 输出可被现有 `Effect::load` 解的 protobuf：technique/pass、shader body（GLSL/HLSL/MSL/SPIR-V）、参数语义（WORLD 等）、includes、attribute/semantic/uniform/symbol、render states
- AlphaTest 编进 PS（`discard`）；可选 sRGB 曲线（`0.0031308`，避免运行时再注入）
- `-DNANOEM_EFFECT_COMPILER=fx9-next` 下 `plugin_effect` 能链上

`fx9next_test` 最近一次：**19 cases / 152 assertions 全绿**。

这只保证 **能编过并打出非空产物**，不保证画面正确、不保证在 app 里加载后能画。

---

## 3. 架构

```
.fx / .fxsub (SJIS/UTF-8)
    → Preprocessor (#include / #define / #if / ## / \ 续行)
    → Lexer + Parser（自研 SM2/SM3 + technique/pass/script）
    → AST (TranslationUnit)
    → SpirvEmitter（手写 SPIR-V + GLSL.std.450）
    → SPIRV-Cross → GLSL 330 / ESSL / HLSL SM4.1 / MSL 2.0
    → ProductWriter → protobuf fx9.effect.Effect
    → plugin_effect（仅当 NANOEM_EFFECT_COMPILER=fx9-next）
    → 现有 Effect::load → sokol
```

门面：`fx9next::Compiler`，API 对齐 `fx9::Compiler`（无 glslang 类型）。

| 文件 | 职责 |
|---|---|
| `Compiler.*` / `Pipeline.*` | 门面、宏、读文件、串起各阶段 |
| `Encoding.*` | UTF-8 / CP932(SJIS) |
| `Preprocessor.*` | C 预处理、续行、`##`、map include |
| `Lexer.*` / `Parser.*` / `AST.*` / `Type.*` | 前端 |
| `SpirvEmitter.*` | IR 发射 |
| `Translator.*` | SPIRV-Cross |
| `ProductWriter.*` | protobuf + alpha bake |
| `RenderState.*` | D3DRS 名/值表 |
| `emapp/plugins/effect/effect.cc` | `#ifdef NANOEM_USE_FX9_NEXT` |
| `emapp/plugins/effect/CMakeLists.txt` | 开关；**始终同时编 fx9 和 fx9next** |

语言范围：DX9 HLSL SM2/SM3 + Effect 框架，不是 SM4+。

---

## 4. 怎么编、怎么测

本机已有 `out/core`（darwin / clang / 当前是 x86_64）。

```bash
# 默认仍走旧 fx9；只编新编译器测试
cmake --build out/core --target fx9next_test -j8
./out/core/fx9next_test --reporter compact

# 用新编译器链 plugin_effect（不要提交 cache 里的默认值）
cmake -DNANOEM_EFFECT_COMPILER=fx9-next -S . -B out/core
cmake --build out/core --target plugin_effect fx9next_test -j8

# 测完改回
cmake -DNANOEM_EFFECT_COMPILER=fx9 -S . -B out/core
```

完整 app 对齐 CI（见 `.github/workflows/main.yml` 的 `build-macos`）：

```bash
cmake -DCONFIG=release -P scripts/build.cmake
mkdir -p out/core && cd out/core
cmake -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_OSX_ARCHITECTURES="$(uname -m)" \
  -DFX9_ENABLE_OPTIMIZER=OFF \
  -DNANOEM_ENABLE_BULLET=ON -DNANOEM_ENABLE_ICU=ON \
  -DNANOEM_ENABLE_MIMALLOC=ON -DNANOEM_ENABLE_NMD=ON \
  -DNANOEM_ENABLE_SANDBOX=ON -DNANOEM_ENABLE_TBB=OFF \
  -DNANOEM_ENABLE_TEST=ON \
  -DNANOEM_INSTALL_EFFECT_PLUGIN=ON \
  -DNANOEM_TARGET_COMPILER=clang -GNinja ../..
cmake --build . --config release
ctest --output-on-failure
```

`MME/` 在 `.gitignore` 里，是本地效果包（ray-mmd-1.5.2 + 滤镜），不是原版 MME 引擎源码。

提交格式跟仓库：一行、`[fx9-next] …` / `[emapp] …`，不要正文、不要提交无关脏文件。

工作区里常有 **不要动** 的东西：`Parser 3.cc`、`CMakeLists 2.txt`、`docs/conf.py`、sokol/macos/WGPU 未提交改动、若干 `?` 的 dependencies。

---

## 5. 编译器已实现 / 仍弱

### 已有

- 预处理：`#include`（文件 + `addIncludeSource`）、`#define` 对象/函数宏、`#if/#ifdef/#elif/#else/#endif`、`defined()`、`##`、`\` 续行、指令行 `//` 剥离
- 编码：非 UTF-8 当 CP932
- 语法：technique/pass/sampler_state、注解（邻接字符串拼接）、`register(sN)`、`out`/`inout`、默认参数、`compile vs_3_0 foo(float2(1,0))` 嵌套括号、无大小数组、花括号初值、尾逗号、`static const`/`inline`、空 `;`
- SPIR-V：if/for/while/do-while、`?:`（OpSelect）、`out` 插值、用户函数**内联**、数组 AccessChain、结构体入口拆字段、`tex2D*`、`texM3x3`/`texM3x3vspec`、矩阵 `_m00` 读写、`mul` 矩阵×向量/向量×矩阵、GLSL.std.450（saturate/lerp/normalize/pow/sin/cos/max/min/clamp/step/sqrt/cross/length/frac…）
- 产物：参数 class/type/semantic/value、includes、shader interface、alpha-test bake

### 已知弱项（画面/加载会踩）

按优先级：

1. **没在 app 里跑过。** 编过 ≠ `Effect::load` + sokol 能画。下一步应 `-DNANOEM_EFFECT_COMPILER=fx9-next` 开 app，加载 Diffusion / LightBloom / ray-mmd。
2. **Uniform 不是真正的 UBO。** 数值全局是 Private 变量，初值几乎是 0；运行时往 `vs_uniforms_vec4` 填的 WORLD/VIEW 绑不上着色器里的 `g_world`。这是「能编不能画」的最大洞。
3. **插值 location 靠 semantic 表**（POSITION=0、TEXCOORD0=4…），和 sokol 默认 attr 槽可能错位；struct 拆分后 VS/PS 要对得上。
4. **函数内联**不支持递归；复杂 ray-mmd 函数可能爆代码或丢赋值。
5. **for 的归纳变量**放在 Private（避免函数中途 OpVariable）；`i++` 已做，但 `i += 1` / 非 ident 左值仍弱。
6. **while 的条件在 header 之后单独块**，极端嵌套可能让 SPIRV-Cross 不高兴。
7. **局部数组**用 Private + ArrayStride；`const float w[9] = {…}` 靠 CompositeConstruct。
8. **texM3x3** 按「三行 .z 拼方向再 2D 采样」近似，不是完整 ps_1.x 语义。
9. **sRGB bake** 依赖找得到 `FragColor =` / `out vec4 _N =`；找不到就只靠运行时注入（MSL 注入本来就不完整）。
10. **VPOS/VFACE** 只当普通 varying，没有 FragCoord / FrontFacing。
11. **没有 SPIRV-Tools validate**（CI 里 optimizer 也是 OFF）。只有自制 word-count 扫描。
12. **PostMovie 等 Shift-JIS 路径名** 在 compatibility 列表里，滤镜测试没覆盖那个文件。
13. Parser 对 `typedef`、部分 annotation 无类型名、FFP 状态仍粗。

---

## 6. 下一 session 建议顺序

不要开始二期/三期，除非用户改口。

### A. 先让「编过的效果在 app 里能画」（最高优先）

1. 用 `NANOEM_EFFECT_COMPILER=fx9-next` 编 `plugin_effect`，跑 nanoem-R.app。
2. 依次加载：`MME/Diffusion7/Diffusion.fx` → LightBloom → ikBokeh → `ray-mmd-1.5.2/ray.fx`（先改 `ray.conf` 的 `FOG_ENABLE 0`）。
3. 对着 log / 品红画面 / 绑错图，修 **UBO / 语义 / location**。
4. 数值全局应进真正的 `vs_uniforms_vec4` / `ps_uniforms_vec4`（float4 数组或等价 UBO），symbol 的 `register_index` 要和运行时 `GlobalUniform` 一致。对照：
   - `emapp/src/Effect.cc`（`uniform_block_name`、`n_symbols`、`n_uniforms`）
   - `emapp/src/effect/GlobalUniform.cc`
   - 旧实现 `dependencies/fx9/src/EffectPipeline.cc` 里 register 打包

### B. 质量补丁（仍属第一期）

- VPOS → FragCoord，VFACE → FrontFacing
- 更稳的 sRGB/alpha bake（MSL）
- 矩阵 `_11` 与列赋值的更多写法
- `texCUBE` 真 cubemap（现在当 2D）
- 给 `sandbox/effect_corpus` / `nanoem_sandbox_effect_corpus` 接 fx9-next 开关
- 更新 `docs/architecture.rst`、`docs/faq_effect.rst`（等切默认或用户要求再写用户文档）

### C. 切默认（需用户点头）

过线标准仍是：语料库全绿 + `MME/` 滤镜全绿 + app 里至少滤镜能画。然后：

- 默认 `NANOEM_EFFECT_COMPILER=fx9-next`
- 删 fx9 的 Lemon/glslang-HLSL 实现和 `emapp/plugins/obsolete/`
- **留下 glslang** 给 `emapp/resources/shaders`

### D. 二期 / 三期（只作路线，别开工）

- 二期：重写 `Effect` / `Technique` / `Pass` / `dx9rt`；3D 仍画到离屏视口；ImGui 只采样
- 三期：宿主 Metal/D3D11 device 收归新层，迁出全部 `sg::*`（等于重写 Project 画帧 + ImGuiWindow + 各平台 service）

sokol 并不拥有设备：宿主建 MTL/D3D11，3D 离屏，ImGui 采样 `viewportPrimaryImage`。双后端必须共享 **device + 视口颜色纹理**，不要去抢 sokol 的 command encoder。

---

## 7. 相关旧栈（对照用，不要先删）

- `dependencies/fx9/`：现行编译器（Lemon + glslang HLSL → SPIR-V → SPIRV-Cross）
- `dependencies/fx9/src/EffectSourcePipeline.cc`：路径键 shim（ray-mmd 等）——新编译器故意不迁
- `emapp/src/Effect.cc`（约 6600 行）、`emapp/src/effect/*`、`dx9rt/`
- `emapp/plugins/obsolete/`：MojoShader / D3DX9
- `docs/faq_effect.rst`、`docs/architecture.rst`、`docs/effect.rst`

Commit 风格样本：

```
[fx9-next] Flatten struct entry I/O and convert mixed int/float
[emapp] Wire NANOEM_EFFECT_COMPILER switch for fx9-next
```

---

## 8. 给下一 session 的第一句话

「读 `docs/fx9-next-handoff.md`。继续第一期：不要切默认、不要动 sokol。优先让 fx9-next 的全局 uniform 进 `vs_uniforms_vec4`/`ps_uniforms_vec4`，并用 `NANOEM_EFFECT_COMPILER=fx9-next` 在 app 里加载 `MME/Diffusion7/Diffusion.fx`。」
