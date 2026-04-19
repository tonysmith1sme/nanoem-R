/*
   Copyright (c) 2015-2023 hkrn All rights reserved

   This file is part of emapp component and it's licensed under Mozilla Public License. see LICENSE.md for more details.
 */

#include "../common.h"

#include "emapp/PluginFactory.h"
#include "emapp/StringUtils.h"
#include "emapp/URI.h"
#include "emapp/FileUtils.h"
#include "emapp/plugin/EffectPlugin.h"
#include "emapp/sdk/Effect.h"

#include "bx/os.h"

using namespace nanoem;
using namespace test;

namespace {

static String
resolveMMEPath(const char *relativePath)
{
    String path(NANOEM_TEST_FIXTURE_PATH);
    path.append("/../../../../MME/");
    path.append(relativePath);
    return path;
}

static String
resolveEffectPluginPath()
{
    String path(NANOEM_TEST_FIXTURE_PATH);
#ifdef CMAKE_INTDIR
    path.append("/../../../emapp/plugins/effect/" CMAKE_INTDIR "/plugin_effect.");
#else
    path.append("/../../emapp/plugins/effect/plugin_effect.");
#endif
    path.append(BX_DL_EXT);
    return path;
}

static bool
compileEffectAsHLSL(const URI &effectURI, String &failureReason)
{
    bool succeeded = false;
    Application application(nullptr);
    Error error;
    const URI &pluginURI = URI::createFromFilePath(resolveEffectPluginPath());
    plugin::EffectPlugin *plugin = PluginFactory::createEffectPlugin(pluginURI, application.eventPublisher(), error);
    REQUIRE(plugin != nullptr);
    plugin->setOption(NANOEM_APPLICATION_PLUGIN_EFFECT_OPTION_OUTPUT_HLSL, 1, error);
    plugin->setOption(NANOEM_APPLICATION_PLUGIN_EFFECT_OPTION_OPTIMIZATION, 0, error);
    plugin->setOption(NANOEM_APPLICATION_PLUGIN_EFFECT_OPTION_VALIDATION, 0, error);
    if (!error.hasReason()) {
        ByteArray output;
        succeeded = plugin->compile(effectURI, output);
        if (!succeeded) {
            failureReason = plugin->failureReason();
        }
    }
    else {
        failureReason = error.reasonConstString();
    }
    PluginFactory::destroyEffectPlugin(plugin);
    return succeeded;
}

} /* namespace anonymous */

TEST_CASE("effect_mme_hlsl_compiles_selected_samples", "[emapp][effect][probe]")
{
    static const char *kSamplePaths[] = {
        "Diffusion7/Diffusion.fx",
        "AnimeScreenTex_v1.0/A-screen.fx",
        "msUnsharp/msUnsharp.fx",
        "ikBokeh_v020a_SJ/ikBokeh.fx",
        "ikDiffusion/ikDiffusion/Diffusion1/ikDiffusion1.fx",
        "ikDiffusion/ikDiffusion/Diffusion2/ikDiffusion2.fx",
        "PostAdultShaderS2_v013/PostAdultShader.fx",
        "PostMovie夵曄/墿宯/嬥巺悵怓.fx",
        "ray-mmd-1.5.2/ray.fx",
        "ray-mmd-1.5.2/Extension/FXAA/FXAA.fx",
        "ray-mmd-1.5.2/Materials/Toon/Toon.fx",
        "ray-mmd-1.5.2/Materials/Transparent/material_glass.fx",
    };
    for (const char *relativePath : kSamplePaths) {
        const String path(resolveMMEPath(relativePath));
        const URI fileURI(URI::createFromFilePath(path));
        INFO(path.c_str());
        if (!FileUtils::exists(fileURI)) {
            WARN(path.c_str());
            continue;
        }
        String reason;
        const bool succeeded = compileEffectAsHLSL(fileURI, reason);
        INFO(reason.c_str());
        CHECK(succeeded);
    }
}
