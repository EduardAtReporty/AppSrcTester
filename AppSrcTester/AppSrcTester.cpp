

#include "Header.h"

static char* TAG = "RSL";

extern char  PseudoDataBlock[1024];
int bPlayinFlag = 0;

gboolean initGStreamerSynchronously(int callback(int));
int stopCurrentStream();
int sendDataToAppsrc(char * data, int size);




int mySpecialCallbackFunck(int i) {
	printf("Called from callback function with %d", i);

	if (i == 1234)
		bPlayinFlag = 1;

	return 1;
}

void do_start_and_stop()
{
	int i;

	initGStreamerSynchronously(&mySpecialCallbackFunck);

	sendDataToAppsrc(PseudoDataBlock, sizeof(PseudoDataBlock));

	while (bPlayinFlag == 0) {
		Sleep(100);
	}
	// send data 
	for (i = 0; i < 1000; i++) {
		// send data to thread
		Sleep(10);
		sendDataToAppsrc(PseudoDataBlock, sizeof(PseudoDataBlock));
	}

	bPlayinFlag = 0; 
	stopCurrentStream();
}

int main(int argc, char *argv[])
{
	int i;
	gst_init(&argc, &argv);

	//GST_DEBUG_CATEGORY_INIT(debug_category, TAG, 0, TAG);
	//gst_debug_set_threshold_for_name(TAG, GST_LEVEL_DEBUG);


	for (i = 0; i < 5; i++) {
	
		do_start_and_stop();
	}



	return 0;
}
















