/**
 * @file vk_pipeline.c
 * @brief Pipelines, Shaders e Command Buffers
 */

#include <stdlib.h>
#include "gui/rhi/vulkan/vk_internal.h"

/* ============================================================================
 * HELPERS DE CONVERSÃO
 * ============================================================================
 */

static VkPrimitiveTopology vk_primitive_from_bhs(enum bhs_gpu_primitive p)
{
	switch (p) {
	case BHS_PRIMITIVE_TRIANGLES:
		return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	case BHS_PRIMITIVE_TRIANGLE_STRIP:
		return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
	case BHS_PRIMITIVE_LINES:
		return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
	case BHS_PRIMITIVE_LINE_STRIP:
		return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
	case BHS_PRIMITIVE_POINTS:
		return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
	default:
		return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	}
}

static VkCullModeFlags vk_cull_mode_from_bhs(enum bhs_gpu_cull_mode c)
{
	switch (c) {
	case BHS_CULL_NONE:
		return VK_CULL_MODE_NONE;
	case BHS_CULL_FRONT:
		return VK_CULL_MODE_FRONT_BIT;
	case BHS_CULL_BACK:
		return VK_CULL_MODE_BACK_BIT;
	default:
		return VK_CULL_MODE_NONE;
	}
}

static VkBlendFactor vk_blend_factor_from_bhs(enum bhs_gpu_blend_factor f)
{
	switch (f) {
	case BHS_BLEND_ZERO:
		return VK_BLEND_FACTOR_ZERO;
	case BHS_BLEND_ONE:
		return VK_BLEND_FACTOR_ONE;
	case BHS_BLEND_SRC_ALPHA:
		return VK_BLEND_FACTOR_SRC_ALPHA;
	case BHS_BLEND_ONE_MINUS_SRC_ALPHA:
		return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	case BHS_BLEND_DST_ALPHA:
		return VK_BLEND_FACTOR_DST_ALPHA;
	case BHS_BLEND_ONE_MINUS_DST_ALPHA:
		return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
	default:
		return VK_BLEND_FACTOR_ONE;
	}
}

static VkBlendOp vk_blend_op_from_bhs(enum bhs_gpu_blend_op o)
{
	switch (o) {
	case BHS_BLEND_OP_ADD:
		return VK_BLEND_OP_ADD;
	case BHS_BLEND_OP_SUBTRACT:
		return VK_BLEND_OP_SUBTRACT;
	case BHS_BLEND_OP_MIN:
		return VK_BLEND_OP_MIN;
	case BHS_BLEND_OP_MAX:
		return VK_BLEND_OP_MAX;
	default:
		return VK_BLEND_OP_ADD;
	}
}

static VkCompareOp vk_compare_op_from_bhs(enum bhs_gpu_compare_func f)
{
	switch (f) {
	case BHS_COMPARE_NEVER:
		return VK_COMPARE_OP_NEVER;
	case BHS_COMPARE_LESS:
		return VK_COMPARE_OP_LESS;
	case BHS_COMPARE_EQUAL:
		return VK_COMPARE_OP_EQUAL;
	case BHS_COMPARE_LESS_EQUAL:
		return VK_COMPARE_OP_LESS_OR_EQUAL;
	case BHS_COMPARE_GREATER:
		return VK_COMPARE_OP_GREATER;
	case BHS_COMPARE_NOT_EQUAL:
		return VK_COMPARE_OP_NOT_EQUAL;
	case BHS_COMPARE_GREATER_EQUAL:
		return VK_COMPARE_OP_GREATER_OR_EQUAL;
	case BHS_COMPARE_ALWAYS:
		return VK_COMPARE_OP_ALWAYS;
	default:
		return VK_COMPARE_OP_LESS;
	}
}

/* ============================================================================
 * SHADERS
 * ============================================================================
 */

