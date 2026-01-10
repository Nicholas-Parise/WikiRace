#include <iostream>
#include <ctime>
#include <unordered_set>
#include <random>
#include "sqlite3.h"
#include "dbUtil.h"
#include "graph.h"

#include <future>
#include <cmath>

#include <SFML/Graphics.hpp>
#include <imgui.h>
#include <imgui-SFML.h>


void Spinner(const char* label, float radius, int thickness, ImU32 color){
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImVec2 center(pos.x + radius, pos.y + radius);

    const int segments = 30;
    const float pi = 3.14159265358979323846f;
    const float tau = pi * 2.0f;

    float baseAngle = (float)ImGui::GetTime() * 6.0f;

    for (int i = 0; i < segments; i++)
    {
        float a = baseAngle + (i / (float)segments) * tau;

        // Distance from opposite side of wheel
        float delta = std::fmod(a - baseAngle + pi, tau) - pi;
        float t = 1.0f - std::fabs(delta) / pi;

        ImU32 col = ImGui::GetColorU32(
            ImVec4(
                ((color >> 0) & 255) / 255.0f * t,
                ((color >> 8) & 255) / 255.0f * t,
                ((color >> 16) & 255) / 255.0f * t,
                ((color >> 24) & 255) / 255.0f
            )
        );

        draw_list->AddCircleFilled(
            ImVec2(center.x + std::cos(a) * radius,
                   center.y + std::sin(a) * radius),
            thickness,
            col
        );
    }

    ImGui::Dummy(ImVec2(radius * 2, radius * 2));
}



