#include <GLFW/glfw3.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <chrono>
#include <iostream>
#include <asio.hpp>
#include <mutex>
#include <thread>
#include <map>
#include <memory>

using asio::ip::tcp;


enum packet_id {
    GET_DATA = 0,
    POPUP_1 = 1
};

#pragma pack(push, 1)
struct Packet {
    packet_id id;
    union {
        int number;
        double double_number;
        char str[256];
    };
    
};
#pragma pack(pop)

class Client {
private:
    tcp::socket socket_;

public:
    Client(tcp::socket socket)
        : socket_(std::move(socket)) {}

    void send(Packet packet) {
        asio::write(socket_, asio::buffer(&packet, sizeof(packet)));
    }
};

class Server {
private:
    std::mutex client_mutex_;  
    asio::io_context io_context_;
    tcp::acceptor acceptor_;
    unsigned int current_id_;
    std::map<int, std::unique_ptr<Client>> clients;

public:
    Server(const std::string& address, unsigned short port)
        : acceptor_(io_context_, tcp::endpoint(asio::ip::make_address(address), port)),
          current_id_(0) {}

    void handle_client_connect() {
        auto socket = std::make_unique<tcp::socket>(io_context_);
        asio::error_code ec;

        while(true) {
            ec = acceptor_.accept(*socket, ec);
            if (ec)
                continue;

            std::lock_guard<std::mutex> lock(client_mutex_);
            auto client = std::make_unique<Client>(std::move(*socket));
            clients[current_id_++] = std::move(client);
            std::cout << "Client connected!" << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }

    void send_to_client(unsigned int id, Packet packet) {
        std::lock_guard<std::mutex> lock(client_mutex_);
        if (clients.count(id)) {
            clients[id]->send(packet);
        }
    }
};


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

    Server server{"127.0.0.1", 1234};
    std::thread accept_thread([&server]() {
        server.handle_client_connect();
    });
    char text_buffer[256] = "";
    accept_thread.detach();
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
            ImGui::Text("Popup");
            ImGui::SameLine();
            ImGui::InputText("",text_buffer, sizeof(text_buffer));
            ImGui::SameLine();
            if(ImGui::Button("Send")) {
                Packet packet{POPUP_1};
                std::strcpy(packet.str,text_buffer);
                server.send_to_client(0,packet);

                std::cout << "Text sent: " << text_buffer << "\n";
                std::strcpy(text_buffer,"");
            }
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