int bhs_gpu_shader_create(bhs_gpu_device_t device,
			  const struct bhs_gpu_shader_config *config,
			  bhs_gpu_shader_t *shader)
{
	if (!device || !config || !shader || !config->code)
		return BHS_GPU_ERR_INVALID;

	struct bhs_gpu_shader_impl *s = calloc(1, sizeof(*s));
	if (!s)
		return BHS_GPU_ERR_NOMEM;

	s->device = device;
	s->stage = config->stage;

	VkShaderModuleCreateInfo create_info = {
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = config->code_size,
		.pCode = config->code,
	};

	VkResult result = vkCreateShaderModule(device->device, &create_info,
					       NULL, &s->module);
	if (result != VK_SUCCESS) {
		free(s);
		return BHS_GPU_ERR_COMPILE;
	}

	*shader = s;
	return BHS_GPU_OK;
}

void bhs_gpu_shader_destroy(bhs_gpu_shader_t shader)
{
	if (!shader)
		return;
	vkDestroyShaderModule(shader->device->device, shader->module, NULL);
	free(shader);
}

/* ============================================================================
 * PIPELINES
 * ============================================================================
 */

void bhs_gpu_cmd_draw_indexed(bhs_gpu_cmd_buffer_t cmd, uint32_t index_count,
			      uint32_t instance_count, uint32_t first_index,
			      int32_t vertex_offset, uint32_t first_instance)
{
	if (!cmd)
		return;
	vkCmdDrawIndexed(cmd->cmd, index_count, instance_count, first_index,
			 vertex_offset, first_instance);
}

void bhs_gpu_cmd_dispatch(bhs_gpu_cmd_buffer_t cmd, uint32_t group_count_x,
			  uint32_t group_count_y, uint32_t group_count_z)
{
	if (!cmd)
		return;
	vkCmdDispatch(cmd->cmd, group_count_x, group_count_y, group_count_z);
}

