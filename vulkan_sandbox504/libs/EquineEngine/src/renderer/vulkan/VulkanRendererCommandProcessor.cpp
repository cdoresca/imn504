#include "VulkanRenderer.hpp"
#include "rhi/PipelineDesc.hpp"
#include "shader/shader.hpp"
#include <iostream>

namespace VKRenderer {
void Renderer::clearCommandList(RHI::CommandList &newCommandList) {
    // if pipeline in the old and new command list is the same, we can keep it
    for (auto pipelineID : m_frameData[0].pipelineIDs) {
        if (std::find_if(newCommandList.pipelines.begin(), newCommandList.pipelines.end(), [pipelineID](RHI::PipelineDesc *pipeline) { return pipeline->uuid == pipelineID; }) == newCommandList.pipelines.end()) {
            Pipeline &localPipeline = m_pipelineRegistry[pipelineID];
            m_logicalDevice.destroyPipeline(localPipeline.pipeline);
            m_logicalDevice.destroyPipelineLayout(localPipeline.layout);
            for (auto layout : localPipeline.setLayouts) {
                m_logicalDevice.destroyDescriptorSetLayout(layout);
            }
            m_pipelineRegistry.erase(pipelineID);
        } else {
            // clear instrumentation
            Pipeline &localPipeline = m_pipelineRegistry[pipelineID];
            localPipeline.instrumentationInfo.pipelineStatsIDs.clear();
            localPipeline.instrumentationInfo.timerIDs.clear();
            localPipeline.instrumentationInfo.meshPrimIDs.clear();
        }
    }
    m_logicalDevice.resetDescriptorPool(m_descriptorPool);
    uint count = 0;
    for (FrameData &frameData : m_frameData) {
        for (int i = 0; i < frameData.pipelineIDs.size(); i++) {
            for (auto [name, ubo] : frameData.ubos[i]) {
                m_logicalDevice.unmapMemory(ubo.memory);
                m_logicalDevice.destroyBuffer(ubo.buffer);
                m_logicalDevice.freeMemory(ubo.memory);
            }
        }
        frameData.ubos.clear();
        frameData.descriptorSets.clear();
        frameData.pipelineIDs.clear();
        resetQueryPools(count);
        count++;
    }
    m_intermediateCommandList.clear();
    m_perfCounterGlobals.timestampResults.clear();
    m_perfCounterGlobals.pipelineStatisticsResults.clear();
    m_perfCounterGlobals.meshPrimitivesResults.clear();
    m_perfCounterGlobals.timestampQueryIndex = 0;
    m_perfCounterGlobals.pipelineStatisticsQueryIndex = 0;
    m_perfCounterGlobals.meshPrimitivesQueryIndex = 0;
}

void Renderer::compileAndUseCommandList(RHI::CommandList &commandList) {
    if (m_intermediateCommandList.hash == commandList.hash) {
        if (m_intermediateCommandList.dynamicHash != commandList.dynamicHash) {
            m_intermediateCommandList.inputParameters = commandList.parameters;
            m_intermediateCommandList.dynamicHash = commandList.dynamicHash;
        }
        return;
    }

    m_logicalDevice.waitIdle();
    clearCommandList(commandList);

    m_intermediateCommandList.hash = commandList.hash;
    m_intermediateCommandList.inputParameters = commandList.parameters;
    m_intermediateCommandList.dynamicHash = commandList.dynamicHash;

    std::list<vk::DescriptorBufferInfo> TempBufferInfos;
    std::list<vk::DescriptorImageInfo> TempImageInfos;
    std::vector<vk::WriteDescriptorSet> descriptorWrites;
    // create pipeline if they dont exist
    // allocate descriptor sets
    // add pipeline id to frameData
    // create UBOs and memory
    // fill in the descriptor writes for the UBOs
    for (FrameData &frameData : m_frameData) {
        frameData.ubos.resize(commandList.pipelines.size());
        frameData.descriptorSets.resize(commandList.pipelines.size());
        int count = 0;
        for (auto pipeline : commandList.pipelines) {
            if (pipeline->isDirty && m_pipelineRegistry.find(pipeline->uuid) != m_pipelineRegistry.end()) {
                Pipeline &localPipeline = m_pipelineRegistry[pipeline->uuid];
                m_logicalDevice.destroyPipeline(localPipeline.pipeline);
                m_logicalDevice.destroyPipelineLayout(localPipeline.layout);
                for (auto layout : localPipeline.setLayouts) {
                    m_logicalDevice.destroyDescriptorSetLayout(layout);
                }
                m_pipelineRegistry.erase(pipeline->uuid);
            }
            createPipeline(*pipeline);
            frameData.pipelineIDs.push_back(pipeline->uuid);
            Pipeline &localPipeline = m_pipelineRegistry[pipeline->uuid];
            if (localPipeline.setLayouts.size() == 0) {
                continue;
            }
            vk::DescriptorSetAllocateInfo allocInfo(m_descriptorPool, localPipeline.setLayouts.size(), localPipeline.setLayouts.data());
            std::vector<vk::DescriptorSet> alloc = m_logicalDevice.allocateDescriptorSets(allocInfo);
            std::copy(alloc.begin(), alloc.begin() + (localPipeline.setLayouts.size()), frameData.descriptorSets[count].begin());

            // gather all the UBOs and Variants used in this frame
            auto ubos = pipeline->getUbos();
            for (auto variants : ubos) {
                if (auto findit = frameData.ubos[count].find(variants[0].layout.uboName); findit == frameData.ubos[count].end()) {
                    auto [buffer, memory] = createBufferInternal(variants[0].layout.paddedSize, vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
                    frameData.ubos[count][variants[0].layout.uboName] = {
                        {buffer, memory},
                        nullptr
                    };
                    frameData.ubos[count][variants[0].layout.uboName].gpuPtr = m_logicalDevice.mapMemory(frameData.ubos[count][variants[0].layout.uboName].memory, 0, VK_WHOLE_SIZE, {});
                }
            }

            auto localDescriptors = pipeline->getDescriptorSetInfos();
            for (auto descriptor : localDescriptors) {
                for (auto binding : descriptor.bindings) {
                    if (binding.binding.descriptorType == vk::DescriptorType::eUniformBuffer) {
                        TempBufferInfos.emplace_back(frameData.ubos[count][binding.uboName].buffer, 0, VK_WHOLE_SIZE);
                        descriptorWrites.emplace_back(frameData.descriptorSets[count][descriptor.setIndex], binding.binding.binding, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr, &TempBufferInfos.back());
                    } else {
                        // check if slot is already filled, otherwise error (or bind missing texture?)
                    }
                }
            }
            count++;
        }
    }

    // process commands
    for (const std::unique_ptr<RHI::RHICommand> &command : commandList.commands) {
        switch (command->type) {
        case RHI::CommandType::UseResource:
            resolveUseResourceCommand(static_cast<RHI::UseResourceCommand *>(command.get()), descriptorWrites, TempBufferInfos, TempImageInfos);
            continue;
        case RHI::CommandType::UseSampler:
            resolveUseSamplerCommand(static_cast<RHI::UseSamplerCommand *>(command.get()), descriptorWrites, TempImageInfos);
            continue;
        case RHI::CommandType::BindPipeline:
            bindPipelineCommand(m_intermediateCommandList, static_cast<RHI::BindPipelineCommand *>(command.get()));
            continue;
        case RHI::CommandType::Barrier:
            resolveBarrierCommand(m_intermediateCommandList, static_cast<RHI::BarrierCommand *>(command.get()));
            continue;

            // check we are in a context
            if (m_intermediateCommandList.context.lastBoundPipeline == nullptr) {
                throw std::runtime_error("Cannot use a command outside of a pipeline context");
            }
        case RHI::CommandType::UseVertexBuffer:
            resolveUseVertexBuffer(m_intermediateCommandList, static_cast<RHI::UseVertexBufferCommand *>(command.get()));
            continue;
        case RHI::CommandType::UseIndexBuffer:
            resolveUseIndexBuffer(m_intermediateCommandList, static_cast<RHI::UseIndexBufferCommand *>(command.get()));
            continue;
        default:
            resolveCommand(m_intermediateCommandList, command.get());
        }
    }
    // close rendering if still on
    if (m_intermediateCommandList.context.isRendering) {
        endRendering(m_intermediateCommandList);
    }

    m_logicalDevice.updateDescriptorSets(descriptorWrites, {});
}
void Renderer::resolveUseResourceCommand(RHI::UseResourceCommand *command, std::vector<vk::WriteDescriptorSet> &descriptorWrites, std::list<vk::DescriptorBufferInfo> &TempBufferInfos, std::list<vk::DescriptorImageInfo> &TempImageInfos) {
    if (command->m_resourceDesc->type == RHI::ResourceDesc::Type::eBuffer) {
        TempBufferInfos.emplace_back(m_bufferRegistry[command->m_resourceDesc->uuid].buffer, 0, VK_WHOLE_SIZE);
        for (auto &frameData : m_frameData) {
            int pipelineIndex = std::find(frameData.pipelineIDs.begin(), frameData.pipelineIDs.end(), command->m_pipelineDesc->uuid) - frameData.pipelineIDs.begin();
            descriptorWrites.emplace_back(frameData.descriptorSets[pipelineIndex][command->m_dstSet], command->m_dstBinding, 0, 1, vk::DescriptorType::eStorageBuffer, nullptr, &TempBufferInfos.back());
        }
    } else {
        // todo: default texture
        bool isStorage = static_cast<RHI::TextureDesc *>(command->m_resourceDesc)->isStorage;
        vk::ImageLayout imageLayout = isStorage ? vk::ImageLayout::eGeneral : vk::ImageLayout::eShaderReadOnlyOptimal;
        TempImageInfos.emplace_back(nullptr, m_textureRegistry[command->m_resourceDesc->uuid].imageView, imageLayout); // we dont support combinedImageSampler
        for (auto &frameData : m_frameData) {
            int pipelineIndex = std::find(frameData.pipelineIDs.begin(), frameData.pipelineIDs.end(), command->m_pipelineDesc->uuid) - frameData.pipelineIDs.begin();
            vk::DescriptorType descriptorType = isStorage ? vk::DescriptorType::eStorageImage : vk::DescriptorType::eSampledImage;
            descriptorWrites.emplace_back(frameData.descriptorSets[pipelineIndex][command->m_dstSet], command->m_dstBinding, 0, 1, descriptorType, &TempImageInfos.back(), nullptr);
        }
    }
}

void Renderer::resolveUseSamplerCommand(RHI::UseSamplerCommand *command, std::vector<vk::WriteDescriptorSet> &descriptorWrites, std::list<vk::DescriptorImageInfo> &TempImageInfos) {
    // todo: default sampler
    createSampler(*command->m_samplerDesc);
    TempImageInfos.emplace_back(m_samplerRegistry[command->m_samplerDesc->uuid], nullptr); // we dont support combinedImageSampler
    for (auto &frameData : m_frameData) {
        int pipelineIndex = std::find(frameData.pipelineIDs.begin(), frameData.pipelineIDs.end(), command->m_pipelineDesc->uuid) - frameData.pipelineIDs.begin();
        descriptorWrites.emplace_back(frameData.descriptorSets[pipelineIndex][command->m_dstSet], command->m_dstBinding, 0, 1, vk::DescriptorType::eSampler, &TempImageInfos.back(), nullptr);
    }
}

void Renderer::bindPipelineCommand(RHI::CommandListIR &ir, RHI::BindPipelineCommand *command) {
    RHI::PipelineDesc *pipelineDesc = command->m_pipelineDesc;
    ir.context.lastBoundPipeline = pipelineDesc;
    ir.context.isPostProcess = pipelineDesc->isPostProcess;

    if (pipelineDesc->isCompute && ir.context.isRendering) {
        endRendering(ir);
    } else if (!pipelineDesc->isCompute && !ir.context.isRendering) {
        beginRendering(ir);
    }

    Pipeline &pipeline = m_pipelineRegistry[pipelineDesc->uuid];
    uint pipelineIndex = std::find(m_frameData[0].pipelineIDs.begin(), m_frameData[0].pipelineIDs.end(), pipelineDesc->uuid) - m_frameData[0].pipelineIDs.begin(); // all frameData have the same pipelineIDs

    uint offset = ir.finalParameters.size();
    RHI::storePtr(pipeline.layout, ir.finalParameters);
    ir.finalParameters.push_back(new RHI::CommandParameter((uint)pipeline.setLayouts.size()));
    ir.finalParameters.push_back(new RHI::CommandParameter(pipelineIndex));

    /* 0 : pipeline layout low
     * 1 : pipeline layout high
     * 2 : setlayoutSize
     * 3 : pipelineIndex
     */
    if (pipeline.setLayouts.size() > 0) {
        ir.offsets.push_back(offset);
        if (pipelineDesc->isCompute) {
            ir.enqueueCommand([](void *dstFrameData, RHI::CommandParameter **parameters) {
                // std::cout << "bindDescriptorSets\n";
                FrameData *frameData = (FrameData *)dstFrameData;
                frameData->commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, (VkPipelineLayout)RHI::loadPtr(0, parameters), 0, parameters[2]->u, frameData->descriptorSets[parameters[3]->u].data(), 0, nullptr);
            },
                              ir.context.isPostProcess);
        } else {
            ir.enqueueCommand([](void *dstFrameData, RHI::CommandParameter **parameters) {
                // std::cout << "bindDescriptorSets\n";
                FrameData *frameData = (FrameData *)dstFrameData;
                frameData->commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, (VkPipelineLayout)RHI::loadPtr(0, parameters), 0, parameters[2]->u, frameData->descriptorSets[parameters[3]->u].data(), 0, nullptr);
            },
                              ir.context.isPostProcess);
        }
    }

