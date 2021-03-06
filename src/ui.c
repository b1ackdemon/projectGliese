#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkcursor.h>

#if defined (GDK_WINDOWING_X11)
#include <gdk/gdkx.h>
#elif defined (GDK_WINDOWING_WIN32)
#include <gdk/gdkwin32.h>
#elif defined (GDK_WINDOWING_QUARTZ)
#include <gdk/gdkquartz.h>
#endif

#include "gst-backend.h"
#include "ui.h"

typedef struct _OpenMenu {
    GtkWidget* openMenu;
    GtkWidget* OpenMi;
    GtkWidget* fileMi;
    GtkWidget* closeMi;
    GtkWidget* exitMi;
} OpenMenu;

typedef struct _VideoMenu {
    GtkWidget* videoMenu;
    GtkWidget* videoMi;
    GtkWidget* trackMi;
    GtkWidget* colorBalanceMi;
} VideoMenu;

typedef struct _AudioMenu {
    GtkWidget* audioMenu;
    GtkWidget* trackMenu;
    GtkWidget* audioMi;
    GtkWidget* trackMi;
} AudioMenu;

typedef struct _SubtitlesMenu {
    GtkWidget* subtitlesMenu;
    GtkWidget* subtitlesMi;
    GtkWidget* primaryTrackMi;
    GtkWidget* secondaryTrackMi;
} SubtitlesMenu;

typedef struct _ViewMenu {
    GtkWidget* viewMenu;
    GtkWidget* viewMi;
    GtkWidget* informationMi;
} ViewMenu;

typedef struct _OptionsMenu {
    GtkWidget* optionsMenu;
    GtkWidget* optionsMi;
    GtkWidget* preferencesMi;
} OptionsMenu;

typedef struct _HelpMenu {
    GtkWidget* helpMenu;
    GtkWidget* helpMi;
    GtkWidget* aboutMi;
} HelpMenu;

typedef struct _Menubar {
    GtkWidget*    menubar;
    OpenMenu      openMenu;
    VideoMenu     videoMenu;
    AudioMenu     audioMenu;
    SubtitlesMenu subtitlesMenu;
    ViewMenu      viewMenu;
    OptionsMenu   optionsMenu;
    HelpMenu      helpMenu;
} Menubar;

typedef struct _UiWidgets {
    GtkWidget* window;
    GtkWidget* videoWindow;
    GtkWidget* playButton;
    GtkWidget* stopButton;
    GtkWidget* volumeButton;
    GtkWidget* fullscreenButton;
    GtkWidget* slider;
    GtkWidget* duration;
    GtkWidget* position;
    gulong     sliderUpdateSignalId;
} UiWidgets;

typedef struct _FullUiWidgets {
    GtkWidget* position;
    GtkWidget* duration;
    GtkWidget* fullscreenSlider;
    gulong fullScreenSliderId;
} FullUiWidgets;

static UiWidgets uiWidgets;
static Menubar menubar;
static FullUiWidgets fullUiWidgets;
static gboolean isPlaying = FALSE;
static GtkWidget* revealer = NULL;
static gulong motionSignalId;
static GtkWidget* videoWindow = NULL;

int createOpenMenu        (OpenMenu* openMenu,           GtkWidget* menubar);
int createVideoMenu       (VideoMenu* videoMenu,         GtkWidget* menubar);
int createAudioMenu       (AudioMenu* audioMenu,         GtkWidget* menubar);
int createSubtitlesMenu   (SubtitlesMenu* subtitlesMenu, GtkWidget* menubar);
int createViewMenu        (ViewMenu* viewMenu,           GtkWidget* menubar);
int createOptionsMenu     (OptionsMenu* optionsMenu,     GtkWidget* menubar);
int createHelpMenu        (HelpMenu* helpMenu,           GtkWidget* menubar);
int createUi (GtkWidget* window);
int createMenubar (Menubar* menubar);
int createWindow (const char* name, int width, int height);
static void createContext (GtkWidget* widget);
void createAboutDialog();
void createInformationWindow();
void createColorBalanceWindow();
void refreshPositionLabel (GtkWidget* positionLabel);
void refreshDurationLabel (GtkWidget* durationLabel);
void hideControls();

static void play_cb              (GtkButton* button,       gpointer data);
static void stop_cb              (GtkButton* button,       gpointer data);
static void slider_cb            (GtkRange*  range,        gpointer data);
static void volume_cb            (GtkRange*  volumeButton, gpointer data);
static void fullscreen_cb        (GtkWidget* button,       gpointer data);
static void fullscreenRealize_cb (GtkWidget* widget,       gpointer data);
static void overlayFullscreen_cb (GtkWidget* widget, GtkWindow* mainWindow);
static void fileMenu_cb  (GtkWidget* widget);
static void closeMenu_cb (GtkWidget* widget);
static void exitMenu_cb  (GtkWidget* widget);
static void deleteEvent_cb (GtkWidget* widget, GdkEvent* event, gpointer data);
static void fullSlider_cb (GtkRange* range, gpointer data);
static void motionNotify_cb (GtkWidget* widget, gpointer data);
static void aboutMenu_cb (GtkWidget* widget, gpointer data);
static void informationMenu_cb (GtkWidget* widget, gpointer data);
static void colorBalanceMenu_cb (GtkWidget* widget, gpointer data);
static void contrast_cb (GtkRange* range, gpointer data);
static void brightness_cb (GtkRange* range, gpointer data);
static void saturation_cb (GtkRange* range, gpointer data);
static void hue_cb (GtkRange* range, gpointer data);

