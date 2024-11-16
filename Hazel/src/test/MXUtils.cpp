#include "test/MXUtils.h"

namespace Hazel {

    namespace mx = MaterialX;

	namespace MXUtils {

        mx::Matrix44 GlmMat4ToMaterialXMat4(const glm::mat4& glmMatrix) {
            mx::Matrix44 materialXMatrix;
            for (int i = 0; i < 4; ++i) {
                for (int j = 0; j < 4; ++j) {
                    materialXMatrix[i][j] = glmMatrix[i][j];
                }
            }
            return materialXMatrix;
        }

	}

}