int main(){
    // sanity check that OMP is enabled
    #ifdef _OPENMP
        std::cout << "OpenMP is enabled." << std::endl;
    #else
        std::cout << "OpenMP is not enabled." << std::endl;
    #endif
    time_t start_t;
    time(&start_t);

    sqlite3 *db;
    int rc = sqlite3_open("../wikipedia.sqlite", &db);

    if (rc)
    {
        std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
        return 1;
    }
    else
    {
        std::cout << "Opened database successfully" << std::endl;
    }

    sf::RenderWindow window(sf::VideoMode({720, 480}), "WikiRace");
    if (!ImGui::SFML::Init(window, true)) {
        std::cerr << "Failed to initialize ImGui-SFML\n";
        return 1;
    }


    bool dbLoading = true;
    std::future<void> dbFuture;

    dbUtil databaseUtil(db);

    std::unordered_map<long, std::vector<long>>* links = nullptr; // = databaseUtil.loadLinks_grouped_Threaded();
    graph* wikiGraph = nullptr;

    dbFuture = std::async(std::launch::async, [&]() {
        links = databaseUtil.loadLinks_grouped_Threaded();
        wikiGraph = new graph(links, databaseUtil);
        dbLoading = false;
        time_t end_t;
        time(&end_t);
        std::cout << "It took " << difftime(end_t, start_t) << " seconds." << std::endl;
    });

    

    char temp[256];
    static char firstArticleInput[256] = "";
    static char secondArticleInput[256] = "";

    std::vector<std::pair<long,std::string>> firstCandidates;
    std::vector<std::pair<long,std::string>> secondCandidates;

    long selectedFirst = -1;
    long selectedSecond = -1;

    bool firstPopupOpen = false;
    bool secondPopupOpen = false;

    bool searchTriggered = false;

    double searchTimeSeconds = 0.0;

    std::vector<std::string> results;

    sf::Clock clock;

    while (window.isOpen()) {
        while (auto event = window.pollEvent()){
            if (event->is<sf::Event::Closed>())
                window.close();
            ImGui::SFML::ProcessEvent(window,*event);
        }

        ImGui::SFML::Update(window, clock.restart());

        sf::Vector2u windowSize = window.getSize();
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2((float)windowSize.x, (float)windowSize.y));
        ImGui::Begin("WikiRace", nullptr, 
                 ImGuiWindowFlags_NoResize | 
                 ImGuiWindowFlags_NoMove |
                 ImGuiWindowFlags_NoCollapse);


        if (dbLoading) {
            ImGui::Text("Loading Wikipedia graph...");
            Spinner("loading", 20, 4, IM_COL32(255,255,255,255));
            ImGui::End();
            ImGui::SFML::Render(window);
            window.display();
            continue;
        }


        //First article input
        ImGui::InputText("First Article", firstArticleInput, 256, ImGuiInputTextFlags_EnterReturnsTrue);
        searchTriggered = ImGui::IsItemDeactivatedAfterEdit();
        
        ImGui::SameLine();
        if (ImGui::Button("Clear##first")) {
            firstArticleInput[0] = '\0';
            firstCandidates.clear();
            selectedFirst = -1;
        } 
        
        if (ImGui::Button("Search First")) {
            searchTriggered = true;
        }

        if (searchTriggered) {
            firstCandidates = databaseUtil.getTitleCandidates(firstArticleInput);
            firstPopupOpen = true;
            selectedFirst = -1;
        }
        

        if (firstPopupOpen && !firstCandidates.empty()) {
            ImGui::BeginChild("FirstCandidates", ImVec2(0, 120), true);
            for (auto& c : firstCandidates) {
                if (ImGui::Selectable(c.second.c_str(), selectedFirst == c.first)) {
                    selectedFirst = c.first;
                    strncpy(firstArticleInput, c.second.c_str(), 255);
                    firstCandidates.clear();
                    firstPopupOpen = false;
                }
            }
            ImGui::EndChild();
        }

        // switch logic
        ImGui::Separator();
        if (ImGui::Button("<-> Switch Articles")) {
            std::swap(selectedFirst, selectedSecond);

            strcpy(temp, firstArticleInput);
            strcpy(firstArticleInput, secondArticleInput);
            strcpy(secondArticleInput, temp);

            firstCandidates.clear();
            secondCandidates.clear();
        }


        //Second article input
        ImGui::Separator();
        
        ImGui::InputText("Second Article", secondArticleInput, 256, ImGuiInputTextFlags_EnterReturnsTrue);
        searchTriggered = ImGui::IsItemDeactivatedAfterEdit();

        ImGui::SameLine();
        if (ImGui::Button("Clear##second")) {
            secondArticleInput[0] = '\0';
            secondCandidates.clear();
            selectedSecond = -1;
        }
        
        if (ImGui::Button("Search Second") || ImGui::IsItemDeactivatedAfterEdit()) {
            searchTriggered = true;   
        }

        if (searchTriggered) {
            secondCandidates = databaseUtil.getTitleCandidates(secondArticleInput);
            secondPopupOpen = true;
            selectedSecond = -1;
        }


        if (secondPopupOpen && !secondCandidates.empty()) {
            ImGui::BeginChild("SecondCandidates", ImVec2(0, 120), true);
            for (auto& c : secondCandidates) {
                if (ImGui::Selectable(c.second.c_str(), selectedFirst == c.first)) {
                    selectedSecond = c.first;
                    strncpy(secondArticleInput, c.second.c_str(), 255);
                    secondCandidates.clear();
                    secondPopupOpen = false;
                }
            }
            ImGui::EndChild();
        }


        //graph search
        ImGui::Separator();
        if (ImGui::Button("Find Path")) {
            if (selectedFirst != -1 && selectedSecond != -1) {
                auto t0 = std::chrono::high_resolution_clock::now();
                results = wikiGraph->search(selectedFirst, selectedSecond, true);
                auto t1 = std::chrono::high_resolution_clock::now();

                searchTimeSeconds = std::chrono::duration<double>(t1 - t0).count();
            }
        }

        //Show path
        ImGui::Separator();
        if (!results.empty()) {
            ImGui::Separator();
            ImGui::Text("Path found in %.3f seconds", searchTimeSeconds);
            ImGui::Separator();

            for (size_t i = 0; i < results.size(); ++i) {
                ImGui::BulletText("%zu. %s", i + 1, results[i].c_str());
            }
        }

        ImGui::End();

        window.clear();
        ImGui::SFML::Render(window);
        window.display();
    }

    ImGui::SFML::Shutdown();
    return 0;
}