void stringReplace (char* str, char rep, char with);
char* getFileName(char* str);

int main (int argc, char **argv) {
    gtk_init (&argc, &argv);
    backendInit (&argc, &argv);


    createWindow ("ProjectGliese", 800, 535);

    fullUiWidgets.fullscreenSlider = NULL;

    g_timeout_add_seconds (1, (GSourceFunc) refreshUi, NULL);

    /* Start the GTK main loop. */
    gtk_main();

    if (isPlaying) {
        backendDeInit();
    }
    return 0;
}
int createWindow (const char* name, int width, int height) {
    uiWidgets.window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_position (GTK_WINDOW (uiWidgets.window), GTK_WIN_POS_CENTER);
    gtk_window_set_default_size (GTK_WINDOW (uiWidgets.window), width, height);
    gtk_window_set_title (GTK_WINDOW (uiWidgets.window), name);
    g_signal_connect (uiWidgets.window, "delete-event",
            G_CALLBACK(deleteEvent_cb), NULL);
    createUi (uiWidgets.window);
    gtk_widget_show_all (uiWidgets.window);
    return 0;
}

int createUi (GtkWidget* window) {
    GtkWidget* controls;
    GtkWidget* mainBox;

    uiWidgets.videoWindow  = gtk_drawing_area_new();
    uiWidgets.playButton   = gtk_button_new_from_icon_name ("media-playback-start",
            GTK_ICON_SIZE_BUTTON);
    uiWidgets.stopButton   = gtk_button_new_from_icon_name ("media-playback-stop",
            GTK_ICON_SIZE_BUTTON);
    uiWidgets.volumeButton = gtk_volume_button_new();
    gtk_scale_button_set_value (GTK_SCALE_BUTTON (uiWidgets.volumeButton), 1.0);

    uiWidgets.fullscreenButton = gtk_button_new_from_icon_name ("view-fullscreen",
            GTK_ICON_SIZE_BUTTON);

    uiWidgets.slider = gtk_scale_new_with_range (GTK_ORIENTATION_HORIZONTAL, 0, 100, 1);
    gtk_scale_set_draw_value (GTK_SCALE (uiWidgets.slider), 0);

    uiWidgets.position = gtk_label_new ("0:00:00");
    uiWidgets.duration = gtk_label_new ("0:00:00");

    createMenubar (&menubar);

    controls = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start (GTK_BOX (controls), uiWidgets.playButton, FALSE, FALSE, 2);
    gtk_box_pack_start (GTK_BOX (controls), uiWidgets.stopButton, FALSE, FALSE, 2);
    gtk_box_pack_start (GTK_BOX (controls), uiWidgets.volumeButton, FALSE, FALSE, 2);
    gtk_box_pack_start (GTK_BOX (controls), uiWidgets.position, FALSE, FALSE, 2);
    gtk_box_pack_start (GTK_BOX (controls), uiWidgets.slider, TRUE, TRUE, 2);
    gtk_box_pack_start (GTK_BOX (controls), uiWidgets.duration, FALSE, FALSE, 2);
    gtk_box_pack_start (GTK_BOX (controls), uiWidgets.fullscreenButton, FALSE, FALSE, 2);

    mainBox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_pack_start (GTK_BOX (mainBox), menubar.menubar, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (mainBox), uiWidgets.videoWindow, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (mainBox), controls, FALSE, FALSE, 5);
    gtk_container_add (GTK_CONTAINER (window), mainBox);


    return 0;
}

gboolean refreshUi() {
    gdouble current  = -1;
    gdouble duration = 0;

    if (!backendIsPausedOrPlaying()) {
        return TRUE;
    }

    if (!backendDurationIsValid()) {
        duration = backendQueryDuration();
        gtk_range_set_range (GTK_RANGE (uiWidgets.slider), 0, duration);

        refreshDurationLabel (uiWidgets.duration);
    }

    if (backendQueryPosition (&current)) {
        /* Block the "value-changed" signal, so the slider_cb function is not called
         * (which would trigger a seek the user has not requested) */
        g_signal_handler_block (uiWidgets.slider, uiWidgets.sliderUpdateSignalId);
        /* Set the position of the slider to the current pipeline position, in SECONDS */
        gtk_range_set_value (GTK_RANGE (uiWidgets.slider), current);
        /* Re-enable the signal */
        g_signal_handler_unblock (uiWidgets.slider, uiWidgets.sliderUpdateSignalId);

        refreshPositionLabel (uiWidgets.position);
    }

    if (fullUiWidgets.fullscreenSlider != NULL) {
        if (backendDurationIsValid()) {
            duration = backendQueryDuration();
            gtk_range_set_range (GTK_RANGE (fullUiWidgets.fullscreenSlider), 0, duration);

            refreshDurationLabel (fullUiWidgets.duration);
        }

        if (backendQueryPosition (&current)) {
            g_signal_handler_block (fullUiWidgets.fullscreenSlider, fullUiWidgets.fullScreenSliderId);
            gtk_range_set_value (GTK_RANGE (fullUiWidgets.fullscreenSlider), current);
            g_signal_handler_unblock (fullUiWidgets.fullscreenSlider, fullUiWidgets.fullScreenSliderId);

            refreshPositionLabel (fullUiWidgets.position);
        }
    }

    return TRUE;
}

