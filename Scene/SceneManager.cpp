#include "Scene.h"

void SceneManager::Update(DX::StepTimer const& timer)
{
    currentScene->Update(timer);
}

void SceneManager::Render()
{
    currentScene->Render();
}

void SceneManager::ChangeScene(Scene* new_scene)
{
    currentScene.reset(new_scene);

    if (!currentScene->initialized)
    {
        currentScene->Initialize();
        currentScene->initialized = true;
    }
}