int bhs_gpu_pipeline_create(bhs_gpu_device_t device,
			    const struct bhs_gpu_pipeline_config *config,
			    bhs_gpu_pipeline_t *pipeline)
{
	if (!device || !config || !pipeline)
		return BHS_GPU_ERR_INVALID;

	struct bhs_gpu_pipeline_impl *p = calloc(1, sizeof(*p));
	if (!p)
		return BHS_GPU_ERR_NOMEM;

	p->device = device;

	/* 1. Pipeline Layout */
	/* Definindo Push Constants range (128 bytes shared) para UI/Geral */
	VkPushConstantRange push_constant = {
		.stageFlags = VK_SHADER_STAGE_VERTEX_BIT |
			      VK_SHADER_STAGE_FRAGMENT_BIT,
		.offset = 0,
		.size = 128
	};

	VkPipelineLayoutCreateInfo layout_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = 0,
		.pushConstantRangeCount = 1,
		.pPushConstantRanges = &push_constant,
	};

	/* Add Texture Descriptor Set Layout de forma segura */
	if (device->texture_layout) {
		layout_info.setLayoutCount = 1;
		layout_info.pSetLayouts = &device->texture_layout;
	}

	if (vkCreatePipelineLayout(device->device, &layout_info, NULL,
				   &p->layout) != VK_SUCCESS) {
		free(p);
		return BHS_GPU_ERR_DEVICE;
	}

	/* 2. Render Pass Temporário (para criação do pipeline) */
	/* NOTA: Vulkan exige um RenderPass compatível na criação.
     Criamos um dummy aqui que bate com a configuração esperada. */
	VkAttachmentDescription attachments[9]; /* 8 color + 1 depth */
	VkAttachmentReference color_refs[8];
	uint32_t att_count = 0;

	for (uint32_t i = 0; i < config->color_format_count && i < 8; i++) {
		attachments[att_count] = (VkAttachmentDescription){
			.format = bhs_vk_format(config->color_formats[i]),
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout = VK_IMAGE_LAYOUT_GENERAL,
		};

		color_refs[i].attachment = att_count;
		color_refs[i].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		att_count++;
	}

	/* Depth Attachment (se necessário) */
	VkAttachmentReference depth_ref = { 0 };
	VkAttachmentReference *p_depth_ref = NULL;

	if (config->depth_test || config->depth_write) {
		attachments[att_count] = (VkAttachmentDescription){
			.format = bhs_vk_format(config->depth_format),
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout =
				VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		};

		depth_ref.attachment = att_count;
		depth_ref.layout =
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		p_depth_ref = &depth_ref;
		att_count++;
	}

	VkSubpassDescription subpass = {
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.colorAttachmentCount = config->color_format_count,
		.pColorAttachments = color_refs,
		.pDepthStencilAttachment = p_depth_ref,
	};

	VkRenderPassCreateInfo rp_info = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.attachmentCount = att_count,
		.pAttachments = attachments,
		.subpassCount = 1,
		.pSubpasses = &subpass,
	};

	VkRenderPass temp_rp;
	if (vkCreateRenderPass(device->device, &rp_info, NULL, &temp_rp) !=
	    VK_SUCCESS) {
		vkDestroyPipelineLayout(device->device, p->layout, NULL);
		free(p);
		return BHS_GPU_ERR_DEVICE;
	}

	/* 3. Shader Stages */
	VkPipelineShaderStageCreateInfo stages[2];
	stages[0] = (VkPipelineShaderStageCreateInfo){
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = VK_SHADER_STAGE_VERTEX_BIT,
		.module = config->vertex_shader->module,
		.pName = "main",
	};
	stages[1] = (VkPipelineShaderStageCreateInfo){
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
		.module = config->fragment_shader->module,
		.pName = "main",
	};

	/* 4. Vertex Input */
	VkVertexInputBindingDescription bindings[8];
	for (uint32_t i = 0; i < config->vertex_binding_count && i < 8; i++) {
		bindings[i].binding = config->vertex_bindings[i].binding;
		bindings[i].stride = config->vertex_bindings[i].stride;
		bindings[i].inputRate = config->vertex_bindings[i].per_instance
						? VK_VERTEX_INPUT_RATE_INSTANCE
						: VK_VERTEX_INPUT_RATE_VERTEX;
	}

	VkVertexInputAttributeDescription attrs[16];
	for (uint32_t i = 0; i < config->vertex_attr_count && i < 16; i++) {
		attrs[i].location = config->vertex_attrs[i].location;
		attrs[i].binding = config->vertex_attrs[i].binding;
		attrs[i].format = bhs_vk_format(config->vertex_attrs[i].format);
		attrs[i].offset = config->vertex_attrs[i].offset;
	}

	VkPipelineVertexInputStateCreateInfo vertex_input = {
		.sType =
			VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.vertexBindingDescriptionCount = config->vertex_binding_count,
		.pVertexBindingDescriptions = bindings,
		.vertexAttributeDescriptionCount = config->vertex_attr_count,
		.pVertexAttributeDescriptions = attrs,
	};

	/* 5. Input Assembly */
	VkPipelineInputAssemblyStateCreateInfo input_assembly = {
		.sType =
			VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.topology = vk_primitive_from_bhs(config->primitive),
		.primitiveRestartEnable = VK_FALSE,
	};

	/* 6. Viewport (Dynamic) */
	VkPipelineViewportStateCreateInfo viewport_state = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.viewportCount = 1,
		.scissorCount = 1,
	};

	/* 7. Rasterizer */
	VkPipelineRasterizationStateCreateInfo rasterizer = {
		.sType =
			VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.depthClampEnable = VK_FALSE,
		.rasterizerDiscardEnable = VK_FALSE,
		.polygonMode = VK_POLYGON_MODE_FILL,
		.lineWidth = 1.0f,
		.cullMode = vk_cull_mode_from_bhs(config->cull_mode),
		.frontFace = config->front_ccw ? VK_FRONT_FACE_COUNTER_CLOCKWISE
					       : VK_FRONT_FACE_CLOCKWISE,
		.depthBiasEnable = VK_FALSE,
	};

	/* 8. Multisample */
	VkPipelineMultisampleStateCreateInfo multisampling = {
		.sType =
			VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.sampleShadingEnable = VK_FALSE,
		.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
	};

	/* 9. Color Blend */
	VkPipelineColorBlendAttachmentState color_blend_att = {
		.colorWriteMask =
			VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
			VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
		.blendEnable = VK_TRUE, /* Default habilitado para UI */
		.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
		.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
		.colorBlendOp = VK_BLEND_OP_ADD,
		.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
		.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
		.alphaBlendOp = VK_BLEND_OP_ADD,
	};

	if (config->blend_state_count > 0) {
		const struct bhs_gpu_blend_state *bs = &config->blend_states[0];
		color_blend_att.blendEnable = bs->enabled;
		color_blend_att.srcColorBlendFactor =
			vk_blend_factor_from_bhs(bs->src_color);
		color_blend_att.dstColorBlendFactor =
			vk_blend_factor_from_bhs(bs->dst_color);
		color_blend_att.colorBlendOp =
			vk_blend_op_from_bhs(bs->color_op);
		color_blend_att.srcAlphaBlendFactor =
			vk_blend_factor_from_bhs(bs->src_alpha);
		color_blend_att.dstAlphaBlendFactor =
			vk_blend_factor_from_bhs(bs->dst_alpha);
		color_blend_att.alphaBlendOp =
			vk_blend_op_from_bhs(bs->alpha_op);
	}

	VkPipelineColorBlendStateCreateInfo color_blending = {
		.sType =
			VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.logicOpEnable = VK_FALSE,
		.attachmentCount = 1,
		.pAttachments = &color_blend_att,
	};

	/* 10. Dynamic State */
	VkDynamicState dynamic_states[] = { VK_DYNAMIC_STATE_VIEWPORT,
					    VK_DYNAMIC_STATE_SCISSOR };
	VkPipelineDynamicStateCreateInfo dynamic_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.dynamicStateCount = 2,
		.pDynamicStates = dynamic_states,
	};

	/* 8. Depth Stencil */
	VkPipelineDepthStencilStateCreateInfo depth_stencil = {
		.sType =
			VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		.depthTestEnable = config->depth_test ? VK_TRUE : VK_FALSE,
		.depthWriteEnable = config->depth_write ? VK_TRUE : VK_FALSE,
		.depthCompareOp = vk_compare_op_from_bhs(config->depth_compare),
		.depthBoundsTestEnable = VK_FALSE,
		.stencilTestEnable = VK_FALSE,
	};

	/* Create! */
	VkGraphicsPipelineCreateInfo pipeline_info = {
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.stageCount = 2,
		.pStages = stages,
		.pVertexInputState = &vertex_input,
		.pInputAssemblyState = &input_assembly,
		.pViewportState = &viewport_state,
		.pRasterizationState = &rasterizer,
		.pMultisampleState = &multisampling,
		.pDepthStencilState = &depth_stencil,
		.pColorBlendState = &color_blending,
		.pDynamicState = &dynamic_info,
		.layout = p->layout,
		.renderPass = temp_rp,
		.subpass = 0,
	};

	if (vkCreateGraphicsPipelines(device->device, VK_NULL_HANDLE, 1,
				      &pipeline_info, NULL,
				      &p->pipeline) != VK_SUCCESS) {
		vkDestroyRenderPass(device->device, temp_rp, NULL);
		vkDestroyPipelineLayout(device->device, p->layout, NULL);
		free(p);
		return BHS_GPU_ERR_DEVICE;
	}

	vkDestroyRenderPass(device->device, temp_rp, NULL);
	*pipeline = p;
	return BHS_GPU_OK;
}