int createMenubar (Menubar* bar) {
    bar->menubar = gtk_menu_bar_new();
    createOpenMenu      (&bar->openMenu,      bar->menubar);
    createVideoMenu     (&bar->videoMenu,     bar->menubar);
    createAudioMenu     (&bar->audioMenu,     bar->menubar);
    createSubtitlesMenu (&bar->subtitlesMenu, bar->menubar);
    createViewMenu      (&bar->viewMenu,      bar->menubar);
    createOptionsMenu   (&bar->optionsMenu,   bar->menubar);
    createHelpMenu      (&bar->helpMenu,      bar->menubar);
    return 0;
}

int createOpenMenu (OpenMenu* openMenu, GtkWidget* bar) {
    openMenu->openMenu = gtk_menu_new();

    openMenu->OpenMi   =
            gtk_menu_item_new_with_label ("Open");
    openMenu->fileMi   =
            gtk_menu_item_new_with_label ("File");
    g_signal_connect (openMenu->fileMi, "activate", G_CALLBACK (fileMenu_cb), NULL);
    openMenu->closeMi  =
            gtk_menu_item_new_with_label ("Close");
    g_signal_connect (openMenu->closeMi, "activate", G_CALLBACK (closeMenu_cb), NULL);
    openMenu->exitMi   =
            gtk_menu_item_new_with_label ("Exit");
    g_signal_connect (openMenu->exitMi, "activate", G_CALLBACK (exitMenu_cb), NULL);

    gtk_menu_item_set_submenu (GTK_MENU_ITEM (openMenu->OpenMi),
            openMenu->openMenu);
    gtk_menu_shell_append (GTK_MENU_SHELL (openMenu->openMenu),
            openMenu->fileMi);
    gtk_menu_shell_append (GTK_MENU_SHELL (openMenu->openMenu),
            openMenu->closeMi);
    gtk_menu_shell_append (GTK_MENU_SHELL (openMenu->openMenu), openMenu->exitMi);
    gtk_menu_shell_append (GTK_MENU_SHELL (bar), openMenu->OpenMi);
    return 0;
}

int createVideoMenu (VideoMenu* videoMenu, GtkWidget* bar) {
    videoMenu->videoMenu      = gtk_menu_new();

    videoMenu->videoMi        =
            gtk_menu_item_new_with_label ("Video");
    videoMenu->trackMi        =
            gtk_menu_item_new_with_label ("Track");

    videoMenu->colorBalanceMi =
            gtk_menu_item_new_with_label ("Color balance");
    g_signal_connect (videoMenu->colorBalanceMi,
                      "activate", G_CALLBACK(colorBalanceMenu_cb), NULL);

    gtk_menu_item_set_submenu (GTK_MENU_ITEM (videoMenu->videoMi),
            videoMenu->videoMenu);
    gtk_menu_shell_append (GTK_MENU_SHELL (videoMenu->videoMenu),
            videoMenu->trackMi);
    gtk_menu_shell_append (GTK_MENU_SHELL (videoMenu->videoMenu),
                           videoMenu->colorBalanceMi);
    gtk_menu_shell_append (GTK_MENU_SHELL (bar), videoMenu->videoMi);
    return 0;
}

int createAudioMenu (AudioMenu* audioMenu, GtkWidget* bar) {
    audioMenu->audioMenu = gtk_menu_new();

    audioMenu->audioMi   =
            gtk_menu_item_new_with_label ("Audio");
    audioMenu->trackMi   =
            gtk_menu_item_new_with_label ("Track");

    audioMenu->trackMenu = gtk_menu_new();
    gtk_menu_item_set_submenu (GTK_MENU_ITEM (audioMenu->trackMi), audioMenu->trackMenu);

    gtk_menu_item_set_submenu (GTK_MENU_ITEM (audioMenu->audioMi),
            audioMenu->audioMenu);
    gtk_menu_shell_append (GTK_MENU_SHELL (audioMenu->audioMenu),
            audioMenu->trackMi);
    gtk_menu_shell_append (GTK_MENU_SHELL(bar),
            audioMenu->audioMi);

    GtkWidget* track1 = gtk_menu_item_new_with_label("Track 1");
    GtkWidget* track2 = gtk_menu_item_new_with_label("Track 2");

    gtk_menu_shell_append (GTK_MENU_SHELL (menubar.audioMenu.trackMenu), track1);
    gtk_menu_shell_append (GTK_MENU_SHELL (menubar.audioMenu.trackMenu), track2);
    return 0;
}

