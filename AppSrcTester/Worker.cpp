#include "Header.h"


App s_app;

int(*upppelLevelCallback)(int);

 int bReadyToSend;

int volatile bIsPipelinePlayin = FALSE;
int volatile bIsPipelineConfigurationDone = FALSE;

char  PseudoDataBlock[1024];

HANDLE       posixThreadID;





int sendDataToAppsrc(char * data, int size)
{

	App *app = &s_app; // in case we running without context
	GstFlowReturn ret = GST_FLOW_OK;

	if (TRUE == bIsPipelinePlayin)
	{
		GstBuffer *buffer;
		gboolean ok = TRUE;


		buffer = gst_buffer_new_allocate(NULL, size, NULL);
		GstMapInfo map;

		if (data != NULL) {

			if (gst_buffer_map(buffer, &map, GST_MAP_WRITE)) {  // <--- There is a memory leak here !!!

				memcpy((void*)map.data, (const void*)data, (size_t)size);
				gst_buffer_unmap(buffer, &map);
			}

			// Set time and duration to the RTP frame.
			static GstClockTime timestamp = 0;
			GST_BUFFER_PTS(buffer) = timestamp;
			guint64 duration = gst_util_uint64_scale_int(1, GST_SECOND, 30);
			GST_BUFFER_DURATION(buffer) = duration;

			timestamp += GST_BUFFER_DURATION(buffer);

			g_signal_emit_by_name(app->appsrc, "push-buffer", buffer, &ret);

			if (ret != GST_FLOW_OK) {
				/* some error, stop sending data */
				GST_DEBUG("Pushing frame error.");
				ok = FALSE;
			}
		}

		gst_buffer_unref(buffer);

		return ok;
	}

	return TRUE;

}


int stopCurrentStream()
{
	App *app = &s_app;

	bIsPipelinePlayin = FALSE;
	if (!app->loop)
		return -1;

	g_main_loop_quit(app->loop);
	bIsPipelineConfigurationDone = FALSE;

	WaitForSingleObject(posixThreadID, INFINITE);
//	pthread_join(posixThreadID, NULL);

	return 0;

}

/* This signal callback is called when appsrc needs data, we add an idle handler
* to the mainloop to start pushing data into the appsrc */
static void
start_feed(GstElement * pipeline, guint size, App * app)
{
	bReadyToSend = TRUE;
}

/* This callback is called when appsrc has enough data and we can stop sending.
* We remove the idle handler from the mainloop */
static void
stop_feed(GstElement * pipeline, App * app)
{

	bReadyToSend = TRUE;
}


static gboolean bus_message(GstBus * bus, GstMessage * message, App * app)
{
	GST_DEBUG("got message %s",
		gst_message_type_get_name(GST_MESSAGE_TYPE(message)));
	printf("got message %s\n", gst_message_type_get_name(GST_MESSAGE_TYPE(message)));


	switch (GST_MESSAGE_TYPE(message)) {
	case GST_MESSAGE_ERROR: {
		GError *err = NULL;
		gchar *dbg_info = NULL;

		gst_message_parse_error(message, &err, &dbg_info);
		g_printerr("ERROR from element %s: %s\n",
			GST_OBJECT_NAME(message->src), err->message);
		g_printerr("Debugging info: %s\n", (dbg_info) ? dbg_info : "none");
		g_error_free(err);
		g_free(dbg_info);

		// Update upper level of bus changes.
		upppelLevelCallback(GST_MESSAGE_TYPE(message));

		//g_main_loop_quit (app->loop);
		stopCurrentStream();

		break;
	}
	case GST_MESSAGE_EOS:
		//g_main_loop_quit (app->loop);
		upppelLevelCallback(GST_MESSAGE_TYPE(message));
		stopCurrentStream();
		break;

	case GST_MESSAGE_CLOCK_LOST:
	{
		gst_element_set_state(app->pipeline, GST_STATE_PAUSED);
		gst_element_set_state(app->pipeline, GST_STATE_PLAYING);


		break;
	}

	case GST_MESSAGE_STATE_CHANGED: {
		GstState old_state, new_state, pending_state;
		gst_message_parse_state_changed(message, &old_state, &new_state, &pending_state);
		/* Only pay attention to messages coming from the pipeline, not its children */

		if (GST_MESSAGE_SRC(message) == GST_OBJECT(app->pipeline)) {
			// Update upper level of bus changes.

			if (new_state == GST_STATE_PLAYING)
				upppelLevelCallback(1234);

			GST_DEBUG("State changed to %s", gst_element_state_get_name(new_state));
			printf("State changed to %s\n", gst_element_state_get_name(new_state));

		}
		break;
	}


	default:
		break;
	}
	return TRUE;
}




