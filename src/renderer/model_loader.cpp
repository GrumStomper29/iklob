#include "model_loader.hpp"

#include "renderer.hpp"
#include "gl_utils.hpp"

#include "glad/glad.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/quaternion.hpp"
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "tinygltf/tiny_gltf.h"

#include <algorithm> // for std::find and std::max
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <iterator> // for std::distance
#include <string>
#include <utility> // for std::pair

template <typename T>
std::vector<T> loadBuffer(const tinygltf::Accessor& accessor, const tinygltf::BufferView& bufferView, const tinygltf::Buffer& buffer)
{
	std::vector<T> r(bufferView.byteLength / sizeof(T));

	auto data{ buffer.data.data() + bufferView.byteOffset + accessor.byteOffset };
	
	for (int i{ 0 }; i < bufferView.byteLength / accessor.ByteStride(bufferView); ++i)
	{
		T t{};
		std::memcpy(&t, data, sizeof(T));
		data += sizeof(T);

		r[i] = t;
	}

	return r;
}

Renderer::Material loadPrimitiveMaterial(const tinygltf::Model& model, const tinygltf::Primitive& primitive)
{
	Renderer::Material ret{};

	if (primitive.material != -1)
	{
		const tinygltf::Material& material{ model.materials[primitive.material] };

		ret.baseColorFactor = {
			material.pbrMetallicRoughness.baseColorFactor[0],
			material.pbrMetallicRoughness.baseColorFactor[1], 
			material.pbrMetallicRoughness.baseColorFactor[2], 
			material.pbrMetallicRoughness.baseColorFactor[3], };

		const tinygltf::TextureInfo& baseColorTextureInfo{ material.pbrMetallicRoughness.baseColorTexture };
		if (baseColorTextureInfo.index != -1)
		{
			const tinygltf::Texture& baseColorTexture{ model.textures[baseColorTextureInfo.index] };
			const tinygltf::Image& baseColorTextureImage{ model.images[baseColorTexture.source] };
			const tinygltf::Sampler& baseColorTextureSampler{ model.samplers[baseColorTexture.sampler] };

			GLint minFilter{ [&]() { 
				switch (baseColorTextureSampler.minFilter)
				{
				case TINYGLTF_TEXTURE_FILTER_NEAREST: return GL_NEAREST;
				case TINYGLTF_TEXTURE_FILTER_LINEAR: return GL_LINEAR;
				default: return GL_LINEAR_MIPMAP_LINEAR;
				} }() };

			GLint magFilter{ [&]() {
				switch (baseColorTextureSampler.magFilter)
				{
				case TINYGLTF_TEXTURE_FILTER_NEAREST: return GL_NEAREST;
				default: return GL_LINEAR;
				} }() };

			ret.hasBaseColorTexture = true;
			ret.baseColorTexture = createTexture(baseColorTextureImage.image.data(),
				baseColorTextureImage.width, baseColorTextureImage.height,
				minFilter, magFilter);
		}
	}

	return ret;
}

glm::mat4 getNodeTransform(const tinygltf::Node& node)
{

	glm::mat4 translationMatrix{ 1.0f };
	if (node.translation.size() != 0)
	{
		glm::vec3 translation{ 
			static_cast<float>(node.translation[0]),
			static_cast<float>(node.translation[1]),
			static_cast<float>(node.translation[2]) };
		translationMatrix = glm::translate(translationMatrix, translation);
	}

	glm::mat4 rotationMatrix{ 1.0f };
	if (node.rotation.size() != 0)
	{
		// glm quaternions are in WXYZ
		glm::quat rotation{
			static_cast<float>(node.rotation[3]),
			static_cast<float>(node.rotation[0]),
			static_cast<float>(node.rotation[1]),
			static_cast<float>(node.rotation[2]), };
		rotationMatrix = glm::toMat4(rotation);
	}

	glm::mat4 scaleMatrix{ 1.0f };
	if (node.scale.size() != 0)
	{
		glm::vec3 scale{
			static_cast<float>(node.scale[0]),
			static_cast<float>(node.scale[1]),
			static_cast<float>(node.scale[2]) };
		scaleMatrix = glm::scale(scaleMatrix, scale);
	}

	return translationMatrix * rotationMatrix * scaleMatrix;
}

