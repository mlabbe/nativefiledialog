/*
  Native File Dialog

  http://www.frogtoss.com/labs
 */


#include "nfd.h"
#include "nfd_common.h"

#include <stdio.h>

#include <Application.h>
#include <Entry.h>
#include <FilePanel.h>
#include <Handler.h>
#include <Messenger.h>
#include <Path.h>
#include <String.h>
#include <StringList.h>

#include <compat/sys/stat.h>

const int32 kOpenResponse   = 'oREF';
const int32 kSaveResponse   = 'sREF';
const int32 kCancelResponse = 'cREF';

class ExtensionRefFilter : public BRefFilter {
public:
	ExtensionRefFilter(BString stuff);
	bool Filter(const entry_ref* ref, BNode* node,
		struct stat_beos* stat, const char* mimeType);
private:
	BStringList mExtensions;
};

#define FILTER 1

ExtensionRefFilter::ExtensionRefFilter(BString stuff) {
	int32 start = 0;
	while(start < stuff.Length()) {
		int32 comma = stuff.FindFirst(",", start);
		int32 semicolon = stuff.FindFirst(";", start);
		if (comma >= 0 && (comma <= semicolon || semicolon < 0)) {
			mExtensions.Add(BString(stuff.String() + start, comma - start));
			start = comma + 1;
		} else if (semicolon >= 0 && (semicolon < comma || comma < 0)) {
			mExtensions.Add(BString(stuff.String() + start, semicolon - start));
			start = semicolon + 1;
		} else {
			if (stuff.Length() - start < 1)
				break;
			mExtensions.Add(BString(stuff.String() + start));
			break;
		}
	}

	for (int32 i = 0; i < mExtensions.CountStrings(); i++) {
			mExtensions.StringAt(i).Prepend(".");
	}
}


bool
ExtensionRefFilter::Filter(const entry_ref* ref, BNode* node,
	struct stat_beos* stat, const char* mimeType)
{
	#if !FILTER
		return true;
	#endif
	if (S_ISDIR(stat->st_mode))
		return true;

	BString name(ref->name);
	for(int32 i = 0; i < mExtensions.CountStrings(); i++) {
		if (name.EndsWith(mExtensions.StringAt(i)))
			return true;
	}
	
	return false;
}


struct response_data {
	struct {
		int32 count;
		entry_ref *refs;
	} open;
		
	struct {
		entry_ref directory;
		BString filename;
	} save;
};


class DialogHandler : public BLooper {
public:
	DialogHandler();
	~DialogHandler() { delete_sem(mSemaphore); }
	void MessageReceived(BMessage *msg);
	int32 ResponseId() { return mResponseId; }
	response_data &ResponseData() { return mResponseData; }
	void Wait() { acquire_sem(mSemaphore); }
private:
	sem_id mSemaphore;
	int32 mResponseId;
	response_data mResponseData;
};


DialogHandler::DialogHandler()
	: BLooper(),
	mResponseId(0)
{
	mSemaphore = create_sem(0, "NativeFileDialog helper");
}

void
DialogHandler::MessageReceived(BMessage *msg)
{
	if (mResponseId != 0) {
		BLooper::MessageReceived(msg);
		return;
	}
		
	switch(msg->what) {
		case B_REFS_RECEIVED: {
			mResponseId = kOpenResponse;
			msg->GetInfo("refs", NULL, &mResponseData.open.count);
			mResponseData.open.refs = new entry_ref[mResponseData.open.count];
			for (int32 i = 0; i < mResponseData.open.count; i++) {
				entry_ref ref;
				msg->FindRef("refs", i, mResponseData.open.refs + i);
			}
			PostMessage(B_QUIT_REQUESTED);
			break;
		}
		
		case B_SAVE_REQUESTED: {
			mResponseId = kSaveResponse;
			msg->FindRef("directory", &mResponseData.save.directory);
			msg->FindString("name", &mResponseData.save.filename);
			PostMessage(B_QUIT_REQUESTED);
			break;
		}
		
		case B_CANCEL: {
			mResponseId = kCancelResponse;
			PostMessage(B_QUIT_REQUESTED);
			break;
		}
		
		default:
			BLooper::MessageReceived(msg);
	}
	
	release_sem(mSemaphore);
}


/* single file open dialog */    
nfdresult_t NFD_OpenDialog( const nfdchar_t *filterList,
                            const nfdchar_t *defaultPath,
                            nfdchar_t **outPath )
{
	BApplication *temporaryApp = NULL;
	if (be_app == NULL) {
		temporaryApp = new BApplication("application/x-vnd.nfd-dialog");
	}

	DialogHandler *handler = new DialogHandler();
	BMessenger messenger(handler);
	BFilePanel *panel = new BFilePanel(B_OPEN_PANEL, NULL, NULL, B_FILE_NODE, false, NULL, NULL, true);
	ExtensionRefFilter *filter = NULL;

	if (filterList != NULL && *filterList != 0) {
		filter = new ExtensionRefFilter(filterList);
		panel->SetRefFilter(filter);
	}

	handler->Run();
	panel->SetTarget(messenger);

	if (defaultPath != NULL) {
		BEntry directory(defaultPath, true);
		panel->SetPanelDirectory(&directory);
	}

	panel->Show();
	
	handler->Wait();

	response_data &data = handler->ResponseData();
	int32 response = handler->ResponseId();
	handler->PostMessage(B_QUIT_REQUESTED);

	if (temporaryApp)
		delete temporaryApp;

	delete panel;

	switch (response) {
		case kCancelResponse:
			if (filter)
				delete filter;
			return NFD_CANCEL;
		case kOpenResponse: {
			if (data.open.count != 1) {
				delete[] data.open.refs;
				if (filter)
					delete filter;

				NFDi_SetError("Got invalid count of refs back");
				return NFD_ERROR;
			}
			
			size_t length = NFDi_UTF8_Strlen((nfdchar_t *)data.open.refs[0].name) + 1;

			*outPath = (nfdchar_t *)NFDi_Malloc(length);
			NFDi_SafeStrncpy(*outPath, data.open.refs[0].name, length);

			if (filter)
				delete filter;
			delete[] data.open.refs;
			return NFD_OKAY;
		}
	}
	
	if (filter)
		delete filter;

	NFDi_SetError("Got invalid response from port");
    return NFD_ERROR;
}


