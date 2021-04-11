#include "Scene.h"
#include <thread>
#include "../SystemTable.h"
#include <memory>

void SceneLoad::Initialize()
{   
    std::thread thread(LoadingThread, this);
    thread.detach();
    count = 0;
}

void SceneLoad::Update(DX::StepTimer const& timer)
{

    ImVec4 windowBgCol = ImVec4(0.1f, 0.4f, 0.1f, 1.0f);
    if (nextScene->initialized)
    {
        windowBgCol = ImVec4(0.1f, 0.7f, 0.1f, 1.0f);
    }
    ImGui::PushStyleColor(ImGuiCol_WindowBg, windowBgCol);
    ImVec2 sizeAppWindow = ImGui::GetIO().DisplaySize;
    ImGui::SetNextWindowPos(ImVec2(sizeAppWindow.x * 0.5, sizeAppWindow.y * 0.5), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(280, 0), ImGuiCond_Always);
    ImGui::Begin("Loading", NULL, ImGuiWindowFlags_NoTitleBar);
    if (!nextScene->initialized)
    {
        count += timer.GetElapsedSeconds();
        ImGui::Text("Loading... time %.2fs", (float)(count));
    }
    else
    {
        ImGui::Text("Loading... time %.2fs. SUCCEED", (float)(count));
    }
    ImGui::End();
    ImGui::PopStyleColor();

    if (nextScene->initialized)
    {
        g_pSys->pSceneManager->ChangeScene(nextScene.release());
        g_pSys->pSceneManager->Update(timer);
    }

}

void SceneLoad::Render()
{

}

void SceneLoad::LoadingThread(SceneLoad* scene)
{
    scene->nextScene->Initialize();
    scene->nextScene->initialized = true;

}
