#pragma once

#include <glm/glm.hpp>
#include <MaterialXRender/Types.h>

namespace Hazel {

    namespace mx = MaterialX;

	namespace MXUtils {

		mx::Matrix44 GlmMat4ToMaterialXMat4(const glm::mat4& glmMatrix);

	}

}