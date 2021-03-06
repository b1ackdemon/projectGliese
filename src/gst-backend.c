#include <gst/gst.h>
#include <gst/video/videooverlay.h>
#include <gst/video/colorbalance.h>
#include <gtk/gtk.h>
#include <glib/gprintf.h>
#include "gst-backend.h"
#include "ui.h"

typedef struct _CustomData {
    GstState state;
    gint64 duration;
    GstStateChangeReturn ret;
} CustomData;

static GstElement* pipeline;
static CustomData customData;

static void eos_cb (GstBus* bus, GstMessage* msg, CustomData* data);
static void error_cb (GstBus* bus, GstMessage* msg, CustomData* data);
static void stateChanged_cb(GstBus* bus, GstMessage* msg, CustomData* data);
static void padAdded_cb (GstElement* dec, GstPad* pad, gpointer data);

void backendInit (int* argc, char*** argv){
    gst_init (argc, argv);
}

int backendSetWindow (guintptr window) {
    gst_video_overlay_set_window_handle (GST_VIDEO_OVERLAY (pipeline), window);
    return 0;
}

int backendPlay (const gchar* filename) {
    GstBus* bus;

    customData.duration = GST_CLOCK_TIME_NONE;
    pipeline = gst_element_factory_make ("playbin", "playbin");
    if (!pipeline) {
        g_printerr ("Not all elements could be created.\n");
        return -1;
    }

    g_object_set (pipeline, "uri", filename, NULL);
    g_signal_connect (pipeline, "pad-added", G_CALLBACK (padAdded_cb), NULL);

    gst_util_set_object_arg ((GObject *) pipeline, "flags",
            "soft-colorbalance+soft-volume+vis+text+audio+video");

    bus = gst_element_get_bus (pipeline);
    gst_bus_add_signal_watch (bus);
    g_signal_connect (bus, "message::error", (GCallback) error_cb, &customData);
    g_signal_connect (bus, "message::eos", (GCallback) eos_cb, &customData);
    g_signal_connect (bus, "message::state-changed", (GCallback) stateChanged_cb, &customData);
    gst_object_unref (bus);

    customData.ret = gst_element_set_state (pipeline, GST_STATE_PLAYING);
    if (customData.ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr ("Unable to set the pipeline to the playing state.\n");
        gst_object_unref (pipeline);
        return -1;
    }
    return 0;
}

void backendChangeUri (const gchar* filename) {
    backendStop();
    g_object_set (pipeline, "uri", filename, NULL);
    backendResume();
}

gdouble backendQueryDuration() {
    gboolean result;

    result = gst_element_query_duration (pipeline, GST_FORMAT_TIME, &customData.duration);
    if (!result) {
        g_printerr ("Could not query current duration.\n");
        return GST_CLOCK_TIME_NONE;
    }
    return (gdouble) customData.duration / GST_SECOND;
}

gboolean backendQueryPosition (gdouble* current) {
    gboolean res;
    gint64 cur;
    res = gst_element_query_position (pipeline, GST_FORMAT_TIME, &cur);
    *current = (gdouble) cur / GST_SECOND;
    g_print ("Position %" GST_TIME_FORMAT " / %" GST_TIME_FORMAT "\r\n",
             GST_TIME_ARGS (cur), GST_TIME_ARGS (customData.duration));
    return res;
}

void backendGetDuration (gchar* str) {
    gdouble duration = backendQueryDuration();
    duration *= GST_SECOND;

    g_sprintf (str, "%u:%02u:%02u", GST_TIME_ARGS (duration));
}

void backendGetPosition (gchar* str) {
    gdouble position;

    if (!backendQueryPosition (&position)) {
        g_sprintf (str, "-:--:--");
        return;
    }

    g_sprintf (str, "%u:%02u:%02u", GST_TIME_ARGS (position * GST_SECOND));
}

gboolean backendDurationIsValid() {
    return GST_CLOCK_TIME_IS_VALID (customData.duration);
}

gboolean backendIsPausedOrPlaying() {
    if (customData.state < GST_STATE_PAUSED) {
        return FALSE;
    }
    return TRUE;
}

gboolean backendIsPlaying() {
    return customData.state == GST_STATE_PLAYING;
}

void backendStop() {
    if (pipeline) {
        gst_element_set_state (pipeline, GST_STATE_READY);
    }
}

void backendResume() {
    gst_element_set_state (pipeline, GST_STATE_PLAYING);
}

void backendPause() {
    gst_element_set_state (pipeline, GST_STATE_PAUSED);
}

