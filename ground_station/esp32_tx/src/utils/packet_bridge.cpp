#include "packet_bridge.hpp"

LoraPayload_t convertControllerData(Controller& controller) {
    LoraPayload_t payload{};
    payload.st_l_ax_x = controller.left_x;
    payload.st_l_ax_y = controller.left_y;
    payload.st_r_ax_x = controller.right_x;
    payload.st_r_ax_y = controller.right_y;

    payload.reserved_buttons = controller.btn_mask;
    payload.trigger_l = controller.l_trigger;
    payload.trigger_r = controller.r_trigger;

    return payload;
}