#include "entity.hpp"

void Entity::render(std::function<void(MeshID, const glm::mat4&)> renderPolicy) const
{
	for (const auto& mesh : m_meshes)
	{
		renderPolicy(mesh, m_transform);
	}
}