int createSubtitlesMenu (SubtitlesMenu* subtitlesMenu, GtkWidget* bar) {
    subtitlesMenu->subtitlesMenu    = gtk_menu_new();

    subtitlesMenu->subtitlesMi      =
            gtk_menu_item_new_with_label ("Subtitles");
    subtitlesMenu->primaryTrackMi   =
            gtk_menu_item_new_with_label ("Primary track");
    subtitlesMenu->secondaryTrackMi =
            gtk_menu_item_new_with_label ("Secondary track");

    gtk_menu_item_set_submenu (GTK_MENU_ITEM (subtitlesMenu->subtitlesMi),
            subtitlesMenu->subtitlesMenu);
    gtk_menu_shell_append (GTK_MENU_SHELL (subtitlesMenu->subtitlesMenu),
            subtitlesMenu->primaryTrackMi);
    gtk_menu_shell_append (GTK_MENU_SHELL (subtitlesMenu->subtitlesMenu),
            subtitlesMenu->secondaryTrackMi);
    gtk_menu_shell_append (GTK_MENU_SHELL(bar),
            subtitlesMenu->subtitlesMi);
    return 0;
}

int createViewMenu (ViewMenu* viewMenu, GtkWidget* bar) {
    viewMenu->viewMenu = gtk_menu_new();

    viewMenu->viewMi   = gtk_menu_item_new_with_label ("View");
    viewMenu->informationMi =
            gtk_menu_item_new_with_label ("Information and properties");
    g_signal_connect (viewMenu->informationMi,
            "activate", G_CALLBACK(informationMenu_cb), NULL);

    gtk_menu_item_set_submenu (GTK_MENU_ITEM (viewMenu->viewMi),
            viewMenu->viewMenu);
    gtk_menu_shell_append (GTK_MENU_SHELL (viewMenu->viewMenu),
            viewMenu->informationMi);
    gtk_menu_shell_append (GTK_MENU_SHELL (bar), viewMenu->viewMi);
    return 0;
}

int createOptionsMenu (OptionsMenu* optionsMenu, GtkWidget* bar) {
    optionsMenu->optionsMenu   = gtk_menu_new();

    optionsMenu->optionsMi     =
            gtk_menu_item_new_with_label ("Options");
    optionsMenu->preferencesMi =
            gtk_menu_item_new_with_label ("Preferences");

    gtk_menu_item_set_submenu (GTK_MENU_ITEM (optionsMenu->optionsMi),
            optionsMenu->optionsMenu);
    gtk_menu_shell_append (GTK_MENU_SHELL (optionsMenu->optionsMenu),
            optionsMenu->preferencesMi);
    gtk_menu_shell_append (GTK_MENU_SHELL (bar),
            optionsMenu->optionsMi);
    return 0;
}

int createHelpMenu (HelpMenu* helpMenu, GtkWidget* bar) {
    helpMenu->helpMenu = gtk_menu_new();

    helpMenu->helpMi  = gtk_menu_item_new_with_label("Help");
    helpMenu->aboutMi = gtk_menu_item_new_with_label("About");
    g_signal_connect (helpMenu->aboutMi, "activate", G_CALLBACK (aboutMenu_cb), NULL);

    gtk_menu_item_set_submenu (GTK_MENU_ITEM (helpMenu->helpMi),
            helpMenu->helpMenu);
    gtk_menu_shell_append (GTK_MENU_SHELL (helpMenu->helpMenu),
            helpMenu->aboutMi);
    gtk_menu_shell_append (GTK_MENU_SHELL(bar), helpMenu->helpMi);
    return 0;
}

static void createContext (GtkWidget* widget) {
    GdkWindow* window = gtk_widget_get_window (widget);
    guintptr window_handle;

    if (!gdk_window_ensure_native (window)) {
        g_error ("Couldn't create native window needed for GstVideoOverlay!");
    }

#if defined (GDK_WINDOWING_WIN32)
    window_handle = (guintptr)GDK_WINDOW_HWND (window);
#elif defined (GDK_WINDOWING_QUARTZ)
    window_handle = gdk_quartz_window_get_nsview (window);
#elif defined (GDK_WINDOWING_X11)
    window_handle = GDK_WINDOW_XID (window);
#endif
    backendSetWindow (window_handle);
}

void createInformationWindow() {
    if (isPlaying) {
        GtkWidget* informationWindow = gtk_window_new (GTK_WINDOW_TOPLEVEL);
        gtk_window_set_position (GTK_WINDOW (informationWindow), GTK_WIN_POS_CENTER);
        gtk_window_set_title (GTK_WINDOW (informationWindow), "File properties");
        gtk_window_set_modal (GTK_WINDOW (informationWindow), TRUE);
        gtk_window_set_default_size (GTK_WINDOW (informationWindow), 400, 300);

        GtkTextBuffer* textBuffer = gtk_text_buffer_new (NULL);
        backendGetInformationAboutStreams(textBuffer);

        GtkWidget* textView = gtk_text_view_new_with_buffer (textBuffer);
        gtk_text_view_set_editable (GTK_TEXT_VIEW (textView), FALSE);
        gtk_container_add (GTK_CONTAINER (informationWindow), textView);

        gtk_widget_show_all (informationWindow);
    }
}

