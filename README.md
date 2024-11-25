# tale

### Install
Open the project in Visual Studoi or VS Code with CMake extension. Most of the dependencies will be installed before build using vcpkg.

### PhysX
You will need to compile PhysX manually first, modify the preset in `physx/buildtools/presets/public/vc17win64.xml` to enable `PX_GENERATE_STATIC_LIBRARIES` and disable `NV_USE_STATIC_WINCRT`. Then compile the `install` target in debug and release.