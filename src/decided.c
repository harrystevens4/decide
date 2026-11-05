#include <glib.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <gio/gio.h>

struct application_state {
	GMainLoop *mainloop;
	char *name;
};

void bus_acquired_handler(GDBusConnection *bus, const gchar *name, gpointer user_data);
void name_acquired_handler(GDBusConnection *bus, const gchar *name, gpointer user_data);
void name_lost_handler(GDBusConnection *bus, const gchar *name, gpointer user_data);

int main(int, char **) {
	//create a mainloop
	GMainLoop *mainloop = g_main_loop_new(NULL,true);
	//initialise our application
	struct application_state state = {
		.mainloop = mainloop,
		.name = NULL,
	};
	//connect to the session bus
	g_bus_own_name(
		G_BUS_TYPE_SESSION,
		"org.freedesktop.impl.portal.desktop.decided",
		G_BUS_NAME_OWNER_FLAGS_REPLACE,
		bus_acquired_handler,
		name_acquired_handler,
		name_lost_handler,
		&state,
		NULL
	);
	//run the mainloop
	g_main_loop_run(mainloop);
	//cleanup
	g_main_loop_unref(mainloop);
	free(state.name);
	return EXIT_SUCCESS;
}

void bus_acquired_handler(GDBusConnection *bus, const gchar *, gpointer user_data) {
	struct application_state *state = user_data;
	printf("bus acquired\n");
	//====== publish objects ======
	//setup methods
	static GDBusArgInfo open_file_method_in_args_handle = {
		.ref_count = -1, .name = "handle", .signature = "o",
	};
	static GDBusArgInfo open_file_method_in_args_app_id = {
		.ref_count = -1, .name = "app_id", .signature = "s",
	};
	static GDBusArgInfo open_file_method_in_args_parent_window = {
		.ref_count = -1, .name = "parent_window", .signature = "s",
	};
	static GDBusArgInfo open_file_method_in_args_title = {
		.ref_count = -1, .name = "title", .signature = "s",
	};
	static GDBusArgInfo open_file_method_in_args_options = {
		.ref_count = -1, .name = "options", .signature = "a{sv}",
	};
	static GDBusArgInfo open_file_method_out_args_response = {
		.ref_count = -1, .name = "response", .signature = "u",
	};
	static GDBusArgInfo open_file_method_out_args_results = {
		.ref_count = -1, .name = "results", .signature = "a{sv}",
	};
	static GDBusArgInfo *open_file_method_in_args[] = {
		&open_file_method_in_args_handle,
		&open_file_method_in_args_app_id,
		&open_file_method_in_args_parent_window,
		&open_file_method_in_args_title,
		&open_file_method_in_args_options,
		NULL,
	};
	static GDBusArgInfo *open_file_method_out_args[] = {
		&open_file_method_out_args_response,
		&open_file_method_out_args_results,
		NULL,
	};
	static GDBusMethodInfo open_file_method = {
		.ref_count = -1,
		.name = "OpenFile",
		.in_args = open_file_method_in_args,
		.out_args = open_file_method_out_args,
	};
	static GDBusMethodInfo *interface_methods[] = {
		&open_file_method,
		NULL,
	};
	//load some object info
	static GDBusInterfaceInfo interface = {
		.ref_count = -1,
		.name = "org.freedesktop.impl.portal.FileChooser",
		.methods = interface_methods,
	};
	//register objects
	GError *error = NULL;
	guint id = g_dbus_connection_register_object(
		bus,
		"/org/freedesktop/portal/desktop",
		&interface,
		NULL,
		user_data,
		NULL,
		&error
	);
	if (id == 0) {
		fprintf(stderr,"error registering object /org/freedesktop/portal/desktop: %s",error->message);
		g_error_free(error);
		g_main_loop_quit(state->mainloop);
	}
}

void name_acquired_handler(GDBusConnection *, const gchar *name, gpointer user_data) {
	struct application_state *state = user_data;
	printf("bus name \"%s\" acquired\n",name);
	state->name = malloc(strlen(name)+1);
	strcpy(state->name,name);
}

void name_lost_handler(GDBusConnection *, const gchar *, gpointer user_data) {
	struct application_state *state = user_data;
	printf("bus name lost\n");
	g_main_loop_quit(state->mainloop);
	if (state->name == NULL) printf("couldnt acquire name\n");
	printf("exiting\n");
}
