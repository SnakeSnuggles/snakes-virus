#include <GLFW/glfw3.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "asio.hpp"

using asio::ip::tcp;

int main() {
    if (!glfwInit()) return -1;
    GLFWwindow* w = glfwCreateWindow(640, 480, "control pannel", 0, 0);
    if (!w) return glfwTerminate(), -1;
    glfwMakeContextCurrent(w);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(w, true);
    ImGui_ImplOpenGL3_Init();
    ImGui::StyleColorsDark();
    
    static ImVec4 background_color = ImVec4(0.102f, 0.102f, 0.102f, 1.0f);

    while (!glfwWindowShouldClose(w)) {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();

        ImGui::NewFrame();
        
        ImGui::Begin("Mouse");
        /*
            - make mouse move randomly (t)
            - inverted mouse (t)
            - lock mouse to a small box
        */
        ImGui::End();

        ImGui::Begin("Keyboard");
        /*
            - send message to be typed by the user's keyboard
            - echo keyboard (repeats what was typed after a few seconds)
        */
        ImGui::End();

        ImGui::Begin("Visual & Sound");
        /*
            - flip screen (0, 90, 180, 270)
            - set system volume
            - fake BSOD 
            - screen tint (send a color to recolor the screen)
        */
        ImGui::End();

        ImGui::Begin("Send Data");
        /*
            - open an image from a link
                - or maybe send the image to be downloaded and then shown
            - send a pop up with a message of my choosing
                - group mode (a few pop ups)
                - life bar (how many times you have to close the popup before it does not come back)
                - infinite life mode
                - create popup that avoids the mouse

            - send link which gets opened in a browser
            - make it so keyboard presses become a random key (t)
            - send tts message
        */
        ImGui::End();

        ImGui::Begin("Manage");
        /*
            - Start/Stop all button
            - A target selector
            - A auto mode which does pranks automatically
            - A log of pranks ran
            - Add a prank schedual
        */
        ImGui::End();



        ImGui::Begin("customize");
            ImGui::ColorPicker3("Background", (float*)&background_color);
        ImGui::End();

        ImGui::Render();
        int w_, h_; glfwGetFramebufferSize(w, &w_, &h_);
        glViewport(0, 0, w_, h_);
        glClearColor(background_color.x, background_color.y, background_color.y, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(w);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(w);
    glfwTerminate();
    return 0;
}