/* multiple file open dialog */    
nfdresult_t NFD_OpenDialogMultiple( const nfdchar_t *filterList,
                                    const nfdchar_t *defaultPath,
                                    nfdpathset_t *outPaths )
{
	BApplication *temporaryApp = NULL;
	if (be_app == NULL) {
		temporaryApp = new BApplication("application/x-vnd.nfd-dialog");
	}

	DialogHandler *handler = new DialogHandler();
	BMessenger messenger(handler);
	BFilePanel *panel = new BFilePanel(B_OPEN_PANEL, NULL, NULL, B_FILE_NODE, false, NULL, NULL, true);
	ExtensionRefFilter *filter = NULL;

	if (filterList != NULL && *filterList != 0) {
		filter = new ExtensionRefFilter(filterList);
		panel->SetRefFilter(filter);
	}

	handler->Run();
	panel->SetTarget(messenger);

	if (defaultPath != NULL) {
		BEntry directory(defaultPath, true);
		panel->SetPanelDirectory(&directory);
	}

	panel->Show();
	
	handler->Wait();

	response_data &data = handler->ResponseData();
	int32 response = handler->ResponseId();

	handler->PostMessage(B_QUIT_REQUESTED);

	if (temporaryApp)
		delete temporaryApp;

	switch (response) {
		case kCancelResponse:
			if (filter)
				delete filter;
			return NFD_CANCEL;
		case kOpenResponse: {
			size_t total_length = 0;
			for (int i = 0; i < data.open.count; i++) {
				total_length += NFDi_UTF8_Strlen((nfdchar_t *)data.open.refs[i].name) + 1;
			}
			
			outPaths->indices = (size_t *)NFDi_Malloc(sizeof(size_t) * data.open.count);
			outPaths->count = data.open.count;
			outPaths->buf = (nfdchar_t *)NFDi_Malloc(total_length);

			nfdchar_t *buflocation = outPaths->buf;
			for (int i = 0; i < data.open.count; i++) {
				size_t length = NFDi_UTF8_Strlen((nfdchar_t *)data.open.refs[i].name) + 1;
				NFDi_SafeStrncpy(buflocation, (nfdchar_t *)data.open.refs[i].name, length);
				outPaths->indices[i] = buflocation - outPaths->buf;
				buflocation += length;
			}

			delete[] data.open.refs;
			if (filter)
				delete filter;
			return NFD_OKAY;
		}
	}

	if (filter)
		delete filter;

	NFDi_SetError("Got invalid response from port");
    return NFD_ERROR;
}


/* save dialog */
nfdresult_t NFD_SaveDialog( const nfdchar_t *filterList,
                            const nfdchar_t *defaultPath,
                            nfdchar_t **outPath )
{
	BApplication *temporaryApp = NULL;
	if (be_app == NULL) {
		temporaryApp = new BApplication("application/x-vnd.nfd-dialog");
	}

	DialogHandler *handler = new DialogHandler();
	handler->Run();
	BMessenger messenger(handler);
	BFilePanel *panel = new BFilePanel(B_SAVE_PANEL, NULL, NULL, B_FILE_NODE, false, NULL, NULL, true);
	panel->SetTarget(messenger);
	if (defaultPath != NULL) {
		BEntry directory(defaultPath, true);
		panel->SetPanelDirectory(&directory);
	}

	panel->Show();
	
	handler->Wait();

	response_data &data = handler->ResponseData();
	int32 response = handler->ResponseId();
	handler->PostMessage(B_QUIT_REQUESTED);

	if (temporaryApp)
		delete temporaryApp;

	switch (response) {
		case kCancelResponse:
			return NFD_CANCEL;
		case kSaveResponse: {
			BEntry entry(&data.save.directory);
			BPath path(&entry);
			size_t dirlength = strlen(path.Path());
			size_t namelength = strlen(data.save.filename);
			*outPath = (nfdchar_t *)NFDi_Malloc(dirlength + namelength + 2);
			NFDi_SafeStrncpy(*outPath, path.Path(), dirlength + 1);
			(*outPath)[dirlength] = '/';
			NFDi_SafeStrncpy((*outPath) + dirlength + 1, data.save.filename, namelength + 1); 
			return NFD_OKAY;
		}
	}

	NFDi_SetError("Got invalid response from port");
    return NFD_ERROR;
}
