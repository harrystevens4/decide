#include <glib.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <gio/gio.h>
#include <alloca.h>
#include "ipc.h"
#include <sys/socket.h>

#define FLAG_MULTIPLE_FILES 1

struct application_state {
	GMainLoop *mainloop;
	GApplication *app;
	char *name;
	int ipc_socket;
};

struct notification {
	char *title;
	char *body;
};

void bus_acquired_handler(GDBusConnection *bus, const gchar *name, gpointer user_data);
void name_acquired_handler(GDBusConnection *bus, const gchar *name, gpointer user_data);
void name_lost_handler(GDBusConnection *bus, const gchar *name, gpointer user_data);
void dbus_method_handler(
		GDBusConnection *bus,
		const gchar *sender,
		const gchar *object_path,
		const gchar *interface_name,
		const gchar *method_name,
		GVariant *parameters,
		GDBusMethodInvocation *invocation,
		gpointer user_data
);
void open_file(GVariant *parameters, GDBusMethodInvocation *invocation, gpointer user_data);
void send_notification(GApplication *app, const struct notification *info);

//for once xml makes my life easier and not harder
const char *xml_schema =
"<node>"
"	<interface name='org.freedesktop.impl.portal.FileChooser'>"
"		<method name='OpenFile'>"
"			<arg type='o' name='handle' direction='in'/>"
"			<arg type='s' name='app_id' direction='in'/>"
"			<arg type='s' name='parent_window' direction='in'/>"
"			<arg type='s' name='title' direction='in'/>"
"			<arg type='a{sv}' name='options' direction='in'/>"
"			<arg type='u' name='response' direction='out'/>"
"			<arg type='a{sv}' name='results' direction='out'/>"
"		</method>"
"	</interface>"
"</node>";

//i justify this global because it is const and has a lifetime of 'static
GDBusNodeInfo *introspection_data = NULL;

int main(int, char **) {
	//create our application instance
	GApplication *application = g_application_new(
		"org.stevens-server.decide",
		G_APPLICATION_DEFAULT_FLAGS
	);
	//initialise our stuff
	int ipc_socket = new_socket_server();
	if (ipc_socket < 0) return EXIT_FAILURE;
	if (listen(ipc_socket,10) < 0) {
		perror("listen");
		close(ipc_socket);
		return EXIT_FAILURE;
	}
	struct application_state state = {
		.name = NULL,
		.ipc_socket = ipc_socket,
		.app = application,
	};
	//preload our introspection data
	introspection_data = g_dbus_node_info_new_for_xml(xml_schema, NULL);
	//create a mainloop
	GMainLoop *mainloop = g_main_loop_new(NULL,true);
	state.mainloop = mainloop;
	//connect to the session bus
	int owner_id = g_bus_own_name(
		G_BUS_TYPE_SESSION,
		"org.freedesktop.impl.portal.desktop.decide",
		G_BUS_NAME_OWNER_FLAGS_REPLACE,
		bus_acquired_handler,
		name_acquired_handler,
		name_lost_handler,
		&state,
		NULL
	);
	//register application
	if (g_application_register(application,NULL,NULL) == false) {
		fprintf(stderr,"failed to register application\n");
	}
	//run the mainloop
	g_main_loop_run(mainloop);
	//cleanup
	g_bus_unown_name(owner_id);
	g_dbus_node_info_unref(introspection_data);
	g_main_loop_unref(mainloop);
	free(state.name);
	close(ipc_socket);
	return EXIT_SUCCESS;
}

void bus_acquired_handler(GDBusConnection *bus, const gchar *, gpointer user_data) {
	struct application_state *state = user_data;
	printf("bus acquired\n");
	//====== publish objects ======
	//create vtable
	static GDBusInterfaceVTable glib_vtable = {
		.method_call = dbus_method_handler,
	};
	//register objects
	GError *error = NULL;
	guint id = g_dbus_connection_register_object(
		bus,
		"/org/freedesktop/portal/desktop",
		introspection_data->interfaces[0],
		&glib_vtable,
		user_data,
		NULL,
		&error
	);
	if (id == 0) {
		fprintf(stderr,"error registering object /org/freedesktop/portal/desktop: %s\n",error->message);
		g_error_free(error);
		g_main_loop_quit(state->mainloop);
	}
	printf("desktop portal object added successfully\n");
}

void name_acquired_handler(GDBusConnection *, const gchar *name, gpointer user_data) {
	struct application_state *state = user_data;
	printf("bus name \"%s\" acquired\n",name);
	state->name = malloc(strlen(name)+1);
	strcpy(state->name,name);
}