int bhs_gpu_pipeline_compute_create(
	bhs_gpu_device_t device,
	const struct bhs_gpu_compute_pipeline_config *config,
	bhs_gpu_pipeline_t *pipeline)
{
	if (!device || !config || !pipeline)
		return BHS_GPU_ERR_INVALID;

	struct bhs_gpu_device_impl *dev = device;
	struct bhs_gpu_pipeline_impl *pipe = calloc(1, sizeof(*pipe));
	if (!pipe)
		return BHS_GPU_ERR_NOMEM;

	pipe->device = dev;
	pipe->bind_point = VK_PIPELINE_BIND_POINT_COMPUTE;

	/* Setup Descriptor Set Layout para Storage Image (Binding 0) */
	/* Isso é crítico para o Compute Shader escrever na textura */
	VkDescriptorSetLayoutBinding binding = {
		.binding = 0,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
		.pImmutableSamplers = NULL,
	};

	VkDescriptorSetLayoutCreateInfo set_info = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.bindingCount = 1,
		.pBindings = &binding,
	};

	if (vkCreateDescriptorSetLayout(dev->device, &set_info, NULL,
					&pipe->set_layout) != VK_SUCCESS) {
		free(pipe);
		return BHS_GPU_ERR_DEVICE;
	}

	/* Setup Push Constants (128 bytes para params) */
	/* Definimos todos os estágios para compatibilidade total com a API
   * bhs_gpu_cmd_push_constants */
	VkPushConstantRange push_constant = {
		.stageFlags = VK_SHADER_STAGE_VERTEX_BIT |
			      VK_SHADER_STAGE_FRAGMENT_BIT |
			      VK_SHADER_STAGE_COMPUTE_BIT,
		.offset = 0,
		.size = 128,
	};

	VkPipelineLayoutCreateInfo layout_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = 1,
		.pSetLayouts = &pipe->set_layout,
		.pushConstantRangeCount = 1,
		.pPushConstantRanges = &push_constant,
	};

	if (vkCreatePipelineLayout(dev->device, &layout_info, NULL,
				   &pipe->layout) != VK_SUCCESS) {
		vkDestroyDescriptorSetLayout(dev->device, pipe->set_layout,
					     NULL);
		free(pipe);
		return BHS_GPU_ERR_DEVICE;
	}

	/* Shader Stage */
	struct bhs_gpu_shader_impl *shader = config->compute_shader;
	if (!shader) {
		vkDestroyPipelineLayout(dev->device, pipe->layout, NULL);
		vkDestroyDescriptorSetLayout(dev->device, pipe->set_layout,
					     NULL);
		free(pipe);
		return BHS_GPU_ERR_INVALID;
	}

	VkPipelineShaderStageCreateInfo stage_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = VK_SHADER_STAGE_COMPUTE_BIT,
		.module = shader->module,
		.pName = "main",
	};

	/* Create Compute Pipeline */
	VkComputePipelineCreateInfo pipeline_info = {
		.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
		.stage = stage_info,
		.layout = pipe->layout,
	};

	if (vkCreateComputePipelines(dev->device, VK_NULL_HANDLE, 1,
				     &pipeline_info, NULL,
				     &pipe->pipeline) != VK_SUCCESS) {
		vkDestroyPipelineLayout(dev->device, pipe->layout, NULL);
		vkDestroyDescriptorSetLayout(dev->device, pipe->set_layout,
					     NULL);
		free(pipe);
		return BHS_GPU_ERR_DEVICE;
	}

	*pipeline = pipe;
	return BHS_GPU_OK;
}