void backendSeek (gdouble value) {
    gst_element_seek_simple (pipeline, GST_FORMAT_TIME,
            GST_SEEK_FLAG_FLUSH, (gint64)(value * GST_SECOND));
}

void backendSetVolume (gdouble volume) {
    g_object_set(pipeline, "volume", volume, NULL);
}

gdouble backendGetVolume() {
    gdouble value = 0;
    g_object_get(pipeline, "volume", &value, NULL);
    return value;
}

gchar** backendGetTitleAudioStreams() {
    GstTagList* tags = NULL;
    gint n_audio;
    g_object_get (pipeline, "n-audio", &n_audio, NULL);
    gchar** audioTitlesArray = (gchar**) g_malloc(sizeof(gchar) * n_audio);

    for (int i = 0; i < n_audio; i++) {
        g_signal_emit_by_name (pipeline, "get-audio-tags", i, &tags);
        gst_tag_list_get_string (tags, GST_TAG_TITLE, &audioTitlesArray[i]);
    }
    return audioTitlesArray;
}

gint backendGetAmountOfAudioStreams() {
    gint n_audio;
    g_object_get (pipeline, "n-audio", &n_audio, NULL);
    return n_audio;
}

void backendGetInformationAboutStreams(GtkTextBuffer *textBuffer) {
    gint i;
    GstTagList* tags;
    gchar *str, *total_str;
    guint rate;
    gint n_video, n_audio, n_text;

    /* Read some properties */
    g_object_get (pipeline, "n-video", &n_video, NULL);
    g_object_get (pipeline, "n-audio", &n_audio, NULL);
    g_object_get (pipeline, "n-text", &n_text, NULL);

    for (i = 0; i < n_video; i++) {
        tags = NULL;
        /* Retrieve the stream's video tags */
        g_signal_emit_by_name (pipeline, "get-video-tags", i, &tags);
        if (tags) {
            total_str = g_strdup_printf ("video stream %d:\n", i);
            gtk_text_buffer_insert_at_cursor (textBuffer, total_str, -1);
            g_free (total_str);
            gst_tag_list_get_string (tags, GST_TAG_VIDEO_CODEC, &str);
            total_str = g_strdup_printf ("  codec: %s\n", str ? str : "unknown");
            gtk_text_buffer_insert_at_cursor (textBuffer, total_str, -1);
            g_free (total_str);
            g_free (str);
            gst_tag_list_free (tags);
        }
    }

    for (i = 0; i < n_audio; i++) {
        tags = NULL;
        /* Retrieve the stream's audio tags */
        g_signal_emit_by_name (pipeline, "get-audio-tags", i, &tags);
        if (tags) {
            total_str = g_strdup_printf ("\naudio stream %d:\n", i);
            gtk_text_buffer_insert_at_cursor (textBuffer, total_str, -1);
            g_free (total_str);
            if (gst_tag_list_get_string (tags, GST_TAG_AUDIO_CODEC, &str)) {
                total_str = g_strdup_printf ("  codec: %s\n", str);
                gtk_text_buffer_insert_at_cursor (textBuffer, total_str, -1);
                g_free (total_str);
                g_free (str);
            }
            if (gst_tag_list_get_string (tags, GST_TAG_LANGUAGE_CODE, &str)) {
                total_str = g_strdup_printf ("  language: %s\n", str);
                gtk_text_buffer_insert_at_cursor (textBuffer, total_str, -1);
                g_free (total_str);
                g_free (str);
            }
            if (gst_tag_list_get_uint (tags, GST_TAG_BITRATE, &rate)) {
                total_str = g_strdup_printf ("  bitrate: %d\n", rate);
                gtk_text_buffer_insert_at_cursor (textBuffer, total_str, -1);
                g_free (total_str);
            }
            gst_tag_list_free (tags);
        }
    }

    for (i = 0; i < n_text; i++) {
        tags = NULL;
        /* Retrieve the stream's subtitle tags */
        g_signal_emit_by_name (pipeline, "get-text-tags", i, &tags);
        if (tags) {
            total_str = g_strdup_printf ("\nsubtitle stream %d:\n", i);
            gtk_text_buffer_insert_at_cursor (textBuffer, total_str, -1);
            g_free (total_str);
            if (gst_tag_list_get_string (tags, GST_TAG_LANGUAGE_CODE, &str)) {
                total_str = g_strdup_printf ("  language: %s\n", str);
                gtk_text_buffer_insert_at_cursor (textBuffer, total_str, -1);
                g_free (total_str);
                g_free (str);
            }
            gst_tag_list_free (tags);
        }
    }
}

