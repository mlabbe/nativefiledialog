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
	if (S_ISDIR(stat->st_mode))
		return true;

	BString name(ref->name);
	for(int32 i = 0; i < mExtensions.CountStrings(); i++) {
		if (name.EndsWith(mExtensions.StringAt(i)))
			return true;
	}
	
	return false;
}

class DialogHandler : public BLooper {
public:
	DialogHandler(port_id port);
	void MessageReceived(BMessage *msg);
private:
	port_id mPort;
};


DialogHandler::DialogHandler(port_id port)
	: BLooper(),
	mPort(port)
{
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

void
DialogHandler::MessageReceived(BMessage *msg)
{
	switch(msg->what) {
		case B_REFS_RECEIVED: {
			response_data data;
			msg->GetInfo("refs", NULL, &data.open.count);
			data.open.refs = new entry_ref[data.open.count];
			for (int32 i = 0; i < data.open.count; i++) {
				entry_ref ref;
				msg->FindRef("refs", i, data.open.refs + i);
			}
			write_port(mPort, kOpenResponse, &data, sizeof(response_data));
			PostMessage(B_QUIT_REQUESTED);
			break;
		}
		
		case B_SAVE_REQUESTED: {
			response_data data;
			msg->FindRef("directory", &data.save.directory);
			msg->FindString("name", &data.save.filename);
			write_port(mPort, kSaveResponse, &data, sizeof(response_data));
			PostMessage(B_QUIT_REQUESTED);
			break;
		}
		
		case B_CANCEL: {
			response_data data;
			write_port(mPort, kCancelResponse, &data, sizeof(response_data));
			PostMessage(B_QUIT_REQUESTED);
			break;
		}
		
		default:
			BLooper::MessageReceived(msg);
	}
}


/* single file open dialog */    
nfdresult_t NFD_OpenDialog( const nfdchar_t *filterList,
                            const nfdchar_t *defaultPath,
                            nfdchar_t **outPath )
{
	if (be_app == NULL) {
		NFDi_SetError("You need a valid BApplication before you can open a file open dialog!");
		return NFD_ERROR;
	}
	
	port_id port = create_port(1, "NFD_OpenDialog helper");
	if (port == B_NO_MORE_PORTS) {
		NFDi_SetError("Couldn't create port for communicating with BFilePanel");
		return NFD_ERROR;
	}

	DialogHandler *handler = new DialogHandler(port);
	BMessenger messenger(handler);
	BFilePanel *panel = new BFilePanel(B_OPEN_PANEL, NULL, NULL, B_FILE_NODE, false);
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
	
	response_data data;
	int32 response;

	read_port(port, &response, &data, sizeof(response_data));

	delete_port(port);
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
	if (be_app == NULL) {
		NFDi_SetError("You need a valid BApplication before you can open a file open dialog!");
		return NFD_ERROR;
	}
	
	port_id port = create_port(1, "NFD_OpenDialogMultiple helper");
	if (port == B_NO_MORE_PORTS) {
		NFDi_SetError("Couldn't create port for communicating with BFilePanel");
		return NFD_ERROR;
	}

	DialogHandler *handler = new DialogHandler(port);
	BMessenger messenger(handler);
	BFilePanel *panel = new BFilePanel(B_OPEN_PANEL, NULL, NULL, B_FILE_NODE, false);
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
	
	response_data data;
	int32 response;

	read_port(port, &response, &data, sizeof(response_data));

	delete_port(port);
	delete panel;
	handler->PostMessage(B_QUIT_REQUESTED);

	switch (response) {
		case kCancelResponse:
			if (filter)
				delete filter;
			return NFD_CANCEL;
		case kOpenResponse: {
			size_t total_length;
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
{	if (be_app == NULL) {
		NFDi_SetError("You need a valid BApplication before you can open a file open dialog!");
		return NFD_ERROR;
	}
	
	port_id port = create_port(1, "NFD_OpenDialog helper");
	if (port == B_NO_MORE_PORTS) {
		NFDi_SetError("Haiku couldn't create port for communicating with BFilePanel");
		return NFD_ERROR;
	}

	DialogHandler *handler = new DialogHandler(port);
	handler->Run();
	BMessenger messenger(handler);
	BFilePanel *panel = new BFilePanel(B_SAVE_PANEL, NULL, NULL, B_FILE_NODE, false);
	panel->SetTarget(messenger);
	if (defaultPath != NULL) {
		BEntry directory(defaultPath, true);
		panel->SetPanelDirectory(&directory);
	}

	panel->Show();
	response_data data;
	int32 response;

	read_port(port, &response, &data, sizeof(response_data));

	delete_port(port);
	delete panel;
	handler->PostMessage(B_QUIT_REQUESTED);

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
