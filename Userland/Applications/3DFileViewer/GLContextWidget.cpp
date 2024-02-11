#include "GLContextWidget.h"
#include "WavefrontOBJLoader.h"

GLContextWidget::GLContextWidget()
{
    constexpr u16 RENDER_WIDTH = 640;
    constexpr u16 RENDER_HEIGHT = 480;
    m_bitmap = Gfx::Bitmap::create(Gfx::BitmapFormat::BGRx8888, { RENDER_WIDTH, RENDER_HEIGHT }).release_value_but_fixme_should_propagate_errors();
    m_context = MUST(GL::create_context(*m_bitmap));
    m_framerate_timer = Core::ElapsedTimer::start_new();

    start_timer(15);

    GL::make_context_current(m_context);
    glFrontFace(GL_CCW);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

    // Enable lighting
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHT1);
    glEnable(GL_LIGHT2);

    // Set projection matrix
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    auto const half_aspect_ratio = static_cast<double>(RENDER_WIDTH) / RENDER_HEIGHT / 2;
    glFrustum(-half_aspect_ratio, half_aspect_ratio, -0.5, 0.5, 1, 1500);

    m_init_list = glGenLists(1);
    glNewList(m_init_list, GL_COMPILE);
    {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClearDepth(1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
    glEndList();
}

void GLContextWidget::drag_enter_event(GUI::DragEvent& event)
{
    auto const& mime_types = event.mime_types();
    if (mime_types.contains_slow("text/uri-list"sv))
        event.accept();
}

void GLContextWidget::drop_event(GUI::DropEvent& event)
{
    if (!event.mime_data().has_urls())
        return;

    event.accept();

    if (event.mime_data().urls().is_empty())
        return;

    for (auto& url : event.mime_data().urls()) {
        if (url.scheme() != "file")
            continue;

        auto response = FileSystemAccessClient::Client::the().request_file_read_only_approved(window(), url.serialize_path());
        if (response.is_error())
            return;
        load_file(response.value().filename(), response.value().release_stream());
    }
}

void GLContextWidget::paint_event(GUI::PaintEvent& event)
{
    GUI::Frame::paint_event(event);

    GUI::Painter painter(*this);
    painter.add_clip_rect(event.rect());
    painter.draw_scaled_bitmap(frame_inner_rect(), *m_bitmap, m_bitmap->rect());
}

void GLContextWidget::resize_event(GUI::ResizeEvent& event)
{
    GUI::Frame::resize_event(event);

    if (m_stats)
        m_stats->set_x(width() - m_stats->width() - 6);
}

void GLContextWidget::mousemove_event(GUI::MouseEvent& event)
{
    if (event.buttons() == GUI::MouseButton::Primary) {
        int delta_x = m_last_mouse.x() - event.x();
        int delta_y = m_last_mouse.y() - event.y();

        m_angle_x -= delta_y / 2.0f;
        m_angle_y -= delta_x / 2.0f;
    }

    m_last_mouse = event.position();
}

void GLContextWidget::mousewheel_event(GUI::MouseEvent& event)
{
    if (event.wheel_delta_y() > 0)
        m_zoom /= 1.1f;
    else
        m_zoom *= 1.1f;
}

void GLContextWidget::keydown_event(GUI::KeyEvent& event)
{
    if (event.key() == Key_Escape && window()->is_fullscreen()) {
        window()->set_fullscreen(false);
        return;
    }

    event.ignore();
}

void GLContextWidget::timer_event(Core::TimerEvent&)
{
    static unsigned int light_counter = 0;

    glCallList(m_init_list);

    if (m_rotate_x)
        m_angle_x -= m_rotation_speed * 0.01f;
    if (m_rotate_y)
        m_angle_y -= m_rotation_speed * 0.01f;
    if (m_rotate_z)
        m_angle_z -= m_rotation_speed * 0.01f;
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0, 0, -8.5);
    glRotatef(m_angle_x, 1, 0, 0);
    glRotatef(m_angle_y, 0, 1, 0);
    glRotatef(m_angle_z, 0, 0, 1);

    glPushMatrix();
    glLoadIdentity();
    // Disco time ;)
    GLfloat const light0_position[4] = { -4.0f, 0.0f, 0.0f, 0.0f };
    GLfloat const light0_diffuse[4] = { 1.0f, 0.0f, 0.0f, 0.0f };
    GLfloat const light0_specular[4] = { 0.75f, 0.75f, 0.75f };
    GLfloat const light1_position[4] = { 4.0f, 0.0f, 0.0f, 0.0f };
    GLfloat const light1_diffuse[4] = { 0.0f, 1.0f, 0.0f, 0.0f };
    GLfloat const light1_specular[4] = { 0.75f, 0.75f, 0.75f };
    GLfloat const light2_position[4] = { 0.0f, 5.0f, 0.0f, 0.0f };
    GLfloat const light2_diffuse[4] = { 0.0f, 0.0f, 1.0f, 0.0f };
    GLfloat const light2_specular[4] = { 0.75f, 0.75f, 0.75f };
    glLightfv(GL_LIGHT0, GL_POSITION, &light0_position[0]);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, &light0_diffuse[0]);
    glLightfv(GL_LIGHT0, GL_SPECULAR, &light0_specular[0]);
    glLightfv(GL_LIGHT1, GL_POSITION, &light1_position[0]);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, &light1_diffuse[0]);
    glLightfv(GL_LIGHT1, GL_SPECULAR, &light1_specular[0]);
    glLightfv(GL_LIGHT2, GL_POSITION, &light2_position[0]);
    glLightfv(GL_LIGHT2, GL_DIFFUSE, &light2_diffuse[0]);
    glLightfv(GL_LIGHT2, GL_SPECULAR, &light2_specular[0]);

    GLfloat const material_specular_color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glMaterialf(GL_FRONT, GL_SHININESS, 45.0f);
    glMaterialfv(GL_FRONT, GL_SPECULAR, &material_specular_color[0]);
    glPopMatrix();

    if (m_texture_enabled) {
        glEnable(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, m_wrap_s_mode);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, m_wrap_t_mode);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, m_mag_filter);
    } else {
        glDisable(GL_TEXTURE_2D);
    }
    glScalef(m_zoom, m_zoom, m_zoom);

    if (m_scene)
        m_scene->draw(m_texture_scale);

    m_context->present();

    if ((m_cycles % UPDATE_FRAMERATE_EVERY_FRAMES) == 0) {
        auto render_time = static_cast<double>(m_framerate_timer.elapsed_milliseconds()) / UPDATE_FRAMERATE_EVERY_FRAMES;
        auto frame_rate = render_time > 0 ? 1000 / render_time : 0;
        m_stats->set_text(String::formatted("{:.0f} fps, {:.1f} ms", frame_rate, render_time).release_value_but_fixme_should_propagate_errors());
        m_framerate_timer = Core::ElapsedTimer::start_new();

        glEnable(GL_LIGHT0);
        glEnable(GL_LIGHT1);
        glEnable(GL_LIGHT2);
        light_counter++;

        if ((light_counter % 3) == 0)
            glDisable(GL_LIGHT0);
        else if ((light_counter % 3) == 1)
            glDisable(GL_LIGHT1);
        else
            glDisable(GL_LIGHT2);
    }

    update();
    m_cycles++;
}