    if (pipelineDesc->getPushConstants() != nullptr) {
        ir.finalParameters.push_back(new RHI::CommandParameter((uint32_t)pipelineDesc->getPushConstants()->stages));
        ir.finalParameters.push_back(new RHI::CommandParameter(pipelineDesc->getPushConstants()->size));
        if (command->m_pushConstantsData.size() > 0) {
            /*
             * 4 : pushConstants mask
             * 5 : pushConstants size
             * 6 - N : pushConstants data
             */
            ir.offsets.push_back(offset);
            ir.enqueueCommand([](void *dstFrameData, RHI::CommandParameter **parameters) {
                // std::cout << "pushConstants\n";
                uint32 size = parameters[5]->u;
                std::vector<byte> pushCst = std::vector<byte>(size);
                RHI::loadBytes(6, parameters, pushCst.data(), size);
                FrameData *frameData = (FrameData *)dstFrameData;
                frameData->commandBuffer.pushConstants((VkPipelineLayout)RHI::loadPtr(0, parameters), (vk::ShaderStageFlags)parameters[4]->u, 0, parameters[5]->u, pushCst.data());
            },
                              ir.context.isPostProcess);

            RHI::storeBytes(command->m_pushConstantsData.data(), pipelineDesc->getPushConstants()->size, ir.finalParameters);
        } else {
            RHI::storePtr(pipeline.pushConstantsData.data(), ir.finalParameters);
            /*
             * 4 : pushConstants mask
             * 5 : pushConstants size
             * 6 : pushConstants low
             * 7 : pushConstants high
             */
            ir.offsets.push_back(offset);
            ir.enqueueCommand([](void *dstFrameData, RHI::CommandParameter **parameters) {
                // std::cout << "pushConstants\n";
                FrameData *frameData = (FrameData *)dstFrameData;
                frameData->commandBuffer.pushConstants((VkPipelineLayout)RHI::loadPtr(0, parameters), (vk::ShaderStageFlags)parameters[4]->u, 0, parameters[5]->u, RHI::loadPtr(6, parameters));
            },
                              ir.context.isPostProcess);
        }
    }

