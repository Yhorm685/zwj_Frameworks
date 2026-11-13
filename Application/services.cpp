#include "Application/commands.hpp"

namespace fw::app {
void startServices(Logger&) {}
void serviceEvents(Logger&) {}
bool boardCommand(Frame, Logger&) { return false; }
}
