
#define _CRT_SECURE_NO_WARNINGS 

#include <iostream>
#include <gst/gst.h>
#include <gst/app/gstappsrc.h>
#include <string.h>

#include "Windows.h"




struct _App
{
	GMainContext *context; /* GLib context used to run the main loop */

	GstElement *pipeline;
	GstElement *appsrc;

	GMainLoop *loop;
	guint sourceid;

	GstCaps *caps;

	void* sampleBuffer;
	char* streamingServerUrl;
	char* streamingPublisherName;
	char* streamingPublisherPassword;
	char* streamID;
};
typedef struct _App App;



#define VERIFY_ELSE_RETURN_ERROR(condition, message) \
if(!(condition))                                     \
{                                                    \
GST_DEBUG(message);                                  \
return 1;                                            \
}                                                    \

#define VERIFY_ELSE_RETURN_FALSE(condition, message) \
if(!(condition))                                     \
{                                                    \
GST_DEBUG(message);                                  \
return JNI_FALSE;                                    \
}                                                    \

#define VERIFY_ELSE_LOG_ERROR(condition, message)    \
if(!(condition))                                     \
{                                                    \
GST_DEBUG(message);                                  \
}

#define VERIFY_ELSE_CLEANUP(condition, message)      \
if(!(condition))                                     \
{                                                    \
GST_DEBUG(message);                                  \
goto CLEANUP;                                        \
}                                                    \

#define VERIFY_ELSE_THROW(condition, message)                                    \
if(!(condition))                                                                 \
{                                                                                \
}
//@throw([NSException exceptionWithName:nil reason:@(message) userInfo:nil]);    \
}                                                                                \





#define IN
#define OUT
#define NOT_EXISTS -1
#define BASE_STRING_LENGTH 128

typedef gint64 INT64;
typedef enum
{
	FAILURE,
	SUCCESS
}Status;