void loadNodeSkin(const tinygltf::Model& model, const::tinygltf::Node& node, Renderer::Mesh& ret)
{
	const auto& skin{ model.skins[node.skin] };

	std::vector<glm::mat4> inverseBindMatrices{};
	if (skin.inverseBindMatrices != -1)
	{
		const auto& inverseBindMatricesAccessor{ model.accessors[skin.inverseBindMatrices] };
		const auto& inverseBindMatricesBufView{ model.bufferViews[inverseBindMatricesAccessor.bufferView] };
		const auto& inverseBindMatricesBuf{ model.buffers[inverseBindMatricesBufView.buffer] };

		auto data{ inverseBindMatricesBuf.data.data() + inverseBindMatricesBufView.byteOffset };
		const std::size_t count{ inverseBindMatricesBufView.byteLength / sizeof(glm::mat4) };

		inverseBindMatrices = loadBuffer<glm::mat4>(inverseBindMatricesAccessor, inverseBindMatricesBufView, inverseBindMatricesBuf);
	}
	else
	{
		std::cerr << "MODEL LOADER: ERROR: Inverse bind matrices expected but not found\n";
	}

	// Create table of known child indices
	// Check if table contains index of joint being loaded

	// This is an array of all children indices. It exists
	// so that a joint being loaded can check here if it is
	// a child joint. There might be a more elegant way to do
	// this but it's probably bikeshedding
	std::vector<int> globalChildrenIndices{};

	int i{ 0 };
	for (const auto& jointIndex : skin.joints)
	{
		const auto& joint{ model.nodes[jointIndex] };

		std::vector<int> children{ joint.children };
		// Convert the children node (GLTF) indices into children joint (engine) indices
		for (int& child : children)
		{
			auto childIt{ std::find(skin.joints.begin(), skin.joints.end(), child) };

			if (childIt != skin.joints.end())
			{
				child = std::distance(skin.joints.begin(), childIt);

				globalChildrenIndices.push_back(child);
			}
		}

		ret.joints.push_back({
			.name{ joint.name },
			.nodeIndex{ jointIndex },
			.hasJointParent{ std::find(globalChildrenIndices.begin(), globalChildrenIndices.end(),
				i) != globalChildrenIndices.end() },
			.children{ children },
			.transform{ getNodeTransform(joint) },
			.inverseBindMatrix{ inverseBindMatrices[i] },
		});

		++i;
	}


	if (model.animations.size() != 0)
	{
		for (const auto& channel : model.animations[0].channels)
		{
			int targetNode{ channel.target_node };

			auto node{ std::find_if(ret.joints.begin(), ret.joints.end(),
				[targetNode](const Renderer::Joint& i) {
					return (i.nodeIndex == targetNode);
				}) };

			if (node != ret.joints.end())
			{
				Renderer::AnimationSampler animationSampler{};

				if (channel.target_path == "translation")
				{
					animationSampler.path = Renderer::AnimationSampler::TRANSLATION;
				}
				else if (channel.target_path == "rotation")
				{
					animationSampler.path = Renderer::AnimationSampler::ROTATION;
				}
				else if (channel.target_path == "scale")
				{
					animationSampler.path = Renderer::AnimationSampler::SCALE;
				}
				else if (channel.target_path == "weights")
				{
					std::cerr << "MODEL LOADER: ERROR: Weights animation target detected but not supported\n";
				}

				const auto& inputAccessor{ model.accessors[model.animations[0].samplers[channel.sampler].input] };
				const auto& inputBufferView{ model.bufferViews[inputAccessor.bufferView] };
				const auto& inputBuffer{ model.buffers[inputBufferView.buffer] };

				if (inputAccessor.maxValues[0] > ret.maxTime)
				{
					ret.maxTime = inputAccessor.maxValues[0];
				}

				auto inputData{ inputBuffer.data.data() + inputBufferView.byteOffset + inputAccessor.byteOffset };

				for (int i{ 0 }; i < inputBufferView.byteLength / sizeof(float); ++i)
				{
					float inputValue{};
					std::memcpy(&inputValue, inputData, sizeof(float));
					inputData += sizeof(float);

					animationSampler.input.push_back(inputValue);
				}

				const auto& outputAccessor{ model.accessors[model.animations[0].samplers[channel.sampler].output] };
				const auto& outputBufferView{ model.bufferViews[outputAccessor.bufferView] };
				const auto& outputBuffer{ model.buffers[outputBufferView.buffer] };

				auto outputData{ outputBuffer.data.data() + outputBufferView.byteOffset + outputAccessor.byteOffset };

				
				if (outputAccessor.type == TINYGLTF_TYPE_VEC3)
				{
					for (int i{ 0 }; i < outputBufferView.byteLength / outputAccessor.ByteStride(outputBufferView); ++i)
					{
						glm::vec3 outputValue{ 0.0f };
						std::memcpy(&outputValue, outputData, sizeof(glm::vec3));
						outputData += sizeof(glm::vec3);

						animationSampler.output.push_back(glm::vec4{ outputValue, 0.0f });
					}
				}
				else if (outputAccessor.type == TINYGLTF_TYPE_VEC4)
				{
					animationSampler.output = loadBuffer<glm::vec4>(outputAccessor, outputBufferView, outputBuffer);
				}

				node->animationSamplers.insert(std::pair{ animationSampler.path, animationSampler });
			}
			else
			{
				std::cerr << "MODEL LOADER: ERROR: Target joint node not found for animation channel\n";
			}
		}
	}
}