void createColorBalanceWindow() {
    if (isPlaying) {
        GtkWidget* colBalWindow = gtk_window_new (GTK_WINDOW_TOPLEVEL);
        gtk_window_set_position (GTK_WINDOW (colBalWindow), GTK_WIN_POS_CENTER);
        gtk_window_set_title (GTK_WINDOW (colBalWindow), "Color balance");
        gtk_window_set_modal (GTK_WINDOW (colBalWindow), TRUE);
        gtk_window_set_default_size (GTK_WINDOW (colBalWindow), 500, 400);

        GtkWidget* contrastLabel = gtk_label_new ("Contrast");

        GtkWidget* contrastSlider = gtk_scale_new_with_range (GTK_ORIENTATION_HORIZONTAL,
                -1000, 1000, 1);
        g_signal_connect (contrastSlider, "value-changed",
                          G_CALLBACK (contrast_cb), NULL);

        gdouble contrastValue;
        backendGetColorBalance ("CONTRAST", &contrastValue);
        gtk_range_set_value (GTK_RANGE (contrastSlider), contrastValue);

        GtkWidget* brightnessLabel = gtk_label_new ("Brightness");

        GtkWidget* brightnessSlider = gtk_scale_new_with_range (GTK_ORIENTATION_HORIZONTAL,
                -1000, 1000, 1);
        g_signal_connect (brightnessSlider, "value-changed",
                          G_CALLBACK (brightness_cb), NULL);

        gdouble brightnessValue;
        backendGetColorBalance ("BRIGHTNESS", &brightnessValue);
        gtk_range_set_value (GTK_RANGE (brightnessSlider), brightnessValue);

        GtkWidget* hueLabel = gtk_label_new ("Hue");

        GtkWidget* hueSlider = gtk_scale_new_with_range (GTK_ORIENTATION_HORIZONTAL,
                -1000, 1000, 1);
        g_signal_connect (hueSlider, "value-changed",
                          G_CALLBACK (hue_cb), NULL);

        gdouble hueValue;
        backendGetColorBalance ("HUE", &hueValue);
        gtk_range_set_value (GTK_RANGE (hueSlider), hueValue);

        GtkWidget* saturationLabel = gtk_label_new ("Saturation");

        GtkWidget* saturationSlider = gtk_scale_new_with_range (GTK_ORIENTATION_HORIZONTAL,
                -1000, 1000, 1);
        g_signal_connect (saturationSlider, "value-changed",
                          G_CALLBACK (saturation_cb), NULL);

        gdouble saturationValue;
        backendGetColorBalance ("SATURATION", &saturationValue);
        gtk_range_set_value (GTK_RANGE (saturationSlider), saturationValue);

        GtkWidget* contrastBox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
        gtk_box_pack_start (GTK_BOX (contrastBox), contrastLabel, FALSE, FALSE, 0);
        gtk_box_pack_start (GTK_BOX (contrastBox), contrastSlider, TRUE, TRUE, 20);


        GtkWidget* brightnessBox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
        gtk_box_pack_start (GTK_BOX (brightnessBox), brightnessLabel, FALSE, FALSE, 0);
        gtk_box_pack_start (GTK_BOX (brightnessBox), brightnessSlider, TRUE, TRUE, 20);

        GtkWidget* hueBox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
        gtk_box_pack_start (GTK_BOX (hueBox), hueLabel, FALSE, FALSE, 0);
        gtk_box_pack_start (GTK_BOX (hueBox), hueSlider, TRUE, TRUE, 20);

        GtkWidget* saturationBox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
        gtk_box_pack_start (GTK_BOX (saturationBox), saturationLabel, FALSE, FALSE, 0);
        gtk_box_pack_start (GTK_BOX (saturationBox), saturationSlider, TRUE, TRUE, 20);

        GtkWidget* mainBox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
        gtk_box_pack_start (GTK_BOX (mainBox), contrastBox, FALSE, FALSE, 0);
        gtk_box_pack_start (GTK_BOX (mainBox), brightnessBox, FALSE, FALSE, 0);
        gtk_box_pack_start (GTK_BOX (mainBox), hueBox, FALSE, FALSE, 0);
        gtk_box_pack_start (GTK_BOX (mainBox), saturationBox, FALSE, FALSE, 0);
        gtk_container_add (GTK_CONTAINER (colBalWindow), mainBox);
        gtk_container_set_border_width (GTK_CONTAINER(colBalWindow), 30);
        gtk_widget_show_all (colBalWindow);
    }
}

