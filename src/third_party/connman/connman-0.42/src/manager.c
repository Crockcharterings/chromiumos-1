/*
 *
 *  Connection Manager
 *
 *  Copyright (C) 2007-2009  Intel Corporation. All rights reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <gdbus.h>

#include "connman.h"

#define	_DBG_MANAGER(fmt, arg...)	DBG(DBG_MANAGER, fmt, ## arg)

static void append_profiles(DBusMessageIter *dict)
{
	DBusMessageIter entry, value, iter;
	const char *key = "Profiles";

	dbus_message_iter_open_container(dict, DBUS_TYPE_DICT_ENTRY,
								NULL, &entry);

	dbus_message_iter_append_basic(&entry, DBUS_TYPE_STRING, &key);

	dbus_message_iter_open_container(&entry, DBUS_TYPE_VARIANT,
		DBUS_TYPE_ARRAY_AS_STRING DBUS_TYPE_OBJECT_PATH_AS_STRING,
								&value);

	dbus_message_iter_open_container(&value, DBUS_TYPE_ARRAY,
				DBUS_TYPE_OBJECT_PATH_AS_STRING, &iter);
	__connman_profile_list(&iter);
	dbus_message_iter_close_container(&value, &iter);

	dbus_message_iter_close_container(&entry, &value);

	dbus_message_iter_close_container(dict, &entry);
}

static void append_services(DBusMessageIter *dict)
{
	DBusMessageIter entry, value, iter;
	const char *key = "Services";

	dbus_message_iter_open_container(dict, DBUS_TYPE_DICT_ENTRY,
								NULL, &entry);

	dbus_message_iter_append_basic(&entry, DBUS_TYPE_STRING, &key);

	dbus_message_iter_open_container(&entry, DBUS_TYPE_VARIANT,
		DBUS_TYPE_ARRAY_AS_STRING DBUS_TYPE_OBJECT_PATH_AS_STRING,
								&value);

	dbus_message_iter_open_container(&value, DBUS_TYPE_ARRAY,
				DBUS_TYPE_OBJECT_PATH_AS_STRING, &iter);
	__connman_service_list(&iter);
	dbus_message_iter_close_container(&value, &iter);

	dbus_message_iter_close_container(&entry, &value);

	dbus_message_iter_close_container(dict, &entry);
}

static void append_devices(DBusMessageIter *dict)
{
	DBusMessageIter entry, value, iter;
	const char *key = "Devices";

	dbus_message_iter_open_container(dict, DBUS_TYPE_DICT_ENTRY,
								NULL, &entry);

	dbus_message_iter_append_basic(&entry, DBUS_TYPE_STRING, &key);

	dbus_message_iter_open_container(&entry, DBUS_TYPE_VARIANT,
		DBUS_TYPE_ARRAY_AS_STRING DBUS_TYPE_OBJECT_PATH_AS_STRING,
								&value);

	dbus_message_iter_open_container(&value, DBUS_TYPE_ARRAY,
				DBUS_TYPE_OBJECT_PATH_AS_STRING, &iter);
	__connman_element_list(NULL, CONNMAN_ELEMENT_TYPE_DEVICE, &iter);
	dbus_message_iter_close_container(&value, &iter);

	dbus_message_iter_close_container(&entry, &value);

	dbus_message_iter_close_container(dict, &entry);
}

static void append_connections(DBusMessageIter *dict)
{
	DBusMessageIter entry, value, iter;
	const char *key = "Connections";

	dbus_message_iter_open_container(dict, DBUS_TYPE_DICT_ENTRY,
								NULL, &entry);

	dbus_message_iter_append_basic(&entry, DBUS_TYPE_STRING, &key);

	dbus_message_iter_open_container(&entry, DBUS_TYPE_VARIANT,
		DBUS_TYPE_ARRAY_AS_STRING DBUS_TYPE_OBJECT_PATH_AS_STRING,
								&value);

	dbus_message_iter_open_container(&value, DBUS_TYPE_ARRAY,
				DBUS_TYPE_OBJECT_PATH_AS_STRING, &iter);
	__connman_element_list(NULL, CONNMAN_ELEMENT_TYPE_CONNECTION, &iter);
	dbus_message_iter_close_container(&value, &iter);

	dbus_message_iter_close_container(&entry, &value);

	dbus_message_iter_close_container(dict, &entry);
}

static void append_available_technologies(DBusMessageIter *dict)
{
	DBusMessageIter entry, value, iter;
	const char *key = "AvailableTechnologies";

	dbus_message_iter_open_container(dict, DBUS_TYPE_DICT_ENTRY,
								NULL, &entry);

	dbus_message_iter_append_basic(&entry, DBUS_TYPE_STRING, &key);

	dbus_message_iter_open_container(&entry, DBUS_TYPE_VARIANT,
			DBUS_TYPE_ARRAY_AS_STRING DBUS_TYPE_STRING_AS_STRING,
								&value);

	dbus_message_iter_open_container(&value, DBUS_TYPE_ARRAY,
					DBUS_TYPE_STRING_AS_STRING, &iter);
	__connman_notifier_list_registered(&iter);
	dbus_message_iter_close_container(&value, &iter);

	dbus_message_iter_close_container(&entry, &value);

	dbus_message_iter_close_container(dict, &entry);
}

static void append_enabled_technologies(DBusMessageIter *dict)
{
	DBusMessageIter entry, value, iter;
	const char *key = "EnabledTechnologies";

	dbus_message_iter_open_container(dict, DBUS_TYPE_DICT_ENTRY,
								NULL, &entry);

	dbus_message_iter_append_basic(&entry, DBUS_TYPE_STRING, &key);

	dbus_message_iter_open_container(&entry, DBUS_TYPE_VARIANT,
			DBUS_TYPE_ARRAY_AS_STRING DBUS_TYPE_STRING_AS_STRING,
								&value);

	dbus_message_iter_open_container(&value, DBUS_TYPE_ARRAY,
					DBUS_TYPE_STRING_AS_STRING, &iter);
	__connman_notifier_list_enabled(&iter);
	dbus_message_iter_close_container(&value, &iter);

	dbus_message_iter_close_container(&entry, &value);

	dbus_message_iter_close_container(dict, &entry);
}

static void append_connected_technologies(DBusMessageIter *dict)
{
	DBusMessageIter entry, value, iter;
	const char *key = "ConnectedTechnologies";

	dbus_message_iter_open_container(dict, DBUS_TYPE_DICT_ENTRY,
								NULL, &entry);

	dbus_message_iter_append_basic(&entry, DBUS_TYPE_STRING, &key);

	dbus_message_iter_open_container(&entry, DBUS_TYPE_VARIANT,
			DBUS_TYPE_ARRAY_AS_STRING DBUS_TYPE_STRING_AS_STRING,
								&value);

	dbus_message_iter_open_container(&value, DBUS_TYPE_ARRAY,
					DBUS_TYPE_STRING_AS_STRING, &iter);
	__connman_notifier_list_connected(&iter);
	dbus_message_iter_close_container(&value, &iter);

	dbus_message_iter_close_container(&entry, &value);

	dbus_message_iter_close_container(dict, &entry);
}

static DBusMessage *get_properties(DBusConnection *conn,
					DBusMessage *msg, void *data)
{
	DBusMessage *reply;
	DBusMessageIter array, dict;
	connman_bool_t offlinemode;
	const char *str;

	_DBG_MANAGER("conn %p", conn);

	if (__connman_security_check_privilege(msg,
					CONNMAN_SECURITY_PRIVILEGE_PUBLIC) < 0)
		return __connman_error_permission_denied(msg);

	reply = dbus_message_new_method_return(msg);
	if (reply == NULL)
		return NULL;

	dbus_message_iter_init_append(reply, &array);

	dbus_message_iter_open_container(&array, DBUS_TYPE_ARRAY,
			DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
			DBUS_TYPE_STRING_AS_STRING DBUS_TYPE_VARIANT_AS_STRING
			DBUS_DICT_ENTRY_END_CHAR_AS_STRING, &dict);

	str = __connman_profile_active_path();
	if (str != NULL)
		connman_dbus_dict_append_variant(&dict, "ActiveProfile",
						DBUS_TYPE_OBJECT_PATH, &str);

	append_profiles(&dict);
	append_services(&dict);

	append_devices(&dict);
	append_connections(&dict);

	if (__connman_element_count(NULL, CONNMAN_ELEMENT_TYPE_CONNECTION) > 0)
		str = "online";
	else
		str = "offline";

	connman_dbus_dict_append_variant(&dict, "State",
						DBUS_TYPE_STRING, &str);

	offlinemode = __connman_profile_get_offlinemode();
	connman_dbus_dict_append_variant(&dict, "OfflineMode",
					DBUS_TYPE_BOOLEAN, &offlinemode);

	append_available_technologies(&dict);
	append_enabled_technologies(&dict);
	append_connected_technologies(&dict);

	str = __connman_service_default();
	if (str != NULL)
		connman_dbus_dict_append_variant(&dict, "DefaultTechnology",
						DBUS_TYPE_STRING, &str);

	dbus_message_iter_close_container(&array, &dict);

	return reply;
}

static DBusMessage *set_property(DBusConnection *conn,
					DBusMessage *msg, void *data)
{
	DBusMessageIter iter, value;
	const char *name;
	int type;

	_DBG_MANAGER("conn %p", conn);

	if (dbus_message_iter_init(msg, &iter) == FALSE)
		return __connman_error_invalid_arguments(msg);

	dbus_message_iter_get_basic(&iter, &name);
	dbus_message_iter_next(&iter);
	dbus_message_iter_recurse(&iter, &value);

	if (__connman_security_check_privilege(msg,
					CONNMAN_SECURITY_PRIVILEGE_MODIFY) < 0)
		return __connman_error_permission_denied(msg);

	type = dbus_message_iter_get_arg_type(&value);

	if (g_str_equal(name, "OfflineMode") == TRUE) {
		connman_bool_t offlinemode;

		if (type != DBUS_TYPE_BOOLEAN)
			return __connman_error_invalid_arguments(msg);

		dbus_message_iter_get_basic(&value, &offlinemode);

		__connman_profile_set_offlinemode(offlinemode);

		__connman_profile_save_default();
	} else if (g_str_equal(name, "ActiveProfile") == TRUE) {
		const char *str;

		dbus_message_iter_get_basic(&value, &str);

		return __connman_error_not_supported(msg);
	} else
		return __connman_error_invalid_property(msg);

	return g_dbus_create_reply(msg, DBUS_TYPE_INVALID);
}

static DBusMessage *get_state(DBusConnection *conn,
					DBusMessage *msg, void *data)
{
	const char *str;

	_DBG_MANAGER("conn %p", conn);

	if (__connman_security_check_privilege(msg,
					CONNMAN_SECURITY_PRIVILEGE_PUBLIC) < 0)
		return __connman_error_permission_denied(msg);

	if (__connman_element_count(NULL, CONNMAN_ELEMENT_TYPE_CONNECTION) > 0)
		str = "online";
	else
		str = "offline";

	return g_dbus_create_reply(msg, DBUS_TYPE_STRING, &str,
						DBUS_TYPE_INVALID);
}

static DBusMessage *create_profile(DBusConnection *conn,
					DBusMessage *msg, void *data)
{
	const char *name, *path;
	int err;

	_DBG_MANAGER("conn %p", conn);

	dbus_message_get_args(msg, NULL, DBUS_TYPE_STRING, &name,
							DBUS_TYPE_INVALID);

	if (__connman_security_check_privilege(msg,
					CONNMAN_SECURITY_PRIVILEGE_MODIFY) < 0)
		return __connman_error_permission_denied(msg);

	err = __connman_profile_create(name, &path);
	if (err < 0)
		return __connman_error_failed(msg, -err);

	return g_dbus_create_reply(msg, DBUS_TYPE_OBJECT_PATH, &path,
							DBUS_TYPE_INVALID);
}

static DBusMessage *remove_profile(DBusConnection *conn,
					DBusMessage *msg, void *data)
{
	const char *path;
	int err;

	_DBG_MANAGER("conn %p", conn);

	dbus_message_get_args(msg, NULL, DBUS_TYPE_OBJECT_PATH, &path,
							DBUS_TYPE_INVALID);

	if (__connman_security_check_privilege(msg,
					CONNMAN_SECURITY_PRIVILEGE_MODIFY) < 0)
		return __connman_error_permission_denied(msg);

	err = __connman_profile_remove(path);
	if (err < 0)
		return __connman_error_failed(msg, -err);

	return g_dbus_create_reply(msg, DBUS_TYPE_INVALID);
}

static DBusMessage *request_scan(DBusConnection *conn,
					DBusMessage *msg, void *data)
{
	enum connman_service_type type;
	const char *str;
	int err;

	_DBG_MANAGER("conn %p", conn);

	dbus_message_get_args(msg, NULL, DBUS_TYPE_STRING, &str,
							DBUS_TYPE_INVALID);

	if (g_strcmp0(str, "") == 0)
		type = CONNMAN_SERVICE_TYPE_UNKNOWN;
	else if (g_strcmp0(str, "wifi") == 0)
		type = CONNMAN_SERVICE_TYPE_WIFI;
	else if (g_strcmp0(str, "wimax") == 0)
		type = CONNMAN_SERVICE_TYPE_WIMAX;
	else
		return __connman_error_invalid_arguments(msg);

	err = __connman_element_request_scan(type);
	if (err < 0) {
		if (err == -EINPROGRESS) {
			connman_error("Invalid return code from scan");
			err = -EINVAL;
		}

		return __connman_error_failed(msg, -err);
	}

	return g_dbus_create_reply(msg, DBUS_TYPE_INVALID);
}

static DBusConnection *connection = NULL;

static enum connman_service_type technology_type;
static connman_bool_t technology_enabled;
static DBusMessage *technology_pending = NULL;
static guint technology_timeout = 0;

static void technology_reply(int error)
{
	_DBG_MANAGER("");

	if (technology_timeout > 0) {
		g_source_remove(technology_timeout);
		technology_timeout = 0;
	}

	if (technology_pending != NULL) {
		if (error > 0) {
			DBusMessage *reply;

			reply = __connman_error_failed(technology_pending,
								error);
			if (reply != NULL)
				g_dbus_send_message(connection, reply);
		} else
			g_dbus_send_reply(connection, technology_pending,
							DBUS_TYPE_INVALID);

		dbus_message_unref(technology_pending);
		technology_pending = NULL;
	}

	technology_type = CONNMAN_SERVICE_TYPE_UNKNOWN;
}

static gboolean technology_abort(gpointer user_data)
{
	_DBG_MANAGER("");

	technology_timeout = 0;

	technology_reply(ETIMEDOUT);

	return FALSE;
}

static void technology_notify(enum connman_service_type type,
						connman_bool_t enabled)
{
	_DBG_MANAGER("type %d enabled %d", type, enabled);

	if (type == technology_type && enabled == technology_enabled)
		technology_reply(0);
}

static struct connman_notifier technology_notifier = {
	.name		= "manager",
	.priority	= CONNMAN_NOTIFIER_PRIORITY_HIGH,
	.service_enabled= technology_notify,
};

static DBusMessage *enable_technology(DBusConnection *conn,
					DBusMessage *msg, void *data)
{
	enum connman_service_type type;
	const char *str;
	int err;

	_DBG_MANAGER("conn %p", conn);

	if (technology_pending != NULL)
		return __connman_error_in_progress(msg);

	dbus_message_get_args(msg, NULL, DBUS_TYPE_STRING, &str,
							DBUS_TYPE_INVALID);

	if (g_strcmp0(str, "ethernet") == 0)
		type = CONNMAN_SERVICE_TYPE_ETHERNET;
	else if (g_strcmp0(str, "wifi") == 0)
		type = CONNMAN_SERVICE_TYPE_WIFI;
	else if (g_strcmp0(str, "wimax") == 0)
		type = CONNMAN_SERVICE_TYPE_WIMAX;
	else if (g_strcmp0(str, "bluetooth") == 0)
		type = CONNMAN_SERVICE_TYPE_BLUETOOTH;
	else if (g_strcmp0(str, "cellular") == 0)
		type = CONNMAN_SERVICE_TYPE_CELLULAR;
	else
		return __connman_error_invalid_arguments(msg);

	if (__connman_notifier_is_enabled(type) == TRUE)
		return __connman_error_already_enabled(msg);

	technology_type = type;
	technology_enabled = TRUE;
	technology_pending = dbus_message_ref(msg);

	err = __connman_element_enable_technology(type);
	if (err < 0 && err != -EINPROGRESS)
		technology_reply(-err);
	else
		technology_timeout = g_timeout_add_seconds(15,
						technology_abort, NULL);

	return NULL;
}

static DBusMessage *disable_technology(DBusConnection *conn,
					DBusMessage *msg, void *data)
{
	enum connman_service_type type;
	const char *str;
	int err;

	_DBG_MANAGER("conn %p", conn);

	if (technology_pending != NULL)
		return __connman_error_in_progress(msg);

	dbus_message_get_args(msg, NULL, DBUS_TYPE_STRING, &str,
							DBUS_TYPE_INVALID);

	if (g_strcmp0(str, "ethernet") == 0)
		type = CONNMAN_SERVICE_TYPE_ETHERNET;
	else if (g_strcmp0(str, "wifi") == 0)
		type = CONNMAN_SERVICE_TYPE_WIFI;
	else if (g_strcmp0(str, "wimax") == 0)
		type = CONNMAN_SERVICE_TYPE_WIMAX;
	else if (g_strcmp0(str, "bluetooth") == 0)
		type = CONNMAN_SERVICE_TYPE_BLUETOOTH;
	else if (g_strcmp0(str, "cellular") == 0)
		type = CONNMAN_SERVICE_TYPE_CELLULAR;
	else
		return __connman_error_invalid_arguments(msg);

	if (__connman_notifier_is_enabled(type) == FALSE)
		return __connman_error_already_disabled(msg);

	technology_type = type;
	technology_enabled = FALSE;
	technology_pending = dbus_message_ref(msg);

	err = __connman_element_disable_technology(type);
	if (err < 0 && err != -EINPROGRESS)
		technology_reply(-err);
	else
		technology_timeout = g_timeout_add_seconds(10,
						technology_abort, NULL);

	return NULL;
}

static DBusMessage *connect_service(DBusConnection *conn,
					DBusMessage *msg, void *data)
{
	int err;

	_DBG_MANAGER("conn %p", conn);

	if (__connman_security_check_privilege(msg,
					CONNMAN_SECURITY_PRIVILEGE_MODIFY) < 0)
		return __connman_error_permission_denied(msg);

	err = __connman_service_create_and_connect(msg);
	if (err < 0) {
		if (err == -EINPROGRESS) {
			connman_error("Invalid return code from connect");
			err = -EINVAL;
		}

		return __connman_error_failed(msg, -err);
	}

	return NULL;
}

static DBusMessage *register_agent(DBusConnection *conn,
					DBusMessage *msg, void *data)
{
	const char *sender, *path;
	int err;

	_DBG_MANAGER("conn %p", conn);

	sender = dbus_message_get_sender(msg);

	dbus_message_get_args(msg, NULL, DBUS_TYPE_OBJECT_PATH, &path,
							DBUS_TYPE_INVALID);

	err = __connman_agent_register(sender, path);
	if (err < 0)
		return __connman_error_failed(msg, -err);

	return g_dbus_create_reply(msg, DBUS_TYPE_INVALID);
}

static DBusMessage *unregister_agent(DBusConnection *conn,
					DBusMessage *msg, void *data)
{
	const char *sender, *path;
	int err;

	_DBG_MANAGER("conn %p", conn);

	sender = dbus_message_get_sender(msg);

	dbus_message_get_args(msg, NULL, DBUS_TYPE_OBJECT_PATH, &path,
							DBUS_TYPE_INVALID);

	err = __connman_agent_unregister(sender, path);
	if (err < 0)
		return __connman_error_failed(msg, -err);

	return g_dbus_create_reply(msg, DBUS_TYPE_INVALID);
}

static DBusMessage *get_debugmask(DBusConnection *conn,
					DBusMessage *msg, void *data)
{
	const gchar *str = __connman_debug_getmask_str();

	_DBG_MANAGER("conn %p", conn);

	return g_dbus_create_reply(msg, DBUS_TYPE_STRING, &str,
							DBUS_TYPE_INVALID);
}

static DBusMessage *set_debugmask(DBusConnection *conn,
					DBusMessage *msg, void *data)
{
	const char *str = NULL;
	unsigned long v;

	_DBG_MANAGER("conn %p", conn);

	if (__connman_security_check_privilege(msg,
					CONNMAN_SECURITY_PRIVILEGE_MODIFY) < 0)
		return __connman_error_permission_denied(msg);

	dbus_message_get_args(msg, NULL, DBUS_TYPE_STRING, &str,
							DBUS_TYPE_INVALID);

	v = strtoul(str, NULL, 0);
	connman_info("Set debug mask %s (0x%lx)", str, v);
	(void) __connman_debug_setmask((unsigned)v);

	return g_dbus_create_reply(msg, DBUS_TYPE_INVALID);
}

static GDBusMethodTable manager_methods[] = {
	{ "GetProperties",     "",      "a{sv}", get_properties     },
	{ "SetProperty",       "sv",    "",      set_property       },
	{ "GetState",          "",      "s",     get_state          },
	{ "CreateProfile",     "s",     "o",     create_profile     },
	{ "RemoveProfile",     "o",     "",      remove_profile     },
	{ "RequestScan",       "s",     "",      request_scan       },
	{ "EnableTechnology",  "s",     "",      enable_technology,
						G_DBUS_METHOD_FLAG_ASYNC },
	{ "DisableTechnology", "s",     "",      disable_technology,
						G_DBUS_METHOD_FLAG_ASYNC },
	{ "ConnectService",    "a{sv}", "o",     connect_service,
						G_DBUS_METHOD_FLAG_ASYNC },
	{ "RegisterAgent",     "o",     "",      register_agent     },
	{ "UnregisterAgent",   "o",     "",      unregister_agent   },
	{ "GetDebugMask",      "",      "s",     get_debugmask      },
	{ "SetDebugMask",      "s",     "",      set_debugmask      },
	{ },
};

static GDBusSignalTable manager_signals[] = {
	{ "PropertyChanged", "sv" },
	{ "StateChanged",    "s"  },
	{ },
};

static DBusMessage *nm_sleep(DBusConnection *conn,
					DBusMessage *msg, void *data)
{
	DBusMessage *reply;

	_DBG_MANAGER("conn %p", conn);

	reply = dbus_message_new_method_return(msg);
	if (reply == NULL)
		return NULL;

	dbus_message_append_args(reply, DBUS_TYPE_INVALID);

	return reply;
}

static DBusMessage *nm_wake(DBusConnection *conn,
					DBusMessage *msg, void *data)
{
	DBusMessage *reply;

	_DBG_MANAGER("conn %p", conn);

	reply = dbus_message_new_method_return(msg);
	if (reply == NULL)
		return NULL;

	dbus_message_append_args(reply, DBUS_TYPE_INVALID);

	return reply;
}

enum {
	NM_STATE_UNKNOWN = 0,
	NM_STATE_ASLEEP,
	NM_STATE_CONNECTING,
	NM_STATE_CONNECTED,
	NM_STATE_DISCONNECTED
};

static DBusMessage *nm_state(DBusConnection *conn,
					DBusMessage *msg, void *data)
{
	DBusMessage *reply;
	dbus_uint32_t state;

	_DBG_MANAGER("conn %p", conn);

	reply = dbus_message_new_method_return(msg);
	if (reply == NULL)
		return NULL;

	if (__connman_element_count(NULL, CONNMAN_ELEMENT_TYPE_CONNECTION) > 0)
		state = NM_STATE_CONNECTED;
	else
		state = NM_STATE_DISCONNECTED;

	dbus_message_append_args(reply, DBUS_TYPE_UINT32, &state,
							DBUS_TYPE_INVALID);

	return reply;
}

static GDBusMethodTable nm_methods[] = {
	{ "sleep", "",  "",   nm_sleep        },
	{ "wake",  "",  "",   nm_wake         },
	{ "state", "",  "u",  nm_state        },
	{ },
};

static gboolean nm_compat = FALSE;

int __connman_manager_init(gboolean compat)
{
	_DBG_MANAGER("");

	connection = connman_dbus_get_connection();
	if (connection == NULL)
		return -1;

	if (connman_notifier_register(&technology_notifier) < 0)
		connman_error("Failed to register technology notifier");

	g_dbus_register_interface(connection, CONNMAN_MANAGER_PATH,
					CONNMAN_MANAGER_INTERFACE,
					manager_methods,
					manager_signals, NULL, NULL, NULL);

	if (compat == TRUE) {
		g_dbus_register_interface(connection, NM_PATH, NM_INTERFACE,
					nm_methods, NULL, NULL, NULL, NULL);

		nm_compat = TRUE;
	}

	return 0;
}

void __connman_manager_cleanup(void)
{
	_DBG_MANAGER("");

	connman_notifier_unregister(&technology_notifier);

	if (connection == NULL)
		return;

	if (nm_compat == TRUE) {
		g_dbus_unregister_interface(connection, NM_PATH, NM_INTERFACE);
	}

	g_dbus_unregister_interface(connection, CONNMAN_MANAGER_PATH,
						CONNMAN_MANAGER_INTERFACE);

	dbus_connection_unref(connection);
}
