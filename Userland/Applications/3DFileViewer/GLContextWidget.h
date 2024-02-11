#pragma once

#include <LibCore/ElapsedTimer.h>
#include <LibFileSystemAccessClient/Client.h>
#include <LibGL/GL/gl.h>
#include <LibGL/GLContext.h>
#include <LibGUI/FilePicker.h>
#include <LibGUI/Frame.h>
#include <LibGUI/Icon.h>
#include <LibGUI/Label.h>
#include <LibGUI/Menubar.h>
#include <LibGUI/MessageBox.h>
#include <LibGUI/Painter.h>
#include <LibGUI/Widget.h>

#include "Scene.h"
#include "SceneLoader.h"

class GLContextWidget final : public GUI::Frame {
    C_OBJECT(GLContextWidget);

public:
    bool load_file(ByteString const& filename, NonnullOwnPtr<Core::File> file);
    void toggle_rotate_x() { m_rotate_x = !m_rotate_x; }
    void toggle_rotate_y() { m_rotate_y = !m_rotate_y; }
    void toggle_rotate_z() { m_rotate_z = !m_rotate_z; }
    void set_rotation_speed(float speed) { m_rotation_speed = speed; }
    void set_stat_label(RefPtr<GUI::Label> l) { m_stats = l; }
    void set_wrap_s_mode(GLint mode) { m_wrap_s_mode = mode; }
    void set_wrap_t_mode(GLint mode) { m_wrap_t_mode = mode; }
    void set_texture_scale(float scale) { m_texture_scale = scale; }
    void set_texture_enabled(bool texture_enabled) { m_texture_enabled = texture_enabled; }
    void set_mag_filter(GLint filter) { m_mag_filter = filter; }
    void toggle_show_frame_rate();

private:
    GLContextWidget();

    virtual void drag_enter_event(GUI::DragEvent&) override;
    virtual void drop_event(GUI::DropEvent&) override;
    virtual void paint_event(GUI::PaintEvent&) override;
    virtual void resize_event(GUI::ResizeEvent&) override;
    virtual void timer_event(Core::TimerEvent&) override;
    virtual void mousemove_event(GUI::MouseEvent&) override;
    virtual void mousewheel_event(GUI::MouseEvent&) override;
    virtual void keydown_event(GUI::KeyEvent&) override;

private:
    static constexpr u32 UPDATE_FRAMERATE_EVERY_FRAMES = 30;

    RefPtr<Scene> m_scene;
    RefPtr<Gfx::Bitmap> m_bitmap;
    OwnPtr<GL::GLContext> m_context;
    OwnPtr<SceneLoader> m_scene_loader;
    Core::ElapsedTimer m_framerate_timer;
    GLuint m_init_list { 0 };
    bool m_rotate_x = true;
    bool m_rotate_y = false;
    bool m_rotate_z = true;
    float m_angle_x = 0.0;
    float m_angle_y = 0.0;
    float m_angle_z = 0.0;
    Gfx::IntPoint m_last_mouse;
    float m_rotation_speed = 60.f;
    bool m_show_frame_rate = false;
    int m_cycles = 0;
    RefPtr<GUI::Label> m_stats;
    GLint m_wrap_s_mode = GL_REPEAT;
    GLint m_wrap_t_mode = GL_REPEAT;
    bool m_texture_enabled { true };
    float m_texture_scale = 1.0f;
    GLint m_mag_filter = GL_NEAREST;
    float m_zoom = 1;
};