void createAboutDialog() {
    GtkWidget* aboutWindow = gtk_about_dialog_new();

    gtk_about_dialog_set_program_name (GTK_ABOUT_DIALOG (aboutWindow), "Project Gliese");
    gtk_about_dialog_set_version (GTK_ABOUT_DIALOG (aboutWindow), "0.14159265358979");

    gtk_about_dialog_set_license (GTK_ABOUT_DIALOG (aboutWindow), "GPL-3.0");
    gtk_about_dialog_set_license_type (GTK_ABOUT_DIALOG (aboutWindow), GTK_LICENSE_GPL_3_0_ONLY);

    gtk_about_dialog_set_website (GTK_ABOUT_DIALOG (aboutWindow),
            "https://github.com/b1ackdemon/projectGliese");
    gtk_about_dialog_set_website_label (GTK_ABOUT_DIALOG (aboutWindow), "Github");

    gtk_about_dialog_set_comments (GTK_ABOUT_DIALOG (aboutWindow), "A simple media player.");

    gtk_widget_show_all (aboutWindow);
}

void refreshPositionLabel (GtkWidget* positionLabel) {
    const gint size = 30;

    gchar* positionText = (gchar*) malloc (sizeof (gchar) * size);
    backendGetPosition (positionText);
    gtk_label_set_label (GTK_LABEL (positionLabel), positionText);
    g_free (positionText);
}

void refreshDurationLabel (GtkWidget* durationLabel) {
    const gint size = 30;

    gchar* durationText = (gchar*) malloc (sizeof (gchar) * size);
    backendGetDuration (durationText);
    gtk_label_set_label (GTK_LABEL (durationLabel), durationText);
    g_free (durationText);
}

void hideControls () {
    GdkWindow* window = gtk_widget_get_parent_window(videoWindow);
    GdkDisplay* display = gdk_display_get_default();
    GdkCursor* watchCursor = gdk_cursor_new_for_display (display, GDK_BLANK_CURSOR);
    gdk_window_set_cursor(window, watchCursor);

    gtk_revealer_set_reveal_child (GTK_REVEALER (revealer), FALSE);
    g_signal_handler_unblock (videoWindow, motionSignalId);
}

static void play_cb (GtkButton* button, gpointer data) {
    UNUSED (data);

    if (!backendIsPlaying()) {
        GtkWidget* icon = gtk_image_new_from_icon_name("media-playback-pause", GTK_ICON_SIZE_BUTTON);
        gtk_button_set_image (button, icon);
        backendResume();
    } else {
        GtkWidget* icon = gtk_image_new_from_icon_name("media-playback-start", GTK_ICON_SIZE_BUTTON);
        gtk_button_set_image (button, icon);
        backendPause();
    }
}

static void stop_cb (GtkButton* button, gpointer data) {
    UNUSED (button);
    UNUSED (data);

    backendStop();
}

static void slider_cb (GtkRange* range, gpointer data) {
    UNUSED (data);

    gdouble value = gtk_range_get_value (GTK_RANGE (range));
    backendSeek (value);
}

static void fullSlider_cb (GtkRange* range, gpointer data) {
    UNUSED (data);

    gdouble value = gtk_range_get_value(GTK_RANGE (range));
    backendSeek(value);
}

static void volume_cb (GtkRange* volumeButton, gpointer data) {
    UNUSED (data);

    gdouble value = gtk_scale_button_get_value (GTK_SCALE_BUTTON (volumeButton));
    backendSetVolume (value);
}

