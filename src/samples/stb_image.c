#include "ut/ut.h"
#pragma warning(disable: 4459) // declaration of '...' hides global declaration
#define STBI_ASSERT(...) assert(__VA_ARGS__)
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