    // 0 : pipeline low
    // 1 : pipeline high
    ir.offsets.push_back(ir.finalParameters.size());
    RHI::storePtr(pipeline.pipeline, ir.finalParameters);
    if (pipelineDesc->isCompute) {
        ir.enqueueCommand([](void *dstFrameData, RHI::CommandParameter **parameters) {
            // std::cout << "bindPipeline\n";
            FrameData *frameData = (FrameData *)dstFrameData;
            frameData->commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, (VkPipeline)RHI::loadPtr(0, parameters));
        },
                          ir.context.isPostProcess);
    } else {
        ir.enqueueCommand([](void *dstFrameData, RHI::CommandParameter **parameters) {
            // std::cout << "bindPipeline\n";
            FrameData *frameData = (FrameData *)dstFrameData;
            frameData->commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, (VkPipeline)RHI::loadPtr(0, parameters));
        },
                          ir.context.isPostProcess);
    }
}

void Renderer::resolveUseVertexBuffer(RHI::CommandListIR &ir, RHI::UseVertexBufferCommand *useVBCmd) {
    vk::Buffer buffer = m_bufferRegistry[useVBCmd->m_bufferDesc->uuid].buffer;
    ir.offsets.push_back(ir.finalParameters.size());
    ir.finalParameters.push_back(new RHI::CommandParameter(useVBCmd->m_binding));
    RHI::storePtr(buffer, ir.finalParameters);
    ir.finalParameters.push_back(new RHI::CommandParameter(useVBCmd->m_offset));
    ir.enqueueCommand([](void *dstFrameData, RHI::CommandParameter **parameters) {
        // std::cout << "bindVertexBuffers\n";
        FrameData *frameData = (FrameData *)dstFrameData;
        vk::Buffer buffer = (VkBuffer)RHI::loadPtr(1, parameters);
        vk::DeviceSize offset = parameters[3]->u;
        frameData->commandBuffer.bindVertexBuffers(parameters[0]->u, 1u, &buffer, &offset);
    },
                      ir.context.isPostProcess);
}