Renderer::Primitive loadPrimitive(const tinygltf::Model& model, const tinygltf::Primitive& primitive,
	const glm::mat4& nodeTransform, std::vector<Renderer::Vertex>& vertices, std::vector<GLuint>& indices)
{
	Renderer::Primitive ret
	{
		.material{ loadPrimitiveMaterial(model, primitive) },
		.transform     { nodeTransform },
		.elementOffset { static_cast<GLsizei>(indices.size()) },
	};

	const tinygltf::Accessor   indicesAccessor   { model.accessors[primitive.indices] };
	const tinygltf::BufferView indicesBufferView { model.bufferViews[indicesAccessor.bufferView] };
	const tinygltf::Buffer     indicesBuffer     { model.buffers[indicesBufferView.buffer] };

	const unsigned char* indexData{ indicesBuffer.data.data() };
	indexData += indicesBufferView.byteOffset;

	const std::uint32_t indexOffset{ static_cast<std::uint32_t>(vertices.size()) };

	switch (indicesAccessor.componentType)
	{
	case (TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT):
		for (int i{ 0 }; i < indicesBufferView.byteLength / sizeof(std::uint16_t); ++i)
		{
			std::uint16_t index{};
			std::memcpy(&index, indexData, sizeof(std::uint16_t));
			indexData += sizeof(std::uint16_t);

			indices.push_back(static_cast<std::uint32_t>(index) + indexOffset);
		}
		break;

	case (TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT):
		for (int i{ 0 }; i < indicesBufferView.byteLength / sizeof(std::uint32_t); ++i)
		{
			std::uint32_t index{};
			std::memcpy(&index, indexData, sizeof(std::uint32_t));
			indexData += sizeof(std::uint32_t);

			indices.push_back(index + indexOffset);
		}
		break;

	default:
		std::cerr << "MODEL LOADER: ERROR: Unrecognized index buffer accessor component type: " << indicesAccessor.componentType << '\n';
		break;
	}

	ret.elementCount = static_cast<GLsizei>(indices.size()) - ret.elementOffset;

	const unsigned char* positionData { nullptr };
	const unsigned char* normalData   { nullptr };
	const unsigned char* texCoordData { nullptr };
	const unsigned char* jointData    { nullptr };
	const unsigned char* weightData   { nullptr };

	std::size_t vertexCount{};

	for (const auto& attribute : primitive.attributes)
	{
		const tinygltf::Accessor& accessor     { model.accessors[attribute.second] };
		const tinygltf::BufferView& bufferView { model.bufferViews[accessor.bufferView] };
		const tinygltf::Buffer& buffer         { model.buffers[bufferView.buffer] };
		
		if (attribute.first == "POSITION")
		{
			vertexCount = bufferView.byteLength / sizeof(glm::vec3);
			positionData = buffer.data.data() + bufferView.byteOffset;
		}
		else if (attribute.first == "NORMAL")
		{
			normalData = buffer.data.data() + bufferView.byteOffset;
		}
		else if (attribute.first == "TEXCOORD_0")
		{
			texCoordData = buffer.data.data() + bufferView.byteOffset;
		}
		else if (attribute.first == "JOINTS_0")
		{
			jointData = buffer.data.data() + bufferView.byteOffset;
		}
		else if (attribute.first == "WEIGHTS_0")
		{
			weightData = buffer.data.data() + bufferView.byteOffset;
		}
	}

	for (std::size_t i{ 0 }; i < vertexCount; ++i)
	{
		glm::vec3 position{};
		std::memcpy(&position, positionData, sizeof(glm::vec3));
		positionData += sizeof(glm::vec3);

		glm::vec3 normal{ 0.0f, 1.0f, 0.0f };
		if (normalData)
		{
			std::memcpy(&normal, normalData, sizeof(glm::vec3));
			normalData += sizeof(glm::vec3);
		}

		glm::vec2 texCoord{ 0.0f, 0.0f };
		if (texCoordData)
		{
			std::memcpy(&texCoord, texCoordData, sizeof(glm::vec2));
			texCoordData += sizeof(glm::vec2);
		}

		// A joint of -1 denotes that there is no bone or weight to account for.
		glm::ivec4 joints{ -1, -1, -1, -1 };
		if (jointData)
		{
			glm::u8vec4 u8joints{};

			std::memcpy(&u8joints, jointData, sizeof(glm::u8vec4));
			jointData += sizeof(glm::u8vec4);

			joints = u8joints;
		}
		
		glm::vec4 weights{ 0.0f, 0.0f, 0.0f, 0.0f };
		if (weightData)
		{
			std::memcpy(&weights, weightData, sizeof(glm::vec4));
			weightData += sizeof(glm::vec4);
		}

		vertices.push_back(Renderer::Vertex{ position, normal, texCoord, joints, weights });
	}

	return ret;
}

