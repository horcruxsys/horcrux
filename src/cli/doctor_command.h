// Horcrux - Doctor Command
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#ifndef HORCRUX_CLI_DOCTOR_COMMAND_H_
#define HORCRUX_CLI_DOCTOR_COMMAND_H_

#include "logger.h"

namespace horcrux::cli {

// Handle the doctor command
auto handle_doctor_command(int argc, char* argv[], Logger& logger) -> int;

// Android-specific doctor command
auto handle_doctor_android(Logger& logger) -> int;

} // namespace horcrux::cli

#endif // HORCRUX_CLI_DOCTOR_COMMAND_H_