static void fullscreen_cb (GtkWidget* button, gpointer data) {
    UNUSED (data);

    GtkWidget* controls;
    GtkWindow* parentWindow = GTK_WINDOW (gtk_widget_get_toplevel(button));

    GtkWidget* fullscreenWindow = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_position (GTK_WINDOW (fullscreenWindow), GTK_WIN_POS_CENTER);
    gtk_window_set_title (GTK_WINDOW (fullscreenWindow), "ProjectGliese");
    GtkWidget* rootPane = gtk_overlay_new();
    gtk_container_add (GTK_CONTAINER (fullscreenWindow), rootPane);

    videoWindow = gtk_drawing_area_new();
    gtk_container_add (GTK_CONTAINER (rootPane), videoWindow);
    g_signal_connect (videoWindow, "realize",
            G_CALLBACK (fullscreenRealize_cb), NULL);
    gtk_widget_add_events (videoWindow, GDK_POINTER_MOTION_MASK);

    GtkWidget* playButton  = gtk_button_new_from_icon_name("media-playback-pause",
            GTK_ICON_SIZE_BUTTON);
    g_signal_connect (playButton, "clicked",
            G_CALLBACK (play_cb), NULL);

    GtkWidget* stopButton   = gtk_button_new_from_icon_name("media-playback-stop",
            GTK_ICON_SIZE_BUTTON);
    g_signal_connect (stopButton, "clicked",
            G_CALLBACK (stop_cb), NULL);

    GtkWidget* volumeButton = gtk_volume_button_new();
    g_signal_connect (volumeButton, "value-changed",
            G_CALLBACK (volume_cb), NULL);
    gtk_scale_button_set_value (GTK_SCALE_BUTTON (volumeButton), backendGetVolume());

    fullUiWidgets.position = gtk_label_new ("0:00:00");

    fullUiWidgets.fullscreenSlider = gtk_scale_new_with_range (GTK_ORIENTATION_HORIZONTAL,
            0, 100, 1);
    gtk_scale_set_draw_value (GTK_SCALE (fullUiWidgets.fullscreenSlider), 0);
    fullUiWidgets.fullScreenSliderId = g_signal_connect (fullUiWidgets.fullscreenSlider,
            "value-changed", G_CALLBACK (fullSlider_cb), NULL);

    fullUiWidgets.duration = gtk_label_new ("0:00:00");

    GtkWidget* fullscreenButton = gtk_button_new_from_icon_name ("view-fullscreen",
            GTK_ICON_SIZE_BUTTON);
    g_signal_connect (fullscreenButton, "clicked",
            G_CALLBACK (overlayFullscreen_cb), parentWindow);

    controls = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start (GTK_BOX (controls), playButton,                     FALSE, FALSE, 2);
    gtk_box_pack_start (GTK_BOX (controls), stopButton,                     FALSE, FALSE, 2);
    gtk_box_pack_start (GTK_BOX (controls), volumeButton,                   FALSE, FALSE, 2);
    gtk_box_pack_start (GTK_BOX (controls), fullUiWidgets.position,         FALSE, FALSE, 2);
    gtk_box_pack_start (GTK_BOX (controls), fullUiWidgets.fullscreenSlider, TRUE,  TRUE,  2);
    gtk_box_pack_start (GTK_BOX (controls), fullUiWidgets.duration,         FALSE, FALSE, 2);
    gtk_box_pack_start (GTK_BOX (controls), fullscreenButton,               FALSE, FALSE, 2);
    gtk_container_set_border_width (GTK_CONTAINER (controls), 3);

    revealer = gtk_revealer_new();
    gtk_revealer_set_transition_type (GTK_REVEALER (revealer), GTK_REVEALER_TRANSITION_TYPE_SLIDE_UP);
    gtk_revealer_set_transition_duration (GTK_REVEALER (revealer), 250);
    gtk_container_add (GTK_CONTAINER (revealer), controls);
    gtk_widget_set_valign (revealer, GTK_ALIGN_END);

    gtk_overlay_add_overlay (GTK_OVERLAY (rootPane), revealer);
    gtk_widget_set_valign (controls, GTK_ALIGN_END);

    gtk_widget_show_all (fullscreenWindow);

    gtk_window_fullscreen (GTK_WINDOW (fullscreenWindow));
    motionSignalId = g_signal_connect (videoWindow, "motion-notify-event",
            G_CALLBACK(motionNotify_cb), NULL);

    gtk_widget_hide (GTK_WIDGET (parentWindow));
}

static void motionNotify_cb (GtkWidget* widget, gpointer data) {
    UNUSED (widget);
    UNUSED (data);

    GdkWindow* window = gtk_widget_get_parent_window(videoWindow);
    GdkDisplay* display = gdk_display_get_default();
    GdkCursor* defaultCursor = gdk_cursor_new_for_display (display, GDK_LEFT_PTR);
    gdk_window_set_cursor(window, defaultCursor);

    gtk_revealer_set_reveal_child (GTK_REVEALER (revealer), TRUE);
    g_signal_handler_block (videoWindow, motionSignalId);
    g_timeout_add_seconds (3, (GSourceFunc) hideControls, NULL);
}

static void overlayFullscreen_cb (GtkWidget* widget, GtkWindow* mainWindow) {
    fullUiWidgets.fullscreenSlider = NULL;

    GtkWindow* fullscreenWindow = GTK_WINDOW (gtk_widget_get_toplevel(widget));
    gtk_window_close (fullscreenWindow);

    gtk_widget_show (GTK_WIDGET (mainWindow));
    createContext (uiWidgets.videoWindow);
    gtk_scale_button_set_value (GTK_SCALE_BUTTON (uiWidgets.volumeButton), backendGetVolume());
}

static void fullscreenRealize_cb (GtkWidget* widget, gpointer data) {
    UNUSED (data);

    createContext (widget);
}

static void closeMenu_cb (GtkWidget* widget) {
    UNUSED (widget);

    backendStop();
}

static void exitMenu_cb (GtkWidget* widget) {
    UNUSED (widget);

    backendStop();
    gtk_main_quit();
}

