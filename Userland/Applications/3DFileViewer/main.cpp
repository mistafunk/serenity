/*
 * Copyright (c) 2021, Jesse Buhagiar <jooster669@gmail.com>
 * Copyright (c) 2022, the SerenityOS developers.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <LibCore/ArgsParser.h>
#include <LibCore/System.h>
#include <LibDesktop/Launcher.h>
#include <LibFileSystemAccessClient/Client.h>
#include <LibGUI/ActionGroup.h>
#include <LibGUI/Application.h>
#include <LibGUI/Menu.h>
#include <LibMain/Main.h>

#include "GLContextWidget.h"

ErrorOr<int> serenity_main(Main::Arguments arguments)
{
    auto app = TRY(GUI::Application::create(arguments));

    auto const man_file = "/usr/share/man/man1/Applications/3DFileViewer.md";

    TRY(Desktop::Launcher::add_allowed_handler_with_only_specific_urls("/bin/Help", { URL::create_with_file_scheme(man_file) }));
    TRY(Desktop::Launcher::seal_allowlist());

    StringView filename;

    Core::ArgsParser args_parser;
    args_parser.add_positional_argument(filename, "3D model file to open", "path", Core::ArgsParser::Required::No);
    args_parser.parse(arguments);

    if (filename.is_empty())
        filename = "/home/anon/Documents/3D Models/Lantern.glb"sv;

    TRY(Core::System::pledge("stdio thread recvfd sendfd rpath unix prot_exec map_fixed"));

    TRY(Core::System::unveil("/tmp/session/%sid/portal/filesystemaccess", "rw"));
    TRY(Core::System::unveil("/res", "r"));
    TRY(Core::System::unveil("/usr/lib", "r"));
    TRY(Core::System::unveil(nullptr, nullptr));

    // Construct the main window
    auto window = GUI::Window::construct();
    auto app_icon = GUI::Icon::default_icon("app-3d-file-viewer"sv);
    window->set_icon(app_icon.bitmap_for_size(16));
    window->set_title("3D File Viewer");
    window->resize(640 + 4, 480 + 4);
    window->set_resizable(false);
    window->set_double_buffering_enabled(true);
    auto widget = window->set_main_widget<GLContextWidget>();

    auto& time = widget->add<GUI::Label>();
    time.set_visible(false);
    time.set_foreground_role(ColorRole::HoverHighlight);
    time.set_relative_rect({ 0, 8, 100, 10 });
    time.set_text_alignment(Gfx::TextAlignment::CenterRight);
    time.set_x(widget->width() - time.width() - 6);
    widget->set_stat_label(time);

    auto file_menu = window->add_menu("&File"_string);

    file_menu->add_action(GUI::CommonActions::make_open_action([&](auto&) {
        FileSystemAccessClient::OpenFileOptions options {
            .allowed_file_types = { { GUI::FileTypeFilter { "Object Files", { { "obj" } } },
                GUI::FileTypeFilter { "Khronos Binary glTF", { { "glb" } } },
                GUI::FileTypeFilter::all_files() } },
        };
        auto response = FileSystemAccessClient::Client::the().open_file(window, options);
        if (response.is_error())
            return;

        auto file = response.release_value();
        widget->load_file(file.filename(), file.release_stream());
    }));
    file_menu->add_separator();
    file_menu->add_action(GUI::CommonActions::make_quit_action([&](auto&) {
        app->quit();
    }));

    auto view_menu = window->add_menu("&View"_string);
    view_menu->add_action(GUI::CommonActions::make_fullscreen_action([&](auto&) {
        window->set_fullscreen(!window->is_fullscreen());
    }));

    auto rotation_axis_menu = view_menu->add_submenu("Rotation &Axis"_string);
    auto rotation_x_action = GUI::Action::create_checkable("&X", [&widget](auto&) {
        widget->toggle_rotate_x();
    });
    auto rotation_y_action = GUI::Action::create_checkable("&Y", [&widget](auto&) {
        widget->toggle_rotate_y();
    });
    auto rotation_z_action = GUI::Action::create_checkable("&Z", [&widget](auto&) {
        widget->toggle_rotate_z();
    });

    rotation_axis_menu->add_action(*rotation_x_action);
    rotation_axis_menu->add_action(*rotation_y_action);
    rotation_axis_menu->add_action(*rotation_z_action);

    rotation_x_action->set_checked(true);
    rotation_z_action->set_checked(true);

    auto rotation_speed_menu = view_menu->add_submenu("Rotation &Speed"_string);
    GUI::ActionGroup rotation_speed_actions;
    rotation_speed_actions.set_exclusive(true);

    auto no_rotation_action = GUI::Action::create_checkable("N&o Rotation", [&widget](auto&) {
        widget->set_rotation_speed(0.f);
    });
    auto slow_rotation_action = GUI::Action::create_checkable("&Slow", [&widget](auto&) {
        widget->set_rotation_speed(30.f);
    });
    auto normal_rotation_action = GUI::Action::create_checkable("&Normal", [&widget](auto&) {
        widget->set_rotation_speed(60.f);
    });
    auto fast_rotation_action = GUI::Action::create_checkable("&Fast", [&widget](auto&) {
        widget->set_rotation_speed(90.f);
    });

    rotation_speed_actions.add_action(*no_rotation_action);
    rotation_speed_actions.add_action(*slow_rotation_action);
    rotation_speed_actions.add_action(*normal_rotation_action);
    rotation_speed_actions.add_action(*fast_rotation_action);

    rotation_speed_menu->add_action(*no_rotation_action);
    rotation_speed_menu->add_action(*slow_rotation_action);
    rotation_speed_menu->add_action(*normal_rotation_action);
    rotation_speed_menu->add_action(*fast_rotation_action);

    normal_rotation_action->set_checked(true);

    auto show_frame_rate_action = GUI::Action::create_checkable("Show Frame &Rate", [&widget](auto&) {
        widget->toggle_show_frame_rate();
    });

    view_menu->add_action(*show_frame_rate_action);

    auto texture_menu = window->add_menu("&Texture"_string);

    auto texture_enabled_action = GUI::Action::create_checkable("&Enable Texture", [&widget](auto& action) {
        widget->set_texture_enabled(action.is_checked());
    });
    texture_enabled_action->set_checked(true);
    texture_menu->add_action(texture_enabled_action);

    auto wrap_u_menu = texture_menu->add_submenu("Wrap &S"_string);
    GUI::ActionGroup wrap_s_actions;
    wrap_s_actions.set_exclusive(true);

    auto wrap_u_repeat_action = GUI::Action::create_checkable("&Repeat", [&widget](auto&) {
        widget->set_wrap_s_mode(GL_REPEAT);
    });
    auto wrap_u_mirrored_repeat_action = GUI::Action::create_checkable("&Mirrored Repeat", [&widget](auto&) {
        widget->set_wrap_s_mode(GL_MIRRORED_REPEAT);
    });
    auto wrap_u_clamp_action = GUI::Action::create_checkable("&Clamp", [&widget](auto&) {
        widget->set_wrap_s_mode(GL_CLAMP);
    });

    wrap_s_actions.add_action(*wrap_u_repeat_action);
    wrap_s_actions.add_action(*wrap_u_mirrored_repeat_action);
    wrap_s_actions.add_action(*wrap_u_clamp_action);

    wrap_u_menu->add_action(*wrap_u_repeat_action);
    wrap_u_menu->add_action(*wrap_u_mirrored_repeat_action);
    wrap_u_menu->add_action(*wrap_u_clamp_action);

    wrap_u_repeat_action->set_checked(true);

    auto wrap_t_menu = texture_menu->add_submenu("Wrap &T"_string);
    GUI::ActionGroup wrap_t_actions;
    wrap_t_actions.set_exclusive(true);

    auto wrap_t_repeat_action = GUI::Action::create_checkable("&Repeat", [&widget](auto&) {
        widget->set_wrap_t_mode(GL_REPEAT);
    });
    auto wrap_t_mirrored_repeat_action = GUI::Action::create_checkable("&Mirrored Repeat", [&widget](auto&) {
        widget->set_wrap_t_mode(GL_MIRRORED_REPEAT);
    });
    auto wrap_t_clamp_action = GUI::Action::create_checkable("&Clamp", [&widget](auto&) {
        widget->set_wrap_t_mode(GL_CLAMP);
    });

    wrap_t_actions.add_action(*wrap_t_repeat_action);
    wrap_t_actions.add_action(*wrap_t_mirrored_repeat_action);
    wrap_t_actions.add_action(*wrap_t_clamp_action);

    wrap_t_menu->add_action(*wrap_t_repeat_action);
    wrap_t_menu->add_action(*wrap_t_mirrored_repeat_action);
    wrap_t_menu->add_action(*wrap_t_clamp_action);

    wrap_t_repeat_action->set_checked(true);

    auto texture_scale_menu = texture_menu->add_submenu("S&cale"_string);
    GUI::ActionGroup texture_scale_actions;
    texture_scale_actions.set_exclusive(true);

    auto texture_scale_025_action = GUI::Action::create_checkable("0.25x", [&widget](auto&) {
        widget->set_texture_scale(0.25f);
    });

    auto texture_scale_05_action = GUI::Action::create_checkable("0.5x", [&widget](auto&) {
        widget->set_texture_scale(0.5f);
    });

    auto texture_scale_1_action = GUI::Action::create_checkable("1x", [&widget](auto&) {
        widget->set_texture_scale(1);
    });

    auto texture_scale_2_action = GUI::Action::create_checkable("2x", [&widget](auto&) {
        widget->set_texture_scale(2);
    });

    auto texture_scale_4_action = GUI::Action::create_checkable("4x", [&widget](auto&) {
        widget->set_texture_scale(4);
    });

    texture_scale_actions.add_action(*texture_scale_025_action);
    texture_scale_actions.add_action(*texture_scale_05_action);
    texture_scale_actions.add_action(*texture_scale_1_action);
    texture_scale_actions.add_action(*texture_scale_2_action);
    texture_scale_actions.add_action(*texture_scale_4_action);

    texture_scale_menu->add_action(*texture_scale_025_action);
    texture_scale_menu->add_action(*texture_scale_05_action);
    texture_scale_menu->add_action(*texture_scale_1_action);
    texture_scale_menu->add_action(*texture_scale_2_action);
    texture_scale_menu->add_action(*texture_scale_4_action);

    texture_scale_1_action->set_checked(true);

    auto texture_mag_filter_menu = texture_menu->add_submenu("Mag Filter"_string);
    GUI::ActionGroup texture_mag_filter_actions;
    texture_mag_filter_actions.set_exclusive(true);

    auto texture_mag_filter_nearest_action = GUI::Action::create_checkable("&Nearest", [&widget](auto&) {
        widget->set_mag_filter(GL_NEAREST);
    });

    auto texture_mag_filter_linear_action = GUI::Action::create_checkable("&Linear", [&widget](auto&) {
        widget->set_mag_filter(GL_LINEAR);
    });

    texture_mag_filter_actions.add_action(*texture_mag_filter_nearest_action);
    texture_mag_filter_actions.add_action(*texture_mag_filter_linear_action);

    texture_mag_filter_menu->add_action(*texture_mag_filter_nearest_action);
    texture_mag_filter_menu->add_action(*texture_mag_filter_linear_action);

    texture_mag_filter_nearest_action->set_checked(true);

    auto help_menu = window->add_menu("&Help"_string);
    help_menu->add_action(GUI::CommonActions::make_command_palette_action(window));
    help_menu->add_action(GUI::CommonActions::make_help_action([&man_file](auto&) {
        Desktop::Launcher::open(URL::create_with_file_scheme(man_file), "/bin/Help");
    }));
    help_menu->add_action(GUI::CommonActions::make_about_action("3D File Viewer"_string, app_icon, window));

    window->show();

    auto file = FileSystemAccessClient::Client::the().request_file_read_only_approved(window, filename);
    if (file.is_error()) {
        if (file.error().code() != ENOENT)
            GUI::MessageBox::show(window, ByteString::formatted("Opening \"{}\" failed: {}", filename, strerror(errno)), "Error"sv, GUI::MessageBox::Type::Error);
        return 1;
    }
    widget->load_file(file.value().filename(), file.value().release_stream());

    return app->exec();
}
