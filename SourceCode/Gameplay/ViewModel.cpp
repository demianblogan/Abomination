#include "Gameplay/ViewModel.h"

#include <glm/ext/matrix_transform.hpp>

namespace Abomination::Gameplay
{
    glm::mat4 CalculateViewModelMatrix(const ViewModel& viewModel)
    {
        glm::vec3 position = viewModel.offset;
        switch (viewModel.side)
        {
        case ViewModelSide::Right:
            break;
        case ViewModelSide::Center:
            position.x = 0.0f;
            break;
        case ViewModelSide::Left:
            position.x = -position.x;
            break;
        }

        return glm::translate(glm::mat4(1.0f), position);
    }
}
