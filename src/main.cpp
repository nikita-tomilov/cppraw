#include <gtkmm.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdint.h>
#include "DoubleImage.h"
#include "DoubleImageLoader.h"

Gtk::ApplicationWindow *pWindow = nullptr;
Gtk::DrawingArea *gImage = nullptr;
Gtk::CheckButton *chbAutoCenter = nullptr;
Gtk::ScaleButton *sldExposure = nullptr;

DoubleImage *doubleImage = nullptr;

uint8_t *static_image_buf_for_preview = new uint8_t[1920 * 1080 * 3];

bool on_imagepane_draw(const Cairo::RefPtr<Cairo::Context> &cr) {
    Gtk::Allocation allocation = gImage->get_allocation();
    const int pane_width = allocation.get_width();
    const int pane_height = allocation.get_height();

    doubleImage->paintOnBuf(static_image_buf_for_preview, pane_width, pane_height);

    Glib::RefPtr<Gdk::Pixbuf> pixbuf = Gdk::Pixbuf::create_from_data(static_image_buf_for_preview,
                                                                     Gdk::COLORSPACE_RGB, false, 8, pane_width,
                                                                     pane_height,
                                                                     pane_width * 3);

    Gdk::Cairo::set_source_pixbuf(cr, pixbuf, 0, 0);
    cr->rectangle(0, 0, pixbuf->get_width(), pixbuf->get_height());
    cr->fill();

    return true;
}

bool on_imagepane_scroll(GdkEventScroll *e) {
    doubleImage->setForceAutoscalingImage(false);
    double image_pane_scale = doubleImage->getImagePaneScale();
    if (e->direction == GDK_SCROLL_UP) {
        image_pane_scale += 0.1;
        if (image_pane_scale > 10) image_pane_scale = 10;
    } else if (e->direction == GDK_SCROLL_DOWN) {
        image_pane_scale -= 0.1;
        if (image_pane_scale < 0.5) image_pane_scale = 0.5;
    }
    doubleImage->setImagePaneScale(image_pane_scale);
    gImage->queue_draw();
    std::cout << "Scale: " << image_pane_scale << std::endl;

    return true;
}

bool on_imagepane_motion(GdkEventMotion *e) {
    double image_pane_mouse_x = e->x;
    double image_pane_mouse_y = e->y;

    int ox = (doubleImage->getImagePaneMouseDownX() - image_pane_mouse_x);
    int oy = (doubleImage->getImagePaneMouseDownY() - image_pane_mouse_y);

    doubleImage->setImagePaneOffsetX(doubleImage->getImagePaneOffsetX() - ox);
    doubleImage->setImagePaneOffsetY(doubleImage->getImagePaneOffsetY() - oy);

    doubleImage->setImagePaneMouseDownX(image_pane_mouse_x);
    doubleImage->setImagePaneMouseDownY(image_pane_mouse_y);

    //std::cout << "offsets " << image_pane_offset_x << " " << image_pane_offset_y << std::endl;

    doubleImage->setImagePaneMouseX(image_pane_mouse_x);
    doubleImage->setImagePaneMouseY(image_pane_mouse_y);

    gImage->queue_draw();
    return true;
}

bool on_imagepane_press_event(GdkEventButton *e) {
    if ((e->type == GDK_BUTTON_PRESS) && (e->button == 1)) {
        doubleImage->setImagePaneMouseDownX(e->x);
        doubleImage->setImagePaneMouseDownY(e->y);
    }
    return true;
}

void on_autocenter_pressed() {
    std::cout << "Autocenter clicked" << std::endl;
    doubleImage->setForceCenteringImage(chbAutoCenter->get_active());
    doubleImage->setForceAutoscalingImage(chbAutoCenter->get_active());
    gImage->queue_draw();
}

void on_slider_changed(double val) {
    double exposureStop = (val - 50.0) / 25.0;
    std::cout << "exposureStop is " << exposureStop << std::endl;
    doubleImage->set_exposure(exposureStop);
    gImage->queue_draw();
}

int main(int argc, char **argv) {

    doubleImage = DoubleImageLoader().load_image();

    auto app = Gtk::Application::create(argc, argv, "org.gtkmm.example");

    auto refBuilder = Gtk::Builder::create();
    try {
        refBuilder->add_from_file("../../test.glade");
    }
    catch (const Glib::FileError &ex) {
        std::cerr << "FileError: " << ex.what() << std::endl;
        return 1;
    }
    catch (const Glib::MarkupError &ex) {
        std::cerr << "MarkupError: " << ex.what() << std::endl;
        return 1;
    }
    catch (const Gtk::BuilderError &ex) {
        std::cerr << "BuilderError: " << ex.what() << std::endl;
        return 1;
    }

    refBuilder->get_widget("applicationwindow1", pWindow);
    if (pWindow) {
        //Get the GtkBuilder-instantiated Button, and connect a signal handler:

        refBuilder->get_widget("gImage", gImage);
        if (gImage) {
            std::cout << "lel" << std::endl;
            gImage->signal_draw().connect(sigc::ptr_fun(on_imagepane_draw));
            gImage->add_events(Gdk::SCROLL_MASK | Gdk::BUTTON1_MOTION_MASK | Gdk::BUTTON_PRESS_MASK |
                               Gdk::POINTER_MOTION_HINT_MASK);
            gImage->signal_scroll_event().connect(sigc::ptr_fun(on_imagepane_scroll));
            gImage->signal_motion_notify_event().connect(sigc::ptr_fun(on_imagepane_motion));
            gImage->signal_button_press_event().connect(sigc::ptr_fun(on_imagepane_press_event));
        }

        refBuilder->get_widget("chbAutoCenter", chbAutoCenter);
        if (chbAutoCenter) {
            chbAutoCenter->signal_toggled().connect(sigc::ptr_fun(on_autocenter_pressed));
        }

        refBuilder->get_widget("sldExposure", sldExposure);
        sldExposure->signal_value_changed().connect(sigc::ptr_fun(on_slider_changed));

        app->run(*pWindow);
    }

    delete pWindow;

    return 0;
}