void name_lost_handler(GDBusConnection *, const gchar *, gpointer ) {
	//struct application_state *state = user_data;
	printf("bus name lost\n");
	//g_main_loop_quit(state->mainloop);
	//if (state->name == NULL) printf("couldnt acquire name\n");
	//printf("exiting\n");
}

void dbus_method_handler(
		GDBusConnection *,
		const gchar *,
		const gchar *,
		const gchar *,
		const gchar *method_name,
		GVariant *parameters,
		GDBusMethodInvocation *invocation,
		gpointer user_data
) {
	if (strcmp(method_name,"OpenFile") == 0) {
		return open_file(parameters,invocation,user_data);
	}
	printf("ignoring unknown method \"%s\"\n",method_name);
}

GVariant *listen_for_filenames(int socket, int flags) {
	//contact selector process
	int selector_fd = accept(socket,NULL,NULL);
	if (selector_fd < 0) {
		perror("accept");
		return NULL;
	}
	char **file_names;
	size_t file_name_count;
	if (receive_files(selector_fd,&file_names,&file_name_count) < 0) return NULL;
	int response_status = STATUS_OK;
	if (file_name_count < 1) response_status = STATUS_NO_FILES_PROVIDED;
	if (file_name_count > 1 && (flags & FLAG_MULTIPLE_FILES) == 0) {
		response_status = STATUS_TOO_MANY_FILES;
	}
	if (send_response(selector_fd,response_status) < 0) {
		free_files(file_names);
		close(selector_fd);
		return NULL;
	}
	close(selector_fd);
	if (response_status == STATUS_OK) {
		//create the array of strings
		GVariant *filename_strings[file_name_count*sizeof(GVariant *)];
		for (size_t i = 0; i < file_name_count; i++) {
			filename_strings[i] = g_variant_new_printf("file://%s",file_names[i]);
		}
		free_files(file_names);
		return g_variant_new_array(
			NULL,
			filename_strings,
			file_name_count
		);
	} else {
		free_files(file_names);
		return NULL;
	}
}

void open_file(GVariant *parameters, GDBusMethodInvocation *invocation, gpointer user_data) {
	struct application_state *state = user_data;
	printf("open_file called\n");
	char *parameters_str = g_variant_print(parameters,true);
	printf("arguments: %s\n",parameters_str);
	//parse arguments
	gchar *handle;
	gchar *app_id;
	gchar *parent_window;
	gchar *title;
	GVariantIter *options;
	g_variant_get(parameters,"(osssa{sv})",&handle,&app_id,&parent_window,&title,&options);
	//notify the user we are waiting for them to select a file
	send_notification(state->app,&(struct notification){
		.title = "decided file chooser",
		.body = "please select a file to upload",
	});
	//get the files from the socket
	GVariant *filename_array = listen_for_filenames(state->ipc_socket,FLAG_MULTIPLE_FILES);
	if (filename_array != NULL) {
		char *filenames_str = g_variant_print(filename_array,false);
		printf("selected files: %s\n",filenames_str);
		//create the dict of return attributes
		GVariant *uris_dict_entry = g_variant_new_dict_entry(
			g_variant_new_string("uris"),
			g_variant_new("v",filename_array)
		);
		g_auto(GVariantBuilder) return_dict = G_VARIANT_BUILDER_INIT(G_VARIANT_TYPE_ARRAY);
		g_variant_builder_add_value(&return_dict,uris_dict_entry);

		//return
		GVariant *return_value = g_variant_new(
			"(ua{sv})",
			0,
			&return_dict
		);
		g_dbus_method_invocation_return_value(invocation,return_value);
	} else {
		printf("canceled selection\n");
		g_dbus_method_invocation_return_error_literal(
			invocation,
			g_quark_from_static_string("IO error"),
			1,
			"Error occurred while talking to chooser process"
		);
	}
	//tell the user it was successfull
	send_notification(state->app,&(struct notification){
		.title = "decided file chooser",
		.body = "files selected successfully",
	});
	//cleanup
	g_free(handle);
	g_free(app_id);
	g_free(parent_window);
	g_variant_iter_free(options);
	g_free(title);
}

void send_notification(GApplication *app,const struct notification *info) {
	GNotification *notification = g_notification_new(info->title);
	if (info->body != NULL) g_notification_set_body(notification,info->body);
	g_application_send_notification(app,NULL,notification);
	g_object_unref(notification);
}