void Renderer::resolveUseIndexBuffer(RHI::CommandListIR &ir, RHI::UseIndexBufferCommand *useIBCmd) {
    vk::Buffer buffer = m_bufferRegistry[useIBCmd->m_bufferDesc->uuid].buffer;
    ir.offsets.push_back(ir.finalParameters.size());
    RHI::storePtr(buffer, ir.finalParameters);
    ir.finalParameters.push_back(new RHI::CommandParameter(useIBCmd->m_offset));
    ir.finalParameters.push_back(new RHI::CommandParameter((uint)(useIBCmd->m_indexType)));
    ir.enqueueCommand([](void *dstFrameData, RHI::CommandParameter **parameters) {
        // std::cout << "bindIndexBuffer\n";
        FrameData *frameData = (FrameData *)dstFrameData;
        frameData->commandBuffer.bindIndexBuffer((VkBuffer)RHI::loadPtr(0, parameters), (VkDeviceSize)parameters[2]->u, (vk::IndexType)parameters[3]->u);
    },
                      ir.context.isPostProcess);
}

void Renderer::resolveCommand(RHI::CommandListIR &ir, RHI::RHICommand *command) {
    RHI::PipelineDesc *pipeline = ir.context.lastBoundPipeline;
    bool timing = pipeline->hasTiming;
    bool instrumentation = pipeline->hasFineInstrumentation;
    uint timingOffset;
    uint instrumentOffset;
    if (instrumentation) {
        instrumentOffset = m_perfCounterGlobals.generatePerfCounterCLParamsWMeshShader(m_pipelineRegistry[pipeline->uuid], ir.finalParameters);
        ir.offsets.push_back(instrumentOffset);
        ir.enqueueCommand([](void *dstFrameData, RHI::CommandParameter **parameters) {
            // std::cout << "beginQuery\n";
            FrameData *frameData = (FrameData *)dstFrameData;
            // frameData->commandBuffer.beginQuery(frameData->perfCounters.pipelineStatisticsQueryPool, parameters[0]->u, {}); // pipeline statistics
            frameData->commandBuffer.beginQuery(frameData->perfCounters.meshPrimitivesQueryPool, parameters[1]->u, {}); // mesh primitives
        },
                          ir.context.isPostProcess);
    }
    if (timing) {
        timingOffset = m_perfCounterGlobals.generateTimestampCLParams(m_pipelineRegistry[pipeline->uuid], ir.finalParameters);
        ir.offsets.push_back(timingOffset);
        ir.enqueueCommand([](void *dstFrameData, RHI::CommandParameter **parameters) {
            // std::cout << "writeTimestamp\n";
            FrameData *frameData = (FrameData *)dstFrameData;
            frameData->commandBuffer.writeTimestamp(vk::PipelineStageFlagBits::eTopOfPipe, frameData->perfCounters.timestampQueryPool, parameters[0]->u); // timestamp start
        },
                          ir.context.isPostProcess);
    }
    if (command->dynamicParamCount > 0) {
        m_intermediateCommandList.offsets.push_back(m_intermediateCommandList.finalParameters.size());
        for (int i = 0; i < command->dynamicParamCount; i++) {
            m_intermediateCommandList.finalParameters.push_back(&m_intermediateCommandList.inputParameters[command->dynamicParamOffset + i]);
        }
    }
    switch (command->type) {
    case RHI::CommandType::DispatchMesh: {
        ir.enqueueCommand([](void *dstFrameData, RHI::CommandParameter **parameters) {
            // std::cout << "drawMeshTasksEXT\n";
            FrameData *frameData = (FrameData *)dstFrameData;
            frameData->commandBuffer.drawMeshTasksEXT(parameters[0]->u, parameters[1]->u, parameters[2]->u);
        },
                          ir.context.isPostProcess);
        break;
    }
    case RHI::CommandType::DispatchMeshIndirect: {
        RHI::IndirectMeshCommand *meshCommand = static_cast<RHI::IndirectMeshCommand *>(command);
        vk::Buffer buffer = m_bufferRegistry[meshCommand->m_bufferDesc->uuid].buffer;

        ir.offsets.push_back(ir.finalParameters.size());
        RHI::storePtr(buffer, ir.finalParameters);
        ir.finalParameters.push_back(new RHI::CommandParameter(meshCommand->m_offset));
        ir.finalParameters.push_back(new RHI::CommandParameter(meshCommand->m_offset));

        ir.enqueueCommand([](void *dstFrameData, RHI::CommandParameter **parameters) {
            // std::cout << "drawMeshTasksEXT\n";
            FrameData *frameData = (FrameData *)dstFrameData;
            frameData->commandBuffer.drawMeshTasksIndirectEXT((VkBuffer)RHI::loadPtr(0, parameters), (VkDeviceSize)parameters[2]->u, parameters[3]->u, sizeof(VkDrawMeshTasksIndirectCommandEXT));
        },
                          ir.context.isPostProcess);
        break;
        break;
    }
    case RHI::CommandType::Draw: {
        ir.enqueueCommand([](void *dstFrameData, RHI::CommandParameter **parameters) {
            // std::cout << "draw\n";
            FrameData *frameData = (FrameData *)dstFrameData;
            frameData->commandBuffer.draw(parameters[0]->u, parameters[1]->u, parameters[2]->u, parameters[3]->u);
        },
                          ir.context.isPostProcess);
        break;
    }
    case RHI::CommandType::DrawIndexed: {
        ir.enqueueCommand([](void *dstFrameData, RHI::CommandParameter **parameters) {
            // std::cout << "drawIndexed\n";
            FrameData *frameData = (FrameData *)dstFrameData;
            frameData->commandBuffer.drawIndexed(parameters[0]->u, parameters[1]->u, parameters[2]->u, parameters[3]->u, parameters[4]->u);
        },
                          ir.context.isPostProcess);
        break;
    }
    case RHI::CommandType::DrawIndirect: {
        RHI::IndirectDrawCommand *drawCommand = static_cast<RHI::IndirectDrawCommand *>(command);
        vk::Buffer buffer = m_bufferRegistry[drawCommand->m_bufferDesc->uuid].buffer;

        ir.offsets.push_back(ir.finalParameters.size());
        RHI::storePtr(buffer, ir.finalParameters);
        ir.finalParameters.push_back(new RHI::CommandParameter(drawCommand->m_offset));
        ir.finalParameters.push_back(new RHI::CommandParameter(drawCommand->m_drawCount));
        ir.finalParameters.push_back(new RHI::CommandParameter(drawCommand->m_stride));

        ir.enqueueCommand([](void *dstFrameData, RHI::CommandParameter **parameters) {
            // std::cout << "drawIndirect\n";
            FrameData *frameData = (FrameData *)dstFrameData;
            frameData->commandBuffer.drawIndirect((VkBuffer)RHI::loadPtr(0, parameters), (VkDeviceSize)parameters[2]->u, parameters[3]->u, parameters[4]->u);
        },
                          ir.context.isPostProcess);
        break;
    }
    case RHI::CommandType::DrawIndexedIndirect: {
        RHI::IndirectDrawCommand *drawCommand = static_cast<RHI::IndirectDrawCommand *>(command);
        vk::Buffer buffer = m_bufferRegistry[drawCommand->m_bufferDesc->uuid].buffer;

        ir.offsets.push_back(ir.finalParameters.size());
        RHI::storePtr(buffer, ir.finalParameters);
        ir.finalParameters.push_back(new RHI::CommandParameter(drawCommand->m_offset));
        ir.finalParameters.push_back(new RHI::CommandParameter(drawCommand->m_drawCount));
        ir.finalParameters.push_back(new RHI::CommandParameter(drawCommand->m_stride));

        ir.enqueueCommand([](void *dstFrameData, RHI::CommandParameter **parameters) {
            // std::cout << "drawIndexedIndirect\n";
            FrameData *frameData = (FrameData *)dstFrameData;
            frameData->commandBuffer.drawIndexedIndirect((VkBuffer)RHI::loadPtr(0, parameters), (VkDeviceSize)parameters[2]->u, parameters[3]->u, parameters[4]->u);
        },
                          ir.context.isPostProcess);
        break;
    }
    case RHI::CommandType::Dispatch: {
        ir.enqueueCommand([](void *dstFrameData, RHI::CommandParameter **parameters) {
            // std::cout << "dispatch\n";
            FrameData *frameData = (FrameData *)dstFrameData;
            frameData->commandBuffer.dispatch(parameters[0]->u, parameters[1]->u, parameters[2]->u);
        },
                          ir.context.isPostProcess);
        break;
    }
    case RHI::CommandType::DispatchIndirect: {
        RHI::IndirectCommand *indirectCommand = static_cast<RHI::IndirectCommand *>(command);
        vk::Buffer buffer = m_bufferRegistry[indirectCommand->m_bufferDesc->uuid].buffer;

        ir.offsets.push_back(ir.finalParameters.size());
        RHI::storePtr(buffer, ir.finalParameters);
        ir.finalParameters.push_back(new RHI::CommandParameter(indirectCommand->m_offset));

        ir.enqueueCommand([](void *dstFrameData, RHI::CommandParameter **parameters) {
            // std::cout << "drawMeshTasksEXT\n";
            FrameData *frameData = (FrameData *)dstFrameData;
            frameData->commandBuffer.dispatchIndirect((VkBuffer)RHI::loadPtr(0, parameters), (VkDeviceSize)parameters[2]->u);
        },
                          ir.context.isPostProcess);
        break;
    }

    case RHI::CommandType::UseResource:
    case RHI::CommandType::BindPipeline:
    case RHI::CommandType::UseVertexBuffer:
    case RHI::CommandType::UseIndexBuffer:
        break;
    default:
        throw std::runtime_error("Unknown command type");
    }
    if (timing) {
        ir.offsets.push_back(timingOffset);
        ir.enqueueCommand([](void *dstFrameData, RHI::CommandParameter **parameters) {
            // std::cout << "writeTimestamp\n";
            FrameData *frameData = (FrameData *)dstFrameData;
            frameData->commandBuffer.writeTimestamp(vk::PipelineStageFlagBits::eBottomOfPipe, frameData->perfCounters.timestampQueryPool, parameters[0]->u + 1); // timestamp end
        },
                          ir.context.isPostProcess);
    }
    if (instrumentation) {
        ir.offsets.push_back(instrumentOffset);
        ir.enqueueCommand([](void *dstFrameData, RHI::CommandParameter **parameters) {
            // std::cout << "endQuery\n";
            FrameData *frameData = (FrameData *)dstFrameData;
            frameData->commandBuffer.endQuery(frameData->perfCounters.meshPrimitivesQueryPool, parameters[1]->u); // mesh primitives
        },
                          ir.context.isPostProcess);
        // frameData->commandBuffer.endQuery(frameData->perfCounters.pipelineStatisticsQueryPool, parameters[0]->u); // pipeline statistics
    }
}