bool GLContextWidget::load_file(ByteString const& filename, NonnullOwnPtr<Core::File> file)
{
    if (filename.ends_with(".obj"sv)) {
        m_scene_loader = adopt_own(*new WavefrontOBJLoader());
    } else {
        GUI::MessageBox::show(window(), ByteString::formatted("Opening \"{}\" failed: invalid file type", filename), "Error"sv, GUI::MessageBox::Type::Error);
        return false;
    }

    auto new_scene = m_scene_loader->load(filename, move(file));
    if (new_scene.is_error()) {
        GUI::MessageBox::show(window(), ByteString::formatted("Reading \"{}\" failed: {}", filename, new_scene.release_error()), "Error"sv, GUI::MessageBox::Type::Error);
        return false;
    }

    // Determine whether or not a texture for this model resides within the same directory
    StringBuilder builder;
    builder.append(filename.split_view('.').at(0));
    builder.append(".bmp"sv);

    // Attempt to open the texture file from disk
    RefPtr<Gfx::Bitmap> texture_image;
    auto response = FileSystemAccessClient::Client::the().request_file_read_only_approved(window(), builder.string_view());
    if (!response.is_error()) {
        auto texture_file = response.release_value();
        auto bitmap_or_error = Gfx::Bitmap::load_from_file(texture_file.release_stream(), texture_file.filename());
        if (!bitmap_or_error.is_error())
            texture_image = bitmap_or_error.release_value_but_fixme_should_propagate_errors();
    }

    GLuint tex;
    glGenTextures(1, &tex);
    if (texture_image) {
        // Upload texture data to the GL
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texture_image->width(), texture_image->height(), 0, GL_BGRA, GL_UNSIGNED_BYTE, texture_image->scanline(0));
    } else {
        dbgln("3DFileViewer: Couldn't load texture for {}", filename);
    }

    m_scene = new_scene.release_value();
    dbgln("3DFileViewer: mesh has {} triangles.", m_scene->triangle_count());

    window()->set_title(ByteString::formatted("{} - 3D File Viewer", filename));

    return true;
}

void GLContextWidget::toggle_show_frame_rate()
{
    m_show_frame_rate = !m_show_frame_rate;
    m_stats->set_visible(m_show_frame_rate);
}
