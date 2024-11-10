#pragma once

#include <fstream>
#include <vector>
#include <list>
#include <queue>
#include <string>
#include <algorithm>
#include <string.h> 
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <cmath>
#include <sstream>
#include <functional>
#include <iostream>
#include <bitset>
#include <array>
#include <limits>
#include <optional>
#include <unordered_map>
#include <memory>

#ifdef PLATFORM_LINUX
// All warning that external libraries give can and should be surpressed here.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wmisleading-indentation"
#pragma GCC diagnostic ignored "-Wreturn-type"
#pragma GCC diagnostic ignored "-Wconversion"

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>

#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>


#elif PLATFORM_WINDOWS

// Disable all warnings from external code
#pragma warning(push, 0)   

// We have to include this before glad.h to avoid the warning C4005: 'APIENTRY': macro redefinition.
// The alternative is to disable that warning, but it's a warning that could be useful and i don't want to disable it globally.
#include <Windows.h> 
#include "glad.h"

#endif // PLATFORM_LINUX

// basic types
typedef unsigned char uchar;
typedef unsigned int uint;
typedef unsigned short ushort;
typedef unsigned long ulong;

#include "common.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp> 
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/norm.hpp>

#include <bullet/btBulletCollisionCommon.h>
#include <bullet/btBulletDynamicsCommon.h>
#include <bullet/BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h>

// You may need to install assimp first: sudo apt-get install libassimp-dev
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/config.h>

// For when we want to load in just mtl's that aren't part of an object.
#include "OBJLoader.h"

#define POISSON_PROGRESS_INDICATOR 0
#include "PoissonGenerator.h"
#include "PerlinNoise.h"

#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_stdlib.h"

#ifdef PLATFORM_LINUX
#pragma GCC diagnostic pop
#elif PLATFORM_WINDOWS
#pragma warning(pop)
#endif // PLATFORM_LINUX

#include "Random.h"
#include "Math.h"
#include "StringFunctions.h"

//most standard OGL demos use int versions of TRUE and FALSE (careful not to mix up with bool true and false)
#define TRUE 1
#define FALSE 0

// generic error checking for OpenGL code
#define CheckGL() { _CheckGL( __FILE__, __LINE__ ); }
// forward declarations of helper functions
void _CheckGL( const char* f, int l );

template<glm::length_t N, typename T>
inline std::ostream& operator<<(std::ostream& os, const glm::vec<N, T>& vec)
{
    os << '(';

    for (glm::length_t i = 0; i < N; i++)
    {
        os << vec[i];

        if (i < N - 1)
        {
            os << ',';
        }
        else
        {
            os << ')';
        }
    }
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const glm::quat& quat)
{
    os << glm::vec4{ quat.w, quat.x, quat.y, quat.z };
    return os;
}