void bhs_gpu_pipeline_destroy(bhs_gpu_pipeline_t pipeline)
{
	if (!pipeline)
		return;

	if (pipeline->pipeline)
		vkDestroyPipeline(pipeline->device->device, pipeline->pipeline,
				  NULL);
	if (pipeline->layout)
		vkDestroyPipelineLayout(pipeline->device->device,
					pipeline->layout, NULL);
	if (pipeline->set_layout)
		vkDestroyDescriptorSetLayout(pipeline->device->device,
					     pipeline->set_layout, NULL);

	/* Nota: não destruímos render_pass aqui pq ele não é owned pelo pipeline
     geralmente, exceto em stubs antigos. Como removemos o temp_rp do struct, tá
     limpo. */

	free(pipeline);
}

/* ============================================================================
 * COMMAND BUFFERS
 * ============================================================================
 */

int bhs_gpu_cmd_buffer_create(bhs_gpu_device_t device,
			      bhs_gpu_cmd_buffer_t *cmd)
{
	if (!device || !cmd)
		return BHS_GPU_ERR_INVALID;

	struct bhs_gpu_cmd_buffer_impl *c = calloc(1, sizeof(*c));
	if (!c)
		return BHS_GPU_ERR_NOMEM;

	c->device = device;

	VkCommandBufferAllocateInfo alloc_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = device->command_pool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1,
	};

	VkResult result =
		vkAllocateCommandBuffers(device->device, &alloc_info, &c->cmd);
	if (result != VK_SUCCESS) {
		free(c);
		return BHS_GPU_ERR_DEVICE;
	}

	/* Create Descriptor Pool for this cmd buffer */
	VkDescriptorPoolSize pool_sizes[] = {
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1024 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
		  1024 }, /* Added for Compute */
	};
	VkDescriptorPoolCreateInfo pool_info = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.poolSizeCount = 2, /* Updated count */
		.pPoolSizes = pool_sizes,
		.maxSets = 2048, /* Increased max sets */
	};
	vkCreateDescriptorPool(device->device, &pool_info, NULL,
			       &c->descriptor_pool);

	*cmd = c;
	return BHS_GPU_OK;
}

