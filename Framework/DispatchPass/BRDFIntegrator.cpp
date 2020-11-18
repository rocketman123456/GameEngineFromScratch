#include "BRDFIntegrator.hpp"

#include "GraphicsManager.hpp"
#include "IPipelineStateManager.hpp"

using namespace My;

void BRDFIntegrator::Dispatch(Frame& frame) {
    auto& pPipelineState =
        g_pPipelineStateManager->GetPipelineState("PBR BRDF CS");

    // Set the color shader as the current shader program and set the matrices
    // that it will use for rendering.
    g_pGraphicsManager->SetPipelineState(pPipelineState, frame);

    const uint32_t width = 512u;
    const uint32_t height = 512u;
    const uint32_t depth = 1u;
    if (frame.brdfLUT == -1) {
        g_pGraphicsManager->GenerateTextureForWrite("BRDF_LUT", width, height);
        frame.brdfLUT = g_pGraphicsManager->GetTexture("BRDF_LUT");
    }
    g_pGraphicsManager->BindTextureForWrite("BRDF_LUT", 0);
    g_pGraphicsManager->Dispatch(width, height, depth);
}