int  mainGstreamerCreator()
{
	guint bus_watch_id;
	App *app = &s_app; // in case we running without context


	GError *error = NULL;
	GstBus *bus;
	char launchString[512];

	

	/* Create our own GLib Main Context and make it the default one */
	app->context = g_main_context_new();
	g_main_context_push_thread_default(app->context);


	/* Set debug for specific level and component */
	//gst_debug_set_default_threshold(GST_LEVEL_DEBUG);
	//gst_debug_set_threshold_for_name( "udpsink" , GST_LEVEL_TRACE);


	app->loop = g_main_loop_new(app->context, TRUE);


	/* setup pipeline */
	g_snprintf(launchString,sizeof(launchString),"appsrc name=appsource ! udpsink host=8.8.8.8 port=8888 ");
	app->pipeline = gst_parse_launch(launchString, &error);





	if (error) {
		gchar *message = g_strdup_printf("Unable to build pipeline: %s", error->message);
		GST_DEBUG("%s", message);
		g_clear_error(&error);
		g_free(message);
		return -1;
	}




	bus = gst_pipeline_get_bus(GST_PIPELINE(app->pipeline));
	VERIFY_ELSE_RETURN_ERROR(bus, "Buss error ");

	/* add watch for messages */
	bus_watch_id = gst_bus_add_watch(bus, (GstBusFunc)bus_message, app);

	/* get the appsrc */
	app->appsrc = gst_bin_get_by_name(GST_BIN(app->pipeline), "appsource");
	VERIFY_ELSE_RETURN_ERROR(app->appsrc, "AppSource not found ");
	VERIFY_ELSE_RETURN_ERROR(GST_IS_APP_SRC(app->appsrc), "AppSource is not functional");
	g_signal_connect(app->appsrc, "need-data", G_CALLBACK(start_feed), app);
	g_signal_connect(app->appsrc, "enough-data", G_CALLBACK(stop_feed), app);

	gst_app_src_set_caps(GST_APP_SRC(app->appsrc), app->caps);

	/* setup appsrc */
	g_object_set(G_OBJECT(app->appsrc),
		"stream-type", 0,
		"format", GST_FORMAT_TIME, NULL);

	/* go to playing and wait in a mainloop. */
	VERIFY_ELSE_RETURN_ERROR(gst_element_set_state(app->pipeline, GST_STATE_PLAYING), "Pipeline couldnot run ");

	// Any way release the waiting calling thread.
	bIsPipelinePlayin = TRUE;
	bIsPipelineConfigurationDone = TRUE;





	/* this mainloop is stopped when we receive an error or EOS */
	g_main_loop_run(app->loop);

	GST_DEBUG("stopping");



	gst_element_set_state(app->pipeline, GST_STATE_NULL);


	//usleep(1000 * 200);

	printf("Stopping main thread!!!!");

	g_source_remove(bus_watch_id);
	gst_object_unref(bus);

	gst_object_unref(app->pipeline);
	g_main_loop_unref(app->loop);
	g_main_context_pop_thread_default(app->context);
	g_main_context_unref(app->context);

	gst_object_unref(app->appsrc);

	// Just make sure we are reset all data 
	memset(app, 0, sizeof(App));


	//return 0;
	ExitThread(NULL);


}


DWORD WINAPI  mainGstreamerCreatorSupervisor(LPVOID lpParam)

{

	App *app = &s_app; // in case we running without context

	//  char *streamingServerUrl, *streamingPublisherName, *streamingPublisherPassword, *streamID;
	// struct arg_struct *args = (struct arg_struct *)arguments;
	//  void *sampleBuffer ;


	//   sampleBuffer = args->sampleBuffer;
	//   streamingServerUrl = args->streamingServerUrl_c ;
	//   streamingPublisherName = args->streamingPublisherName_c ;
	//   streamingPublisherPassword = args->streamingPublisherPassword_c ;
	//   streamID = args->streamID_c ;
	//   


	if (!app->sampleBuffer)
		return NULL;

	int res = mainGstreamerCreator();

	if (0 != res) {

		// Any way release the waiting calling thread.
		bIsPipelineConfigurationDone = TRUE;
		GST_DEBUG(" GStreamer init error, no picture will be sent! ");

	}

	return NULL;
}

gboolean initGStreamerSynchronously(int callback(int))

{

	App *app = &s_app;

	upppelLevelCallback = callback;

	app->sampleBuffer = PseudoDataBlock;

	// Create the thread using POSIX routines.
	//pthread_attr_t  attr;
	int             returnVal;

	//returnVal = pthread_attr_init(&attr);
	//g_assert(!returnVal);

	//returnVal = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	//g_assert(!returnVal);

	// run GStreamer in different thread.
	//int threadError = pthread_create(&posixThreadID, &attr, &mainGstreamerCreatorSupervisor, NULL);
	DWORD   dwThreadIdArray;

	posixThreadID = CreateThread(NULL, 0, &mainGstreamerCreatorSupervisor, NULL, 0, &dwThreadIdArray);

	//returnVal = pthread_attr_destroy(&attr);
	//g_assert(!returnVal);
//	if (threadError != 0)
//	{
//		GST_DEBUG("FATAL: Failed spaning the gstreamer thread");
//		return FALSE;
//	}


	while (bIsPipelineConfigurationDone == FALSE) {

		// Sleep for 10ms.
		Sleep(10);
		GST_DEBUG("Not configured yet, waiting...");

	}

	GST_DEBUG("GST pipeline Configured !!! ");

	return TRUE;
}


