#include <string>
#if defined(_MSC_VER)
    #define MY_LIB_API __declspec(dllexport) // Microsoft
#elif defined(__GNUC__)
    #define UNITY_LIB_API __attribute__((visibility("default"))) // GCC
#else
    #define MY_LIB_API // Most compilers export all the symbols by default. We hope for the best here.
    #pragma warning Unknown dynamic link import/export semantics.
#endif
using namespace std;

extern "C" {
    UNITY_LIB_API void getRegion(char*, int);
    UNITY_LIB_API int createMap(int seed, int w, int h,int octave,float frequency,int pointCount,const char* terrainType);
    UNITY_LIB_API float GetHeightMapNoise(int x, int y);
    UNITY_LIB_API float InitHeightMapNoise(int w,int h,int octaves,float freq,int seed,const char* terrainT);
}
