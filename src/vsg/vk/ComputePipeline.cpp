/* <editor-fold desc="MIT License">

Copyright(c) 2018 Robert Osfield

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

</editor-fold> */

#include <vsg/traversals/CompileTraversal.h>

#include <vsg/vk/CommandBuffer.h>
#include <vsg/vk/ComputePipeline.h>

using namespace vsg;

////////////////////////////////////////////////////////////////////////
//
// ComputePipeline
//
ComputePipeline::ComputePipeline()
{
}

ComputePipeline::ComputePipeline(PipelineLayout* pipelineLayout, ShaderStage* shaderStage, AllocationCallbacks* allocator) :
    _pipelineLayout(pipelineLayout),
    _shaderStage(shaderStage),
    _allocator(allocator)
{
}

ComputePipeline::~ComputePipeline()
{
}

void ComputePipeline::read(Input& input)
{
    Object::read(input);

    _pipelineLayout = input.readObject<PipelineLayout>("PipelineLayout");
    _shaderStage = input.readObject<ShaderStage>("ShaderStage");
}

void ComputePipeline::write(Output& output) const
{
    Object::write(output);

    output.writeObject("PipelineLayout", _pipelineLayout.get());
    output.writeObject("ShaderStage", _shaderStage.get());
}

void ComputePipeline::compile(Context& context)
{
    if (!_implementation)
    {
        _pipelineLayout->compile(context);
        _shaderStage->compile(context);
        _implementation = ComputePipeline::Implementation::create(context.device, _pipelineLayout, _shaderStage, _allocator);
    }
}

////////////////////////////////////////////////////////////////////////
//
// ComputePipeline::Implementation
//
ComputePipeline::Implementation::Implementation(VkPipeline pipeline, Device* device, PipelineLayout* pipelineLayout, ShaderStage* shaderStage, AllocationCallbacks* allocator) :
    _pipeline(pipeline),
    _device(device),
    _pipelineLayout(pipelineLayout),
    _shaderStage(shaderStage),
    _allocator(allocator)
{
}

ComputePipeline::Implementation::~Implementation()
{
    vkDestroyPipeline(*_device, _pipeline, _allocator);
}

ComputePipeline::Implementation::Result ComputePipeline::Implementation::create(Device* device, PipelineLayout* pipelineLayout, ShaderStage* shaderStage, AllocationCallbacks* allocator)
{
    if (!device || !pipelineLayout || !shaderStage)
    {
        return Result("Error: vsg::ComputePipeline::create(...) failed to create compute pipeline, undefined device, pipelinLayout or shaderStage.", VK_ERROR_INVALID_EXTERNAL_HANDLE);
    }

    VkSpecializationInfo specializationInfo = {};
    VkPipelineShaderStageCreateInfo stageInfo = {};
    stageInfo.pNext = nullptr;
    shaderStage->apply(stageInfo);

    if (!shaderStage->getSpecializationMapEntries().empty() && shaderStage->getSpecializationData()!=nullptr)
    {
        // assign a VkSpecializationInfo for this shaderStageCreateInfo
        stageInfo.pSpecializationInfo = &specializationInfo;

        // assign the values from the ShaderStage into the specializationInfo
        specializationInfo.mapEntryCount = shaderStage->getSpecializationMapEntries().size();
        specializationInfo.pMapEntries = shaderStage->getSpecializationMapEntries().data();
        specializationInfo.dataSize = shaderStage->getSpecializationData()->dataSize();
        specializationInfo.pData = shaderStage->getSpecializationData()->dataPointer();
    }

    VkComputePipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.layout = *pipelineLayout;
    pipelineInfo.stage = stageInfo;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.pNext = nullptr;

    VkPipeline pipeline;
    VkResult result = vkCreateComputePipelines(*device, VK_NULL_HANDLE, 1, &pipelineInfo, allocator, &pipeline);
    if (result == VK_SUCCESS)
    {
        return Result(new ComputePipeline::Implementation(pipeline, device, pipelineLayout, shaderStage, allocator));
    }
    else
    {
        return Result("Error: vsg::Pipeline::createCompute(...) failed to create VkPipeline.", result);
    }
}

////////////////////////////////////////////////////////////////////////
//
// BindComputePipeline
//
BindComputePipeline::BindComputePipeline(ComputePipeline* pipeline) :
    Inherit(0), // slot 0
    _pipeline(pipeline)
{
}

BindComputePipeline::~BindComputePipeline()
{
}

void BindComputePipeline::read(Input& input)
{
    StateCommand::read(input);

    _pipeline = input.readObject<ComputePipeline>("ComputePipeline");
}

void BindComputePipeline::write(Output& output) const
{
    StateCommand::write(output);

    output.writeObject("ComputePipeline", _pipeline.get());
}

void BindComputePipeline::dispatch(CommandBuffer& commandBuffer) const
{
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *_pipeline);
    commandBuffer.setCurrentPipelineLayout(*(_pipeline->getPipelineLayout()));
}

void BindComputePipeline::compile(Context& context)
{
    if (_pipeline) _pipeline->compile(context);
}