void loadNode(const tinygltf::Model& model, const tinygltf::Node& node, 
	const glm::mat4& inheritedTransform, std::vector<Renderer::Vertex>& vertices, 
	std::vector<GLuint>& indices, Renderer::Mesh& ret)
{
	// Todo: test if this works as intended
	glm::mat4 transform{ inheritedTransform * getNodeTransform(node) };

	// IMPORTANT: This is only prepared for models with one or fewer skins.
	// Any more skins will result in UB
	if (node.skin != -1)
	{
		loadNodeSkin(model, node, ret);
	}

	if (node.mesh != -1)
	{
		const tinygltf::Mesh& mesh{ model.meshes[node.mesh] };

		for (const tinygltf::Primitive& primitive : mesh.primitives)
		{
			ret.primitives.push_back(loadPrimitive(model, primitive, transform, vertices, indices));
		}
	}

	for (int nodeIndex : node.children)
	{
		loadNode(model, model.nodes[nodeIndex], transform, vertices, indices, ret);
	}
}

Renderer::Mesh loadModel(const std::string& path, std::vector<Renderer::Vertex>& vertices, std::vector<GLuint>& indices)
{
	Renderer::Mesh ret{};

	tinygltf::Model    model{};
	tinygltf::TinyGLTF loader{};
	std::string        warning{};
	std::string        error{};

	loader.LoadBinaryFromFile(&model, &error, &warning, path);

	if (!error.empty())
	{
		std::cerr << error << '\n';
	}
	if (!warning.empty())
	{
		std::cerr << warning << '\n';
	}

	for (const tinygltf::Scene& scene : model.scenes)
	{
		for (int nodeIndex : scene.nodes)
		{
			loadNode(model, model.nodes[nodeIndex], glm::mat4{ 1.0f }, vertices, indices, ret);
		}
	}

	return ret;
}