static void fileMenu_cb (GtkWidget* widget) {
    if (!isPlaying) {
        GtkFileChooserNative* fileChooser;
        GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;
        GtkWindow* window = GTK_WINDOW (gtk_widget_get_toplevel(widget));
        int res;

        fileChooser = gtk_file_chooser_native_new ("Open File", window,
                                                   action, "_Open", "_Cancel");

        res = gtk_native_dialog_run (GTK_NATIVE_DIALOG (fileChooser));
        if (res == GTK_RESPONSE_ACCEPT) {
            isPlaying = TRUE;

            char* fileName;
            GtkFileChooser* chooser = GTK_FILE_CHOOSER (fileChooser);
            fileName = gtk_file_chooser_get_filename (chooser);

            #if defined (GDK_WINDOWING_WIN32)
                stringReplace(fileName, '\\', '/');
            #endif

             char* file = getFileName (fileName);
             gtk_window_set_title (GTK_WINDOW (uiWidgets.window), file);

            const char* path = g_strconcat ("file://", fileName, NULL);

            backendPlay (path);
            createContext (uiWidgets.videoWindow);

            GtkWidget* icon = gtk_image_new_from_icon_name ("media-playback-pause",
                    GTK_ICON_SIZE_BUTTON);
            gtk_button_set_image (GTK_BUTTON (uiWidgets.playButton), icon);

            g_signal_connect (uiWidgets.playButton, "clicked",
                    G_CALLBACK (play_cb), NULL);
            g_signal_connect (uiWidgets.stopButton, "clicked",
                    G_CALLBACK (stop_cb), NULL);
            g_signal_connect (uiWidgets.fullscreenButton, "clicked",
                    G_CALLBACK (fullscreen_cb), NULL);
            g_signal_connect (uiWidgets.volumeButton, "value-changed",
                    G_CALLBACK (volume_cb), NULL);
            uiWidgets.sliderUpdateSignalId =
                    g_signal_connect (uiWidgets.slider, "value-changed",
                                      G_CALLBACK (slider_cb), NULL);
            gtk_scale_button_set_value (GTK_SCALE_BUTTON (uiWidgets.volumeButton), 1.0);

            g_free (file);
            g_free (fileName);
        }
        g_object_unref (fileChooser);
    } else {
        GtkFileChooserNative* fileChooser;
        GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;
        GtkWindow* window = GTK_WINDOW (gtk_widget_get_toplevel(widget));
        int res;

        fileChooser = gtk_file_chooser_native_new ("Open File", window,
                                                   action, "_Open", "_Cancel");

        res = gtk_native_dialog_run (GTK_NATIVE_DIALOG (fileChooser));
        if (res == GTK_RESPONSE_ACCEPT) {
            isPlaying = TRUE;

            char* fileName;
            GtkFileChooser* chooser = GTK_FILE_CHOOSER (fileChooser);
            fileName = gtk_file_chooser_get_filename (chooser);

            char* file = getFileName(fileName);
            gtk_window_set_title (GTK_WINDOW (uiWidgets.window), file);

            const char* path = g_strconcat ("file://", fileName, NULL);

            backendChangeUri (path);
            refreshDurationLabel (uiWidgets.duration);

            GtkWidget* icon = gtk_image_new_from_icon_name ("media-playback-pause",
                    GTK_ICON_SIZE_BUTTON);
            gtk_button_set_image (GTK_BUTTON (uiWidgets.playButton), icon);

            g_free (file);
            g_free (fileName);
        }
        g_object_unref (fileChooser);
    }
}

static void aboutMenu_cb (GtkWidget* widget, gpointer data) {
    UNUSED (widget);
    UNUSED (data);

    createAboutDialog();
}

static void informationMenu_cb (GtkWidget* widget, gpointer data) {
    UNUSED (widget);
    UNUSED (data);

    createInformationWindow();
}

static void colorBalanceMenu_cb (GtkWidget* widget, gpointer data) {
    UNUSED (widget);
    UNUSED (data);

    createColorBalanceWindow();
}

static void contrast_cb (GtkRange* range, gpointer data) {
    UNUSED (data);

    gdouble value = gtk_range_get_value (GTK_RANGE (range));
    backendSetColorBalance ("CONTRAST", value);
}

static void brightness_cb (GtkRange* range, gpointer data) {
    UNUSED (data);

    gdouble value = gtk_range_get_value (GTK_RANGE (range));
    backendSetColorBalance ("BRIGHTNESS", value);
}

static void saturation_cb (GtkRange* range, gpointer data) {
    UNUSED (data);

    gdouble value = gtk_range_get_value (GTK_RANGE (range));
    backendSetColorBalance ("SATURATION", value);
}

static void hue_cb (GtkRange* range, gpointer data) {
    UNUSED (data);

    gdouble value = gtk_range_get_value (GTK_RANGE (range));
    backendSetColorBalance ("HUE", value);
}

/* This function is called when the main window is closed */
static void deleteEvent_cb (GtkWidget* widget, GdkEvent* event, gpointer data) {
    UNUSED (widget);
    UNUSED (event);
    UNUSED (data);

    backendStop();
    gtk_main_quit();
}

void stringReplace (char* str, char rep, char with) {
    int length = (int) strlen(str);

    for (int i = 0; i < length; i++) {
        if (str[i] == rep)
        {
            str[i] = with;
        }
    }
}

char* getFileName(char* str) {
    int length = (int) strlen(str);

    for (int i = length; i > 0; i--)
    {
        if (str[i] == '/')
        {
            int size = length - ++i;
            char* fileName = (char*) malloc(sizeof(char) * size);

            memcpy(fileName, str + i, size);
            fileName[size] = '\0';

            return fileName;
        }
    }
    return NULL;
}