void backendGetColorBalance (gchar* channelName, gdouble* value) {
    GstColorBalance* colorBalance = GST_COLOR_BALANCE(pipeline);
    GstColorBalanceChannel* channel = NULL;
    const GList* channels, *l;

    channels = gst_color_balance_list_channels (colorBalance);
    for (l = channels; l != NULL; l = l->next) {
        GstColorBalanceChannel* tmp = (GstColorBalanceChannel*) l->data;

        if (g_strrstr (tmp->label, channelName)) {
            channel = tmp;
            break;
        }
    }

    if (!channel) {
        return;
    }
    *value = gst_color_balance_get_value (colorBalance, channel);
}

void backendSetColorBalance (gchar* channelName, const gdouble value) {
    GstColorBalance* colorBalance = GST_COLOR_BALANCE (pipeline);
    GstColorBalanceChannel* channel = NULL;
    const GList* channels, *l;

    channels = gst_color_balance_list_channels (colorBalance);
    for (l = channels; l != NULL; l = l->next) {
        GstColorBalanceChannel* tmp = (GstColorBalanceChannel*) l->data;

        if (g_strrstr (tmp->label, channelName)) {
            channel = tmp;
            break;
        }
    }

    if(!channel) {
        return;
    }
    gst_color_balance_set_value (colorBalance, channel, (gint) value);
}

void backendDeInit() {
    gst_element_set_state (pipeline, GST_STATE_NULL);
    gst_object_unref (pipeline);
}

static void eos_cb (GstBus* bus, GstMessage* msg, CustomData* data) {
    UNUSED (bus);
    UNUSED (msg);
    UNUSED (data);

    g_print ("End-Of-Stream reached.\n");
    gst_element_set_state (pipeline, GST_STATE_READY);
}

/* This function is called when an error message is posted on the bus */
static void error_cb (GstBus* bus, GstMessage* msg, CustomData* data) {
    UNUSED (bus);
    UNUSED (data);

    GError* err;
    gchar* debug_info;

    /* Print error details on the screen */
    gst_message_parse_error (msg, &err, &debug_info);
    g_printerr ("Error received from element %s: %s\n",
            GST_OBJECT_NAME (msg->src), err->message);
    g_printerr ("Debugging information: %s\n", debug_info ? debug_info : "none");
    g_clear_error (&err);
    g_free (debug_info);

    /* Set the pipeline to READY (which stops playback) */
    gst_element_set_state (pipeline, GST_STATE_READY);
}

/* This function is called when the pipeline changes states. We use it to
 * keep track of the current state. */
static void stateChanged_cb(GstBus* bus, GstMessage* msg, CustomData* data) {
    UNUSED (bus);

    GstState old_state, new_state, pending_state;
    gst_message_parse_state_changed (msg, &old_state, &new_state, &pending_state);
    if (GST_MESSAGE_SRC (msg) == GST_OBJECT (pipeline)) {
        data->state = new_state;
        g_print ("State set to %s\n", gst_element_state_get_name (new_state));
        if (old_state == GST_STATE_READY && new_state == GST_STATE_PAUSED) {
            refreshUi();
        }
    }
}

static void padAdded_cb (GstElement* dec, GstPad* pad, gpointer data) {
    UNUSED (dec);
    UNUSED (data);

    GstCaps* caps;
    GstStructure* structure;
    const gchar* name;
    GstPadTemplate* template;
    GstElementClass* class;

    /* check media type */
    caps = gst_pad_query_caps (pad, NULL);
    structure = gst_caps_get_structure (caps, 0);
    name = gst_structure_get_name (structure);

    class = GST_ELEMENT_GET_CLASS (pipeline);

    if (g_str_has_prefix (name, "audio")) {
        template = gst_element_class_get_pad_template (class, "audio_sink");
    } else if (g_str_has_prefix (name, "video")) {
        template = gst_element_class_get_pad_template (class, "video_sink");
    } else if (g_str_has_prefix (name, "text")) {
        template = gst_element_class_get_pad_template (class, "text_sink");
    } else {
        template = NULL;
    }

    if (template) {
        GstPad* sinkpad;

        sinkpad = gst_element_request_pad (pipeline, template, NULL, NULL);

        if (!gst_pad_is_linked (sinkpad)) {
            gst_pad_link (pad, sinkpad);
        }

        gst_object_unref (sinkpad);
    }
}