// big fat dumb barrier for now,
void Renderer::resolveBarrierCommand(RHI::CommandListIR &ir, RHI::RHICommand *command) {
    if (ir.context.isRendering) {
        endRendering(ir);
    }
    ir.offsets.push_back(ir.finalParameters.size());
    ir.enqueueCommand([](void *dstFrameData, RHI::CommandParameter **parameters) {
        // std::cout << "barrier\n";
        FrameData *frameData = (FrameData *)dstFrameData;

        vk::MemoryBarrier2 barrier = {vk::PipelineStageFlagBits2::eAllCommands, vk::AccessFlagBits2::eMemoryWrite, vk::PipelineStageFlagBits2::eAllCommands, vk::AccessFlagBits2::eMemoryRead | vk::AccessFlagBits2::eMemoryWrite};
        frameData->commandBuffer.pipelineBarrier2({{}, 1, &barrier, {}, {}, {}, {}});
    },
                      ir.context.isPostProcess);

    // disabled for now as entering the context forces a clear
    // bindPipelineCommand(ir, ir.context.lastBoundPipeline);
}

void Renderer::beginRendering(RHI::CommandListIR &ir) {
    ir.offsets.push_back(ir.finalParameters.size());
    ir.enqueueCommand([](void *dstFrameData, RHI::CommandParameter **parameters) {
        // std::cout << "beginRendering\n";
        FrameData *frameData = (FrameData *)dstFrameData;
        vk::ClearValue clearColor{vk::ClearColorValue{std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f}}};
        vk::ClearValue clearDepth{vk::ClearDepthStencilValue{1.0f, 0}};
        vk::RenderingAttachmentInfo colorAttachmentInfoMain{frameData->swapchainImage.imageView, vk::ImageLayout::eColorAttachmentOptimal, {}, {}, {}, vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore, clearColor};
        vk::RenderingAttachmentInfo depthAttachmentInfo{frameData->depth.imageView, vk::ImageLayout::eDepthStencilAttachmentOptimal, {}, {}, {}, vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore, clearDepth};

        vk::RenderingInfo renderingInfoMain{{}, vk::Rect2D({0, 0}, {static_cast<uint>(frameData->swapchainImage.size.x), static_cast<uint>(frameData->swapchainImage.size.y)}), 1, {}, 1, &colorAttachmentInfoMain, &depthAttachmentInfo};

        frameData->commandBuffer.beginRendering(&renderingInfoMain);
    },
                      ir.context.isPostProcess);
    ir.context.isRendering = true;
}
void Renderer::endRendering(RHI::CommandListIR &ir) {
    ir.offsets.push_back(ir.finalParameters.size());
    ir.enqueueCommand([](void *dstFrameData, RHI::CommandParameter **parameters) {
        // std::cout << "endRendering\n";
        FrameData *frameData = (FrameData *)dstFrameData;
        frameData->commandBuffer.endRendering();
    },
                      ir.context.isPostProcess);
    ir.context.isRendering = false;
}
} // namespace VKRenderer