void bhs_gpu_cmd_buffer_destroy(bhs_gpu_cmd_buffer_t cmd)
{
	if (!cmd)
		return;
	if (cmd->descriptor_pool)
		vkDestroyDescriptorPool(cmd->device->device,
					cmd->descriptor_pool, NULL);
	vkFreeCommandBuffers(cmd->device->device, cmd->device->command_pool, 1,
			     &cmd->cmd);
	free(cmd);
}

void bhs_gpu_cmd_begin(bhs_gpu_cmd_buffer_t cmd)
{
	if (!cmd || cmd->recording)
		return;

	/* Reset pool on begin/reset */
	if (cmd->descriptor_pool)
		vkResetDescriptorPool(cmd->device->device, cmd->descriptor_pool,
				      0);

	VkCommandBufferBeginInfo begin_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	};
	vkBeginCommandBuffer(cmd->cmd, &begin_info);
	cmd->recording = true;
}

void bhs_gpu_cmd_end(bhs_gpu_cmd_buffer_t cmd)
{
	if (!cmd || !cmd->recording)
		return;
	vkEndCommandBuffer(cmd->cmd);
	cmd->recording = false;
}

void bhs_gpu_cmd_reset(bhs_gpu_cmd_buffer_t cmd)
{
	if (!cmd)
		return;

	if (cmd->descriptor_pool) {
		vkResetDescriptorPool(cmd->device->device, cmd->descriptor_pool,
				      0);
	}
	vkResetCommandBuffer(cmd->cmd, 0);
	cmd->recording = false;
}

void bhs_gpu_cmd_set_pipeline(bhs_gpu_cmd_buffer_t cmd,
			      bhs_gpu_pipeline_t pipeline)
{
	if (!cmd || !pipeline)
		return;
	vkCmdBindPipeline(cmd->cmd, pipeline->bind_point, pipeline->pipeline);
	cmd->current_pipeline_layout = pipeline->layout;
}

void bhs_gpu_cmd_set_viewport(bhs_gpu_cmd_buffer_t cmd, float x, float y,
			      float width, float height, float min_depth,
			      float max_depth)
{
	if (!cmd)
		return;
	VkViewport vp = { x, y, width, height, min_depth, max_depth };
	vkCmdSetViewport(cmd->cmd, 0, 1, &vp);
}

void bhs_gpu_cmd_set_scissor(bhs_gpu_cmd_buffer_t cmd, int32_t x, int32_t y,
			     uint32_t width, uint32_t height)
{
	if (!cmd)
		return;
	VkRect2D scissor = { { x, y }, { width, height } };
	vkCmdSetScissor(cmd->cmd, 0, 1, &scissor);
}

void bhs_gpu_cmd_set_vertex_buffer(bhs_gpu_cmd_buffer_t cmd, uint32_t binding,
				   bhs_gpu_buffer_t buffer, uint64_t offset)
{
	if (!cmd || !buffer)
		return;
	VkBuffer buf = buffer->buffer;
	VkDeviceSize off = offset;
	vkCmdBindVertexBuffers(cmd->cmd, binding, 1, &buf, &off);
}

void bhs_gpu_cmd_set_index_buffer(bhs_gpu_cmd_buffer_t cmd,
				  bhs_gpu_buffer_t buffer, uint64_t offset,
				  bool is_32bit)
{
	if (!cmd || !buffer)
		return;
	vkCmdBindIndexBuffer(cmd->cmd, buffer->buffer, offset,
			     is_32bit ? VK_INDEX_TYPE_UINT32
				      : VK_INDEX_TYPE_UINT16);
}

void bhs_gpu_cmd_draw(bhs_gpu_cmd_buffer_t cmd, uint32_t vertex_count,
		      uint32_t instance_count, uint32_t first_vertex,
		      uint32_t first_instance)
{
	if (!cmd)
		return;
	vkCmdDraw(cmd->cmd, vertex_count, instance_count, first_vertex,
		  first_instance);
}

