#pragma once

#include "glm/glm.hpp"

#include <functional> // for std::function
#include <string>
#include <utility> // for std::pair
#include <vector>

class Entity final
{
public:
	
	using MeshID = std::pair<glm::mat4, std::string>;

	Entity(const glm::mat4& transform)
		: m_transform{ transform }
	{}
	Entity(std::vector<MeshID>&& meshes)
		: m_meshes{ meshes }
	{}
	Entity(const glm::mat4& transform, std::vector<MeshID>&& meshes)
		: m_transform{ transform }
		, m_meshes{ meshes }
	{}

	void addMesh(MeshID mesh)
	{
		m_meshes.push_back(mesh);
	}
	void setMeshTransform(int index, const glm::mat4& mesh)
	{
		m_meshes[index].first = mesh;
	}
	void setTransform(const glm::mat4& transform)
	{
		m_transform = transform;
	}

	void render(std::function<void(MeshID, const glm::mat4&)> renderPolicy) const;

private:

	glm::mat4 m_transform{ 1.0f };

	std::vector<MeshID> m_meshes{};

};