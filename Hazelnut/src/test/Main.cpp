#include <test/Viewer.h>

#include <MaterialXFormat/Util.h>

int main()
{
    mx::FileSearchPath searchPath = mx::getDefaultDataSearchPath();
    mx::FilePathVec libraryFolders;

    mx::HwSpecularEnvironmentMethod specularEnvironmentMethod = mx::SPECULAR_ENVIRONMENT_FIS;
    bool shadowMap = true;
    bool drawEnvironment = false;
    Hazel::DocumentModifiers modifiers;

    mx::Color3 screenColor(mx::DEFAULT_SCREEN_COLOR_SRGB);

    // Append the standard library folder, giving it a lower precedence than user-supplied libraries.
    libraryFolders.push_back("libraries");

    {
        Hazel::Viewer* viewer = new Hazel::Viewer(
            searchPath,
            libraryFolders,
            screenColor);
        viewer->setSpecularEnvironmentMethod(specularEnvironmentMethod);
        viewer->setShadowMapEnable(shadowMap);
        viewer->setDrawEnvironment(drawEnvironment);
        viewer->setDocumentModifiers(modifiers);

        viewer->initialize();
        viewer->mainloop();
    }

    return 0;
}