void bhs_gpu_cmd_bind_texture(bhs_gpu_cmd_buffer_t cmd, uint32_t set,
			      uint32_t binding, bhs_gpu_texture_t texture,
			      bhs_gpu_sampler_t sampler)
{
	if (!cmd || !texture || !sampler || !cmd->recording)
		return;

	if (!cmd->descriptor_pool)
		return;
	if (cmd->current_pipeline_layout == VK_NULL_HANDLE) {
		BHS_VK_LOG("aviso: bind_texture chamado sem pipeline setado");
		return;
	}

	struct bhs_gpu_device_impl *dev = cmd->device;

	/* Aloca descriptor set novo */
	VkDescriptorSetAllocateInfo alloc_info = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool = cmd->descriptor_pool,
		.descriptorSetCount = 1,
		.pSetLayouts = &dev->texture_layout,
	};

	VkDescriptorSet desc_set;
	if (vkAllocateDescriptorSets(dev->device, &alloc_info, &desc_set) !=
	    VK_SUCCESS) {
		BHS_VK_LOG(
			"aviso: falha ao alocar descriptor set (pool cheio?)");
		return;
	}

	/* Atualiza set */
	struct bhs_gpu_texture_impl *tex = texture;
	struct bhs_gpu_sampler_impl *sam = sampler;

	VkDescriptorImageInfo image_info = {
		.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		.imageView = tex->view,
		.sampler = sam->sampler,
	};

	VkWriteDescriptorSet descriptor_write = {
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet = desc_set,
		.dstBinding = binding, /* Ignora parametro binding (assume 0) */
		.dstArrayElement = 0,
		.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.descriptorCount = 1,
		.pImageInfo = &image_info,
	};

	vkUpdateDescriptorSets(dev->device, 1, &descriptor_write, 0, NULL);

	/* Bind set */
	/* Assumindo Ponto de Bind Gráfico */
	vkCmdBindDescriptorSets(cmd->cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
				cmd->current_pipeline_layout, set, 1, &desc_set,
				0, NULL);
}

void bhs_gpu_cmd_push_constants(bhs_gpu_cmd_buffer_t cmd, uint32_t offset,
				const void *data, uint32_t size)
{
	if (!cmd || !data || !size || !cmd->recording ||
	    cmd->current_pipeline_layout == VK_NULL_HANDLE) {
		return;
	}

	/* Usamos a união de todos os estágios suportados pelo gui para evitar
   * erros de compatibilidade entre layouts */
	vkCmdPushConstants(cmd->cmd, cmd->current_pipeline_layout,
			   VK_SHADER_STAGE_VERTEX_BIT |
				   VK_SHADER_STAGE_FRAGMENT_BIT |
				   VK_SHADER_STAGE_COMPUTE_BIT,
			   offset, size, data);
}

void bhs_gpu_cmd_begin_render_pass(bhs_gpu_cmd_buffer_t cmd,
				   const struct bhs_gpu_render_pass *pass)
{
	if (!cmd || !pass || !cmd->recording)
		return;

	// Hack: pegando do swapchain global device
	// uint32_t width = cmd->device->swapchain->extent.width;
	// uint32_t height = cmd->device->swapchain->extent.height;

	/* HACK: Criando RenderPass e Framebuffer on-the-fly é terrível para
     performance. Para um driver "Linus Torvalds approved", isso deveria ser
     cacheado ou passado explicitamente. Mas para fazer funcionar AGORA, vamos
     criar um RP compatível.

     Idealmente: `bhs_gpu_render_pass` deveria ser um objeto opaco criado antes,
     não uma struct descritora. Dado o tempo, vamos usar o RenderPass padrão do
     Swapchain se disponível, ou falhar.
  */

	/* Validação rigorosa do alvo de renderização */
	VkRenderPass target_rp = VK_NULL_HANDLE;
	VkFramebuffer target_fb = VK_NULL_HANDLE;
	struct bhs_gpu_swapchain_impl *sc = cmd->device->swapchain;
	if (sc) {
		bool is_swapchain = false;
		for (uint32_t i = 0; i < sc->image_count; i++) {
			if (&sc->texture_wrappers[i] ==
			    pass->color_attachments[0].texture) {
				is_swapchain = true;
				target_rp = sc->render_pass;
				target_fb = sc->framebuffers[sc->current_image];
				break;
			}
		}

		if (!is_swapchain) {
			target_rp = sc->render_pass;
			target_fb = sc->framebuffers[sc->current_image];
		}
	}

	if (target_rp == VK_NULL_HANDLE || target_fb == VK_NULL_HANDLE) {
		return;
	}

	/* Clear Values: [0] = Cor, [1] = Depth */
	VkClearValue clear_values[2] = {
		{ .color = { { 0.1f, 0.1f, 0.1f, 1.0f } } },
		{ .depthStencil = { 1.0f, 0 } },
	};

	VkRenderPassBeginInfo rp_info = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass = target_rp,
		.framebuffer = sc->framebuffers[sc->current_image],
		.renderArea = { .offset = { 0, 0 }, .extent = sc->extent },
		.clearValueCount = 2,
		.pClearValues = clear_values,
	};

	vkCmdBeginRenderPass(cmd->cmd, &rp_info, VK_SUBPASS_CONTENTS_INLINE);
}

