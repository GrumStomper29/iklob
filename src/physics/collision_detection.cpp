#include "collision_detection.hpp"

#include "../renderer/renderer.hpp"

#include "glad/glad.h"
#include "glm/glm.hpp"
#include "Mathematics/IntrAlignedBox3Cylinder3.h"
#include "Mathematics/IntrTriangle3Cylinder3.h"


#include <limits>

void calculatePrimitiveBounds(const Renderer& renderer, Renderer::Primitive& primitive)
{
	primitive.minBound = glm::vec3{ std::numeric_limits<float>{}.max() };
	primitive.maxBound = glm::vec3{ std::numeric_limits<float>{}.min() };

	auto evaluateVertex{ [&](GLuint index)
		{
			glm::vec4 v{ renderer.sceneVertices()[index].position, 1.0f };
			v = primitive.transform * v;

			if (v.x < primitive.minBound.x) primitive.minBound.x = v.x;
			if (v.y < primitive.minBound.y) primitive.minBound.y = v.y;
			if (v.z < primitive.minBound.z) primitive.minBound.z = v.z;

			if (v.x > primitive.maxBound.x) primitive.maxBound.x = v.x;
			if (v.y > primitive.maxBound.y) primitive.maxBound.y = v.y;
			if (v.z > primitive.maxBound.z) primitive.maxBound.z = v.z;
		} };

	for (int i{ primitive.elementOffset }; i < primitive.elementCount + primitive.elementOffset; i += 3)
	{
		evaluateVertex(renderer.sceneIndices()[i + 0]);
		evaluateVertex(renderer.sceneIndices()[i + 1]);
		evaluateVertex(renderer.sceneIndices()[i + 2]);
	}
}

// template this lol
bool intersectCylinderWithMesh(glm::vec3 cylinderOrigin, glm::vec3 cylinderDirection,
	float cylinderRadius, float cylinderHeight, const Renderer& renderer, const Renderer::Mesh& mesh,
	glm::vec3& outNormal)
{

	gte::Line3<float> cylinderAxis{ { cylinderOrigin.x, cylinderOrigin.y, cylinderOrigin.z },
		{ cylinderDirection.x, cylinderDirection.y, cylinderDirection.z } };
	gte::Cylinder3<float> cylinder{ cylinderAxis, cylinderRadius, cylinderHeight };

	gte::TIQuery<float, gte::AlignedBox3<float>, gte::Cylinder3<float>> alignedBoxQuery{};

	gte::Triangle3<float> triangle{};

	gte::TIQuery<float, gte::Triangle3<float>, gte::Cylinder3<float>> query{};

	for (const auto& primitive : mesh.primitives)
	{
		gte::Vector3<float> min{ primitive.minBound.x, primitive.minBound.y, primitive.minBound.z };
		gte::Vector3<float> max{ primitive.maxBound.x, primitive.maxBound.y, primitive.maxBound.z };
		gte::AlignedBox3<float> box{ min, max };

		if (alignedBoxQuery(box, cylinder).intersect)
		{
			auto getVertex{ [&](GLuint index)
				{
					glm::vec4 v{ renderer.sceneVertices()[index].position, 1.0f };
					v = primitive.transform * v;

					return gte::Vector3<float>{ v.x, v.y, v.z };
				} };

			for (int i{ primitive.elementOffset }; i < primitive.elementCount + primitive.elementOffset; i += 3)
			{
				triangle = { getVertex(renderer.sceneIndices()[i + 0]), 
					getVertex(renderer.sceneIndices()[i + 1]), 
					getVertex(renderer.sceneIndices()[i + 2]) };

				if (query(triangle, cylinder).intersect)
				{
					// Calculate triangle normal
					gte::Vector3<float> a = getVertex(renderer.sceneIndices()[i + 1]) - getVertex(renderer.sceneIndices()[i + 0]);
					gte::Vector3<float> b = getVertex(renderer.sceneIndices()[i + 2]) - getVertex(renderer.sceneIndices()[i + 0]);

					outNormal.x = a[1] * b[2] - a[2] * b[1];
					outNormal.y = a[2] * b[0] - a[0] * b[2];
					outNormal.z = a[0] * b[1] - a[1] * b[0];

					return true;
				}
			}
		}
	}

	return false;
}

bool intersectCylinderWithMeshCulled(glm::vec3 cylinderOrigin, glm::vec3 cylinderDirection,
	float cylinderRadius, float cylinderHeight, const Renderer& renderer, const Renderer::Mesh& mesh,
	glm::vec3& outNormal, const glm::vec3& cullDir)
{

	gte::Line3<float> cylinderAxis{ { cylinderOrigin.x, cylinderOrigin.y, cylinderOrigin.z },
		{ cylinderDirection.x, cylinderDirection.y, cylinderDirection.z } };
	gte::Cylinder3<float> cylinder{ cylinderAxis, cylinderRadius, cylinderHeight };

	gte::TIQuery<float, gte::AlignedBox3<float>, gte::Cylinder3<float>> alignedBoxQuery{};

	gte::Triangle3<float> triangle{};

	gte::TIQuery<float, gte::Triangle3<float>, gte::Cylinder3<float>> query{};

	for (const auto& primitive : mesh.primitives)
	{
		gte::Vector3<float> min{ primitive.minBound.x, primitive.minBound.y, primitive.minBound.z };
		gte::Vector3<float> max{ primitive.maxBound.x, primitive.maxBound.y, primitive.maxBound.z };
		gte::AlignedBox3<float> box{ min, max };

		if (alignedBoxQuery(box, cylinder).intersect)
		{
			auto getVertex{ [&](GLuint index)
				{
					glm::vec4 v{ renderer.sceneVertices()[index].position, 1.0f };
					v = primitive.transform * v;

					return gte::Vector3<float>{ v.x, v.y, v.z };
				} };

			for (int i{ primitive.elementOffset }; i < primitive.elementCount + primitive.elementOffset; i += 3)
			{
				triangle = { getVertex(renderer.sceneIndices()[i + 0]),
					getVertex(renderer.sceneIndices()[i + 1]),
					getVertex(renderer.sceneIndices()[i + 2]) };

				if (query(triangle, cylinder).intersect)
				{
					// Calculate triangle normal
					gte::Vector3<float> a = getVertex(renderer.sceneIndices()[i + 1]) - getVertex(renderer.sceneIndices()[i + 0]);
					gte::Vector3<float> b = getVertex(renderer.sceneIndices()[i + 2]) - getVertex(renderer.sceneIndices()[i + 0]);

					outNormal.x = a[1] * b[2] - a[2] * b[1];
					outNormal.y = a[2] * b[0] - a[0] * b[2];
					outNormal.z = a[0] * b[1] - a[1] * b[0];

					outNormal = glm::normalize(outNormal);

					if (glm::dot(outNormal, cullDir) <= 0.0f)
					{
						return true;
					}
				}
			}
		}
	}

	return false;
}