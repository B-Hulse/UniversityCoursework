#include <ostream>
#include "glm/glm.hpp"
#include <iomanip>

std::ostream& operator<<(std::ostream& outStream, glm::mat4& matrix)
{
	for (int row = 0; row < 4; row++)
	{
		for (int col = 0; col < 4; col++)
		{
            outStream << std::setprecision(4) << std::setw(8) << matrix[row][col] << ((col == 3) ? "\n" : " ");
		}
	}
    // and return the stream
    return outStream;
}

std::ostream& operator<<(std::ostream& outStream, glm::vec3& vector)
{
    outStream << std::setprecision(4) << vector.x << " " << std::setprecision(4) << vector.y << " " << std::setprecision(4) << vector.z;
    return outStream;
}

std::istream& operator>>(std::istream& inStream, glm::vec4& value)
{
    int component;
    inStream >> component;
    value.r = component;
    inStream >> component;
    value.g = component;
    inStream >> component;
    value.b = component;
    value.a = 1.0f;
    return inStream;
}

std::ostream& operator<<(std::ostream& outStream, glm::vec4& vector)
{
    outStream << std::setprecision(4) << vector.x << " " << std::setprecision(4) << vector.y << " " << std::setprecision(4) << vector.z << " " << std::setprecision(4) << vector.w;
    return outStream;
}
