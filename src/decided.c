#include <glib.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <gio/gio.h>

struct application_state {
	GMainLoop *mainloop;
	char *name;
};

struct vtable_handler_data {
	gpointer user_data;
	struct {
		char method_name[1024];
		void (*method)(GVariant *, GDBusMethodInvocation *, gpointer);
	} entries[];
};

void bus_acquired_handler(GDBusConnection *bus, const gchar *name, gpointer user_data);
void name_acquired_handler(GDBusConnection *bus, const gchar *name, gpointer user_data);
void name_lost_handler(GDBusConnection *bus, const gchar *name, gpointer user_data);
void dbus_method_vtable_handler(
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

//for once xml makes my life easier and not harder
const char *xml_schema =
"<node>"
"	<interface name=\"org.freedesktop.impl.portal.FileChooser\">"
"		<method name=\"OpenFile\">"
"			<arg type=\"o\" name=\"handle\" direction=\"in\"/>"
"			<arg type=\"s\" name=\"app_id\" direction=\"in\"/>"
"			<arg type=\"s\" name=\"parent_window\" direction=\"in\"/>"
"			<arg type=\"s\" name=\"title\" direction=\"in\"/>"
"			<arg type=\"a\{sv}\" name=\"options\" direction=\"in\"/>"
"			<arg type=\"u\" name=\"response\" direction=\"out\"/>"
"			<arg type=\"a{sv}\" name=\"results\" direction=\"out\"/>"
"		</method>"
"	</interface>"
"</node>";

//i justify this global because it is const and has a lifetime of 'static
GDBusNodeInfo *introspection_data = NULL;

int main(int, char **) {
	//preload our introspection data
	introspection_data = g_dbus_node_info_new_for_xml(xml_schema, NULL);
	//create a mainloop
	GMainLoop *mainloop = g_main_loop_new(NULL,true);
	//initialise our application
	struct application_state state = {
		.mainloop = mainloop,
		.name = NULL,
	};
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
	//run the mainloop
	g_main_loop_run(mainloop);
	//cleanup
	g_bus_unown_name(owner_id);
	g_dbus_node_info_unref(introspection_data);
	g_main_loop_unref(mainloop);
	free(state.name);
	return EXIT_SUCCESS;
}

void bus_acquired_handler(GDBusConnection *bus, const gchar *, gpointer user_data) {
	struct application_state *state = user_data;
	printf("bus acquired\n");
	//====== publish objects ======
	//create vtable
	static struct vtable_handler_data vtable_info = {
		.user_data = NULL,
		.entries = {
			{ .method_name = "OpenFile", .method = open_file },
			{ .method_name = "" },
		}
	};
	vtable_info.user_data = user_data;
	//register objects
	GError *error = NULL;
	guint id = g_dbus_connection_register_object(
		bus,
		"/org/freedesktop/portal/desktop",
		introspection_data->interfaces[0],
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
	//g_main_loop_quit(state->mainloop);
	//if (state->name == NULL) printf("couldnt acquire name\n");
	//printf("exiting\n");
}

void dbus_method_vtable_handler(
		GDBusConnection *bus,
		const gchar *sender,
		const gchar *object_path,
		const gchar *interface_name,
		const gchar *method_name,
		GVariant *parameters,
		GDBusMethodInvocation *invocation,
		gpointer vtable_ptr
) {
	printf("vtable handler called for method: %s\n",method_name);
	struct vtable_handler_data *vtable_data = vtable_ptr;
	gpointer user_data = vtable_data->user_data;
	for (int i = 0;; i++) {
		if (*vtable_data->entries[i].method_name == '\0') break;
		if (strcmp(vtable_data->entries[i].method_name,method_name) == 0) {
			return vtable_data->entries[i].method(parameters,invocation,user_data);
		}
	}
	printf("ignoring method \"%s\": not in vtable\n",method_name);
}

void open_file(GVariant *parameters, GDBusMethodInvocation *invocation, gpointer user_data) {
	printf("open_file called\n");
}
