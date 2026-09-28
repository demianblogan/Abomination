#include "Core/Scene/Transform.h"

#include <glm/ext/matrix_transform.hpp>

namespace Abomination::Core
{
    glm::mat4 CalculateModelMatrix(const Transform& transform)
    {
        // A vertex is multiplied by this matrix from the right, so the operations happen from right to left:
        // scale around the mesh's own origin, rotate around it, then move it to its place in the world.
        // Scaling after moving would scale the distance from the world origin too; rotating after moving would swing
        // the object around the world origin instead of turning it in place.
        const glm::mat4 translation = glm::translate(glm::mat4(1.0f), transform.position);
        const glm::mat4 rotation = glm::mat4_cast(transform.rotation); // quaternion -> 4x4 rotation matrix
        const glm::mat4 scale = glm::scale(glm::mat4(1.0f), transform.scale);

        return translation * rotation * scale;
    }
}