void bhs_gpu_cmd_end_render_pass(bhs_gpu_cmd_buffer_t cmd)
{
	if (!cmd || !cmd->recording)
		return;
	vkCmdEndRenderPass(cmd->cmd);
}

/* Implementation of external texture transition */
void bhs_gpu_cmd_transition_texture(bhs_gpu_cmd_buffer_t cmd,
				    bhs_gpu_texture_t texture)
{
	if (!cmd || !texture || !cmd->recording)
		return;

	struct bhs_gpu_texture_impl *tex = texture;

	VkImageMemoryBarrier barrier = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      .oldLayout = VK_IMAGE_LAYOUT_GENERAL, /* Compute output */
      .newLayout =
          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, /* Graphics input */
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .image = tex->image,
      .subresourceRange =
          {
              .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
              .baseMipLevel = 0,
              .levelCount = 1,
              .baseArrayLayer = 0,
              .layerCount = 1,
          },
      .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
      .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
  };

	vkCmdPipelineBarrier(cmd->cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			     VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL,
			     0, NULL, 1, &barrier);
}

/**
 * bhs_gpu_cmd_bind_compute_storage_texture - Binda storage image para compute
 */
void bhs_gpu_cmd_bind_compute_storage_texture(bhs_gpu_cmd_buffer_t cmd,
					      bhs_gpu_pipeline_t pipeline,
					      uint32_t set, uint32_t binding,
					      bhs_gpu_texture_t texture)
{
	if (!cmd || !pipeline || !texture || !cmd->recording)
		return;

	if (!cmd->descriptor_pool)
		return;
	if (!pipeline->set_layout) {
		BHS_VK_LOG(
			"erro: pipeline nao tem set_layout (nao eh compute?)");
		return;
	}

	struct bhs_gpu_device_impl *dev = cmd->device;

	/* Aloca Set */
	VkDescriptorSetAllocateInfo alloc_info = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool = cmd->descriptor_pool,
		.descriptorSetCount = 1,
		.pSetLayouts = &pipeline->set_layout,
	};

	VkDescriptorSet desc_set;
	if (vkAllocateDescriptorSets(dev->device, &alloc_info, &desc_set) !=
	    VK_SUCCESS) {
		BHS_VK_LOG("aviso: falha aloc set compute");
		return;
	}

	/* Atualiza (Storage Image) */
	struct bhs_gpu_texture_impl *tex = texture;

	VkDescriptorImageInfo image_info = {
		/* Compute escreve em GENERAL */
		.imageLayout = VK_IMAGE_LAYOUT_GENERAL,
		.imageView = tex->view,
		.sampler = VK_NULL_HANDLE,
	};

	VkWriteDescriptorSet descriptor_write = {
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet = desc_set,
		.dstBinding = binding,
		.dstArrayElement = 0,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
		.descriptorCount = 1,
		.pImageInfo = &image_info,
	};

	vkUpdateDescriptorSets(dev->device, 1, &descriptor_write, 0, NULL);

	/* Bind */
	vkCmdBindDescriptorSets(cmd->cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
				pipeline->layout, set, 1, &desc_set, 0, NULL);
}
