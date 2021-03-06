/*     Copyright 2015-2018 Egor Yusov
 *  
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF ANY PROPRIETARY RIGHTS.
 *
 *  In no event and under no legal theory, whether in tort (including negligence), 
 *  contract, or otherwise, unless required by applicable law (such as deliberate 
 *  and grossly negligent acts) or agreed to in writing, shall any Contributor be
 *  liable for any damages, including any direct, indirect, special, incidental, 
 *  or consequential damages of any character arising as a result of this License or 
 *  out of the use or inability to use the software (including but not limited to damages 
 *  for loss of goodwill, work stoppage, computer failure or malfunction, or any and 
 *  all other commercial damages or losses), even if such Contributor has been advised 
 *  of the possibility of such damages.
 */

// EngineSandbox.cpp : Defines the entry point for the application.
//

#include "pch.h"
#include "TestShaderResourceLayout.h"
#include "BasicShaderSourceStreamFactory.h"

using namespace Diligent;

TestShaderResourceLayout::TestShaderResourceLayout( IRenderDevice *pDevice, IDeviceContext *pContext ) :
    UnitTestBase("Shader resource layout initialization test"),
    m_pDeviceContext(pContext)
{
    if (pDevice->GetDeviceCaps().DevType != DeviceType::Vulkan)
    {
        return;
    }

    ShaderCreationAttribs CreationAttrs;
    BasicShaderSourceStreamFactory BasicSSSFactory("Shaders");
    CreationAttrs.pShaderSourceStreamFactory = &BasicSSSFactory;
    CreationAttrs.EntryPoint = "main";


    RefCntAutoPtr<ISampler> pSamplers[4];
    IDeviceObject *pSams[4];
    for (int i = 0; i < 4; ++i)
    {
        SamplerDesc SamDesc;
        pDevice->CreateSampler(SamDesc, &(pSamplers[i]));
        pSams[i] = pSamplers[i];
    }

    RefCntAutoPtr<ITexture> pTex[4];
    TextureDesc TexDesc;
    TexDesc.Type = RESOURCE_DIM_TEX_2D;
    TexDesc.Width = 1024;
    TexDesc.Height = 1024;
    TexDesc.Format = TEX_FORMAT_RGBA8_UNORM_SRGB;
    TexDesc.BindFlags = BIND_SHADER_RESOURCE;
    IDeviceObject *pSRVs[4];
    for(int i=0; i < 4; ++i)
    {
        pDevice->CreateTexture(TexDesc, TextureData{}, &(pTex[i]));
        auto *pSRV = pTex[i]->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
        pSRV->SetSampler(pSamplers[i]);
        pSRVs[i] = pSRV;
    }

    TexDesc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
    TexDesc.Format = TEX_FORMAT_RGBA8_UNORM;
    RefCntAutoPtr<ITexture> pStorageTex[4];
    IDeviceObject *pUAVs[4];
    for (int i = 0; i < 4; ++i)
    {
        pDevice->CreateTexture(TexDesc, TextureData{}, &(pStorageTex[i]));
        pUAVs[i] = pStorageTex[i]->GetDefaultView(TEXTURE_VIEW_UNORDERED_ACCESS);
    }

    TexDesc.BindFlags = BIND_RENDER_TARGET;
    RefCntAutoPtr<ITexture> pRenderTarget;
    pDevice->CreateTexture(TexDesc, TextureData{}, &pRenderTarget);
    auto *pRTV = pRenderTarget->GetDefaultView(TEXTURE_VIEW_RENDER_TARGET);
    m_pDeviceContext->SetRenderTargets(1, &pRTV, nullptr);
    float Zero[4] = {};
    m_pDeviceContext->ClearRenderTarget(pRTV, Zero);

    BufferDesc BuffDesc;
    BuffDesc.uiSizeInBytes = 1024;
    BuffDesc.BindFlags = BIND_UNIFORM_BUFFER;
    RefCntAutoPtr<IBuffer> pUniformBuffs[4];
    IDeviceObject *pUBs[4];
    for (int i = 0; i < 4; ++i)
    {
        pDevice->CreateBuffer(BuffDesc, BufferData{}, &(pUniformBuffs[i]));
        pUBs[i] = pUniformBuffs[i];
    }

    BuffDesc.BindFlags = BIND_UNORDERED_ACCESS;
    BuffDesc.Mode = BUFFER_MODE_STRUCTURED;
    BuffDesc.ElementByteStride = 16;
    RefCntAutoPtr<IBuffer> pStorgeBuffs[4];
    IDeviceObject *pSBUAVs[4];
    for (int i = 0; i < 4; ++i)
    {
        pDevice->CreateBuffer(BuffDesc, BufferData{}, &(pStorgeBuffs[i]));
        pSBUAVs[i] = pStorgeBuffs[i]->GetDefaultView(BUFFER_VIEW_UNORDERED_ACCESS);
    }

    RefCntAutoPtr<IBuffer> pUniformTexelBuff, pStorageTexelBuff;
    IBufferView *pUniformTexelBuffSRV = nullptr, *pStorageTexelBuffUAV = nullptr;
    {
        Diligent::BufferDesc TxlBuffDesc;
        TxlBuffDesc.Name = "Uniform texel buffer test";
        TxlBuffDesc.uiSizeInBytes = 256;
        TxlBuffDesc.BindFlags = BIND_SHADER_RESOURCE;
        TxlBuffDesc.Usage = USAGE_DEFAULT;
        TxlBuffDesc.Format.ValueType = VT_FLOAT32;
        TxlBuffDesc.Format.NumComponents = 4;
        TxlBuffDesc.Format.IsNormalized = false;
        TxlBuffDesc.Mode = BUFFER_MODE_FORMATTED;
        pDevice->CreateBuffer(TxlBuffDesc, BufferData{}, &pUniformTexelBuff);
        pUniformTexelBuffSRV = pUniformTexelBuff->GetDefaultView(BUFFER_VIEW_SHADER_RESOURCE);

        TxlBuffDesc.Name = "Storage texel buffer test";
        TxlBuffDesc.BindFlags = BIND_UNORDERED_ACCESS;
        pDevice->CreateBuffer(TxlBuffDesc, BufferData{}, &pStorageTexelBuff);
        pStorageTexelBuffUAV = pStorageTexelBuff->GetDefaultView(BUFFER_VIEW_UNORDERED_ACCESS);
    }
    
    ResourceMappingDesc ResMappingDesc;
    ResourceMappingEntry MappingEntries[] = 
    {
        {"g_tex2D_Static", pSRVs[0]},
        {"g_tex2DArr_Static", pSRVs[0], 0},
        {"g_tex2DArr_Static", pSRVs[1], 1},
        {"g_sepTex2D_static", pSRVs[0]},
        {"g_sepTex2DArr_static", pSRVs[0], 0},
        {"g_sepTex2DArr_static", pSRVs[1], 1},
        {"g_tex2D_Mut", pSRVs[0]},
        {"g_tex2DArr_Mut", pSRVs[0], 0},
        {"g_tex2DArr_Mut", pSRVs[1], 1},
        {"g_tex2DArr_Mut", pSRVs[2], 2},
        {"g_tex2D_Dyn", pSRVs[0]},
        {"g_tex2DArr_Dyn", pSRVs[0], 0},
        {"g_tex2DArr_Dyn", pSRVs[1], 1},
        {"g_tex2DArr_Dyn", pSRVs[2], 2},
        {"g_tex2DArr_Dyn", pSRVs[3], 3},
        {"g_tex2DArr_Dyn", pSRVs[0], 4},
        {"g_tex2DArr_Dyn", pSRVs[1], 5},
        {"g_tex2DArr_Dyn", pSRVs[2], 6},
        {"g_tex2DArr_Dyn", pSRVs[3], 7},
        {"g_sepTex2D_mut", pSRVs[0]},
        {"g_sepTex2DArr_mut", pSRVs[0], 0},
        {"g_sepTex2DArr_mut", pSRVs[1], 1},
        {"g_sepTex2DArr_mut", pSRVs[2], 2},
        {"g_sepTex2D_dyn", pSRVs[0]},
        {"g_sepTex2DArr_dyn", pSRVs[0], 0},
        {"g_sepTex2DArr_dyn", pSRVs[1], 1},
        {"g_sepTex2DArr_dyn", pSRVs[2], 2},
        {"g_sepTex2DArr_dyn", pSRVs[3], 3},
        {"g_sepTex2DArr_dyn", pSRVs[0], 4},
        {"g_sepTex2DArr_dyn", pSRVs[1], 5},
        {"g_sepTex2DArr_dyn", pSRVs[2], 6},
        {"g_sepTex2DArr_dyn", pSRVs[3], 7},
        {}
    };
    ResMappingDesc.pEntries = MappingEntries;
    RefCntAutoPtr<IResourceMapping> pResMapping;
    pDevice->CreateResourceMapping(ResMappingDesc, &pResMapping);

    RefCntAutoPtr<IShader> pVS;
    {
        CreationAttrs.Desc.Name = "Shader resource layout test VS";
        CreationAttrs.Desc.ShaderType = SHADER_TYPE_VERTEX;
        CreationAttrs.SourceLanguage = SHADER_SOURCE_LANGUAGE_GLSL;
        CreationAttrs.FilePath = "Shaders\\ShaderResLayoutTestGL.vsh";
        ShaderVariableDesc VarDesc[] = 
        {
            { "g_tex2D_Static", SHADER_VARIABLE_TYPE_STATIC },
            { "g_tex2D_Mut", SHADER_VARIABLE_TYPE_MUTABLE },
            { "g_tex2D_Dyn", SHADER_VARIABLE_TYPE_DYNAMIC },
            { "g_tex2DArr_Static", SHADER_VARIABLE_TYPE_STATIC },
            { "g_tex2DArr_Mut", SHADER_VARIABLE_TYPE_MUTABLE },
            { "g_tex2DArr_Dyn", SHADER_VARIABLE_TYPE_DYNAMIC },
            { "g_sepTex2DArr_static", SHADER_VARIABLE_TYPE_STATIC },
            { "g_sepTex2D_mut", SHADER_VARIABLE_TYPE_MUTABLE },
            { "g_sepTex2DArr_mut", SHADER_VARIABLE_TYPE_MUTABLE },
            { "g_sepTex2D_dyn", SHADER_VARIABLE_TYPE_DYNAMIC },
            { "g_sepTex2DArr_dyn", SHADER_VARIABLE_TYPE_DYNAMIC },
            { "g_SamArr_static", SHADER_VARIABLE_TYPE_STATIC},
            { "g_Sam_mut", SHADER_VARIABLE_TYPE_MUTABLE },
            { "g_SamArr_mut", SHADER_VARIABLE_TYPE_MUTABLE },
            { "g_Sam_dyn", SHADER_VARIABLE_TYPE_DYNAMIC },
            { "g_SamArr_dyn", SHADER_VARIABLE_TYPE_DYNAMIC },
            { "UniformBuff_Mut", SHADER_VARIABLE_TYPE_MUTABLE },
            { "UniformBuff_Dyn", SHADER_VARIABLE_TYPE_DYNAMIC },
            { "UniformBuffArr_Mut", SHADER_VARIABLE_TYPE_MUTABLE },
            { "UniformBuffArr_Dyn", SHADER_VARIABLE_TYPE_DYNAMIC },
            { "storageBuff_Mut", SHADER_VARIABLE_TYPE_MUTABLE },
            { "storageBuff_Dyn", SHADER_VARIABLE_TYPE_DYNAMIC },
            { "storageBuffArr_Mut", SHADER_VARIABLE_TYPE_MUTABLE },
            { "storageBuffArr_Dyn", SHADER_VARIABLE_TYPE_DYNAMIC },
            { "g_tex2DStorageImgArr_Mut", SHADER_VARIABLE_TYPE_MUTABLE },
            { "g_tex2DStorageImgArr_Dyn", SHADER_VARIABLE_TYPE_DYNAMIC },
            { "g_tex2DNoResourceTest", SHADER_VARIABLE_TYPE_DYNAMIC },
            { "g_UniformTexelBuff_mut", SHADER_VARIABLE_TYPE_DYNAMIC },
            { "g_StorageTexelBuff_mut", SHADER_VARIABLE_TYPE_DYNAMIC }
        };
        StaticSamplerDesc StaticSamplers[] = 
        {
            {"g_tex2D_Static", SamplerDesc{}},
            {"g_tex2DArr_Mut", SamplerDesc{}},
            {"g_Sam_static", SamplerDesc{}},
            {"g_SamArr_mut", SamplerDesc{} },
            {"g_Sam_dyn", SamplerDesc{} },
            {"g_tex2DNoStaticSamplerTest", SamplerDesc{} }
        };
        CreationAttrs.Desc.VariableDesc = VarDesc;
        CreationAttrs.Desc.NumVariables = _countof(VarDesc);
        CreationAttrs.Desc.NumStaticSamplers = _countof(StaticSamplers);
        CreationAttrs.Desc.StaticSamplers = StaticSamplers;

        pDevice->CreateShader(CreationAttrs, &pVS);
        VERIFY_EXPR(pVS);
        
        //pVS->GetShaderVariable("g_tex2D_Static")->Set(pSRVs[0]);
        //pVS->GetShaderVariable("g_tex2DArr_Static")->SetArray(pSRVs, 0, 2);
        pVS->GetShaderVariable("g_sepTex2D_static")->Set(pSRVs[0]);
        pVS->GetShaderVariable("g_sepTex2DArr_static")->SetArray(pSRVs, 0, 2);
        pVS->GetShaderVariable("g_SamArr_static")->SetArray(pSams, 0, 2);
        pVS->GetShaderVariable("UniformBuff_Stat")->Set(pUBs[0]);
        pVS->GetShaderVariable("UniformBuffArr_Stat")->SetArray(pUBs, 0, 2);
        pVS->GetShaderVariable("storageBuff_Static")->Set(pSBUAVs[0]);
        pVS->GetShaderVariable("storageBuffArr_Static")->SetArray(pSBUAVs, 0, 2);
        pVS->GetShaderVariable("g_tex2DStorageImg_Stat")->Set(pUAVs[0]);
        pVS->GetShaderVariable("g_UniformTexelBuff")->Set(pUniformTexelBuffSRV);
        pVS->GetShaderVariable("g_StorageTexelBuff")->Set(pStorageTexelBuffUAV);
        pVS->GetShaderVariable("g_tex2D_Mut");
        LOG_INFO_MESSAGE("The above 2 warnings and 1 errors about missing shader resources are part of the test");
        pVS->BindResources(pResMapping, BIND_SHADER_RESOURCES_UPDATE_UNRESOLVED | BIND_SHADER_RESOURCES_ALL_RESOLVED);
    }

    RefCntAutoPtr<IShader> pPS;
    {
        CreationAttrs.Desc.Name = "Shader resource layout test FS";
        CreationAttrs.Desc.ShaderType = SHADER_TYPE_PIXEL;
        CreationAttrs.SourceLanguage = SHADER_SOURCE_LANGUAGE_GLSL;
        CreationAttrs.FilePath = "Shaders\\ShaderResLayoutTestGL.psh";

        pDevice->CreateShader(CreationAttrs, &pPS);
        VERIFY_EXPR(pPS);

        pPS->GetShaderVariable("g_tex2D_Static")->Set(pSRVs[0]);
        pPS->GetShaderVariable("g_tex2DArr_Static")->SetArray(pSRVs, 0, 2);
        //pPS->GetShaderVariable("g_sepTex2D_static")->Set(pSRVs[0]);
        //pPS->GetShaderVariable("g_sepTex2DArr_static")->SetArray(pSRVs, 0, 2);
        pPS->GetShaderVariable("g_SamArr_static")->SetArray(pSams, 0, 2);
        pPS->GetShaderVariable("UniformBuff_Stat")->Set(pUBs[0]);
        pPS->GetShaderVariable("UniformBuffArr_Stat")->SetArray(pUBs, 0, 2);
        pPS->GetShaderVariable("storageBuff_Static")->Set(pSBUAVs[0]);
        pPS->GetShaderVariable("storageBuffArr_Static")->SetArray(pSBUAVs, 0, 2);
        pPS->GetShaderVariable("g_tex2DStorageImg_Stat")->Set(pUAVs[0]);
        pPS->GetShaderVariable("g_UniformTexelBuff")->Set(pUniformTexelBuffSRV);
        pPS->GetShaderVariable("g_StorageTexelBuff")->Set(pStorageTexelBuffUAV);
        pPS->GetShaderVariable("storageBuff_Dyn");
        LOG_INFO_MESSAGE("The above 2 warnings and 1 errors about missing shader resources are part of the test");
        pPS->BindResources(pResMapping, BIND_SHADER_RESOURCES_UPDATE_UNRESOLVED | BIND_SHADER_RESOURCES_ALL_RESOLVED);
    }

    PipelineStateDesc PSODesc;
    PSODesc.Name = "Shader resource layout test";
    PSODesc.GraphicsPipeline.pVS = pVS;
    PSODesc.GraphicsPipeline.pPS = pPS;
    PSODesc.GraphicsPipeline.PrimitiveTopology = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    PSODesc.GraphicsPipeline.NumRenderTargets = 1;
    PSODesc.GraphicsPipeline.RTVFormats[0] = TEX_FORMAT_RGBA8_UNORM;
    PSODesc.GraphicsPipeline.DSVFormat = TEX_FORMAT_UNKNOWN;
    PSODesc.SRBAllocationGranularity = 16;

    RefCntAutoPtr<IPipelineState> pTestPSO;
    pDevice->CreatePipelineState(PSODesc, &pTestPSO);
    VERIFY_EXPR(pTestPSO);

    RefCntAutoPtr<IShaderResourceBinding> pSRB;
    pTestPSO->CreateShaderResourceBinding(&pSRB);

    pSRB->GetVariable(SHADER_TYPE_VERTEX, "UniformBuff_Stat");
    pSRB->GetVariable(SHADER_TYPE_PIXEL, "g_sepTex2DArr_static");
    LOG_INFO_MESSAGE("The above 2 errors about missing shader resources are part of the test");

    //pSRB->GetVariable(SHADER_TYPE_VERTEX, "g_tex2D_Mut")->Set(pSRVs[0]);
    //pSRB->GetVariable(SHADER_TYPE_VERTEX, "g_tex2DArr_Mut")->SetArray(pSRVs, 0, 3);
    //pSRB->GetVariable(SHADER_TYPE_VERTEX, "g_tex2D_Dyn")->Set(pSRVs[0]);
    //pSRB->GetVariable(SHADER_TYPE_VERTEX, "g_tex2DArr_Dyn")->SetArray(pSRVs, 0, 4);

    pSRB->GetVariable(SHADER_TYPE_VERTEX, "g_sepTex2D_mut")->Set(pSRVs[0]);
    pSRB->GetVariable(SHADER_TYPE_VERTEX, "g_sepTex2DArr_mut")->SetArray(pSRVs, 0, 3);
    pSRB->GetVariable(SHADER_TYPE_VERTEX, "g_sepTex2D_dyn")->Set(pSRVs[0]);
    pSRB->GetVariable(SHADER_TYPE_VERTEX, "g_sepTex2DArr_dyn")->SetArray(pSRVs, 0, 4);

    pSRB->GetVariable(SHADER_TYPE_VERTEX, "g_Sam_mut")->Set(pSams[0]);
    pSRB->GetVariable(SHADER_TYPE_VERTEX, "g_SamArr_dyn")->SetArray(pSams, 0, 4);

    pSRB->GetVariable(SHADER_TYPE_VERTEX, "UniformBuff_Mut")->Set(pUBs[0]);
    pSRB->GetVariable(SHADER_TYPE_VERTEX, "UniformBuffArr_Mut")->SetArray(pUBs, 0, 3);
    pSRB->GetVariable(SHADER_TYPE_VERTEX, "UniformBuff_Dyn")->Set(pUBs[0]);
    pSRB->GetVariable(SHADER_TYPE_VERTEX, "UniformBuffArr_Dyn")->SetArray(pUBs, 0, 4);

    pSRB->GetVariable(SHADER_TYPE_VERTEX, "storageBuff_Mut")->Set(pSBUAVs[0]);
    pSRB->GetVariable(SHADER_TYPE_VERTEX, "storageBuffArr_Mut")->SetArray(pSBUAVs, 0, 3);
    pSRB->GetVariable(SHADER_TYPE_VERTEX, "storageBuff_Dyn")->Set(pSBUAVs[0]);
    pSRB->GetVariable(SHADER_TYPE_VERTEX, "storageBuffArr_Dyn")->SetArray(pSBUAVs, 0, 4);

    pSRB->GetVariable(SHADER_TYPE_VERTEX, "g_tex2DStorageImgArr_Mut")->SetArray(pUAVs, 0, 2);
    pSRB->GetVariable(SHADER_TYPE_VERTEX, "g_tex2DStorageImgArr_Dyn")->SetArray(pUAVs, 0, 2);

    pSRB->GetVariable(SHADER_TYPE_VERTEX, "g_UniformTexelBuff_mut")->Set(pUniformTexelBuffSRV);
    pSRB->GetVariable(SHADER_TYPE_VERTEX, "g_StorageTexelBuff_mut")->Set(pStorageTexelBuffUAV);

    

    pSRB->GetVariable(SHADER_TYPE_PIXEL, "g_tex2D_Mut")->Set(pSRVs[0]);
    pSRB->GetVariable(SHADER_TYPE_PIXEL, "g_tex2DArr_Mut")->SetArray(pSRVs, 0, 3);
    pSRB->GetVariable(SHADER_TYPE_PIXEL, "g_tex2D_Dyn")->Set(pSRVs[0]);
    pSRB->GetVariable(SHADER_TYPE_PIXEL, "g_tex2DArr_Dyn")->SetArray(pSRVs, 0, 4);

    //pSRB->GetVariable(SHADER_TYPE_PIXEL, "g_sepTex2D_mut")->Set(pSRVs[0]);
    //pSRB->GetVariable(SHADER_TYPE_PIXEL, "g_sepTex2DArr_mut")->SetArray(pSRVs, 0, 3);
    //pSRB->GetVariable(SHADER_TYPE_PIXEL, "g_sepTex2D_dyn")->Set(pSRVs[0]);
    //pSRB->GetVariable(SHADER_TYPE_PIXEL, "g_sepTex2DArr_dyn")->SetArray(pSRVs, 0, 4);
    
    pSRB->GetVariable(SHADER_TYPE_PIXEL, "g_Sam_mut")->Set(pSams[0]);
    pSRB->GetVariable(SHADER_TYPE_PIXEL, "g_SamArr_dyn")->SetArray(pSams, 0, 4);

    pSRB->GetVariable(SHADER_TYPE_PIXEL, "UniformBuff_Mut")->Set(pUBs[0]);
    pSRB->GetVariable(SHADER_TYPE_PIXEL, "UniformBuffArr_Mut")->SetArray(pUBs, 0, 3);
    pSRB->GetVariable(SHADER_TYPE_PIXEL, "UniformBuff_Dyn")->Set(pUBs[0]);
    pSRB->GetVariable(SHADER_TYPE_PIXEL, "UniformBuffArr_Dyn")->SetArray(pUBs, 0, 4);

    pSRB->GetVariable(SHADER_TYPE_PIXEL, "storageBuff_Mut")->Set(pSBUAVs[0]);
    pSRB->GetVariable(SHADER_TYPE_PIXEL, "storageBuffArr_Mut")->SetArray(pSBUAVs, 0, 3);
    pSRB->GetVariable(SHADER_TYPE_PIXEL, "storageBuff_Dyn")->Set(pSBUAVs[0]);
    pSRB->GetVariable(SHADER_TYPE_PIXEL, "storageBuffArr_Dyn")->SetArray(pSBUAVs, 0, 4);

    pSRB->GetVariable(SHADER_TYPE_PIXEL, "g_tex2DStorageImgArr_Mut")->SetArray(pUAVs, 0, 2);
    pSRB->GetVariable(SHADER_TYPE_PIXEL, "g_tex2DStorageImgArr_Dyn")->SetArray(pUAVs, 0, 2);

    pSRB->GetVariable(SHADER_TYPE_PIXEL, "g_UniformTexelBuff_mut")->Set(pUniformTexelBuffSRV);
    pSRB->GetVariable(SHADER_TYPE_PIXEL, "g_StorageTexelBuff_mut")->Set(pStorageTexelBuffUAV);

    pSRB->BindResources(SHADER_TYPE_PIXEL | SHADER_TYPE_VERTEX, pResMapping, BIND_SHADER_RESOURCES_UPDATE_UNRESOLVED | BIND_SHADER_RESOURCES_ALL_RESOLVED);

    pContext->SetPipelineState(pTestPSO);
    pContext->CommitShaderResources(pSRB, COMMIT_SHADER_RESOURCES_FLAG_TRANSITION_RESOURCES);
    
    DrawAttribs DrawAttrs;
    DrawAttrs.NumVertices = 3;
    pContext->Draw(DrawAttrs);

    pSRB->GetVariable(SHADER_TYPE_PIXEL, "storageBuff_Dyn")->Set(pSBUAVs[1]);
    pSRB->GetVariable(SHADER_TYPE_VERTEX, "g_tex2D_Dyn")->Set(pSRVs[1]);
    pSRB->GetVariable(SHADER_TYPE_VERTEX, "g_sepTex2D_dyn")->Set(pSRVs[1]);
    pSRB->GetVariable(SHADER_TYPE_PIXEL, "g_SamArr_dyn")->SetArray(pSams+1, 1, 3);
    pSRB->GetVariable(SHADER_TYPE_PIXEL, "UniformBuff_Dyn")->Set(pUBs[1]);
    pSRB->GetVariable(SHADER_TYPE_VERTEX, "g_tex2DStorageImgArr_Dyn")->SetArray(pUAVs+1, 1, 1);
    pContext->CommitShaderResources(pSRB, COMMIT_SHADER_RESOURCES_FLAG_TRANSITION_RESOURCES);
    pContext->Draw(DrawAttrs);

    SetStatus(TestResult::Succeeded);
}
