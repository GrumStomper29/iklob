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

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>

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

			ret.hasBaseColorTexture = true;
			ret.baseColorTexture = createTexture(baseColorTextureImage.image.data(),
				baseColorTextureImage.width, baseColorTextureImage.height);
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

	std::size_t vertexCount{};

	for (const auto& attribute : primitive.attributes)
	{
		const tinygltf::Accessor& accessor   { model.accessors[attribute.second] };
		const tinygltf::BufferView& bufferView { model.bufferViews[accessor.bufferView] };
		const tinygltf::Buffer& buffer     { model.buffers[bufferView.buffer] };

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

		vertices.push_back(Renderer::Vertex{ position, normal, texCoord });
	}

	return ret;
}

void loadNode(const tinygltf::Model& model, const tinygltf::Node& node, 
	const glm::mat4& inheritedTransform, std::vector<Renderer::Vertex>& vertices, 
	std::vector<GLuint>& indices, Renderer::Mesh& ret)
{
	// Todo: test if this works as intended
	glm::mat4 transform{ inheritedTransform * getNodeTransform(node) };
	//transform = inheritedTransform * transform;

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