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

#include <string.h>

#include <glib.h>
#include <gdbus.h>

#include "connman.h"

#define PROFILE_DEFAULT_IDENT  "default"

#define	_DBG_PROFILE(fmt, arg...)	DBG(DBG_PROFILE, fmt, ## arg)

struct connman_profile {
	char *ident;
	char *path;
	char *name;
	connman_bool_t offlinemode;
};

static GHashTable *profiles = NULL;
static struct connman_profile *default_profile = NULL;

static DBusConnection *connection = NULL;

static void append_path(gpointer key, gpointer value, gpointer user_data)
{
	struct connman_profile *profile = value;
	DBusMessageIter *iter = user_data;

	dbus_message_iter_append_basic(iter, DBUS_TYPE_OBJECT_PATH,
							&profile->path);
}

void __connman_profile_list(DBusMessageIter *iter)
{
	_DBG_PROFILE("");

	g_hash_table_foreach(profiles, append_path, iter);
}

static void append_profiles(DBusMessageIter *entry)
{
	DBusMessageIter value, iter;
	const char *key = "Profiles";

	dbus_message_iter_append_basic(entry, DBUS_TYPE_STRING, &key);

	dbus_message_iter_open_container(entry, DBUS_TYPE_VARIANT,
		DBUS_TYPE_ARRAY_AS_STRING DBUS_TYPE_OBJECT_PATH_AS_STRING,
								&value);

	dbus_message_iter_open_container(&value, DBUS_TYPE_ARRAY,
				DBUS_TYPE_OBJECT_PATH_AS_STRING, &iter);
	__connman_profile_list(&iter);
	dbus_message_iter_close_container(&value, &iter);

	dbus_message_iter_close_container(entry, &value);
}

static void profiles_changed(void)
{
	DBusMessage *signal;
	DBusMessageIter entry;

	_DBG_PROFILE("");

	signal = dbus_message_new_signal(CONNMAN_MANAGER_PATH,
				CONNMAN_MANAGER_INTERFACE, "PropertyChanged");
	if (signal == NULL)
		return;

	dbus_message_iter_init_append(signal, &entry);
	append_profiles(&entry);
	g_dbus_send_message(connection, signal);
}

static void name_changed(struct connman_profile *profile)
{
	DBusMessage *signal;
	DBusMessageIter entry, value;
	const char *key = "Name";

	_DBG_PROFILE("profile %p", profile);

	signal = dbus_message_new_signal(profile->path,
				CONNMAN_PROFILE_INTERFACE, "PropertyChanged");
	if (signal == NULL)
		return;

	dbus_message_iter_init_append(signal, &entry);

	dbus_message_iter_append_basic(&entry, DBUS_TYPE_STRING, &key);

	dbus_message_iter_open_container(&entry, DBUS_TYPE_VARIANT,
					DBUS_TYPE_STRING_AS_STRING, &value);
	dbus_message_iter_append_basic(&value, DBUS_TYPE_STRING,
							&profile->name);
	dbus_message_iter_close_container(&entry, &value);

	g_dbus_send_message(connection, signal);
}

static void offlinemode_changed(struct connman_profile *profile)
{
	DBusMessage *signal;
	DBusMessageIter entry, value;
	const char *key = "OfflineMode";

	_DBG_PROFILE("profile %p", profile);

	signal = dbus_message_new_signal(profile->path,
				CONNMAN_PROFILE_INTERFACE, "PropertyChanged");
	if (signal == NULL)
		return;

	dbus_message_iter_init_append(signal, &entry);

	dbus_message_iter_append_basic(&entry, DBUS_TYPE_STRING, &key);

	dbus_message_iter_open_container(&entry, DBUS_TYPE_VARIANT,
					DBUS_TYPE_BOOLEAN_AS_STRING, &value);
	dbus_message_iter_append_basic(&value, DBUS_TYPE_BOOLEAN,
						&profile->offlinemode);
	dbus_message_iter_close_container(&entry, &value);

	g_dbus_send_message(connection, signal);
}

connman_bool_t __connman_profile_get_offlinemode(void)
{
	if (default_profile == NULL)
		return FALSE;

	_DBG_PROFILE("offlinemode %d", default_profile->offlinemode);

	return default_profile->offlinemode;
}

int __connman_profile_set_offlinemode(connman_bool_t offlinemode)
{
	_DBG_PROFILE("offlinemode %d", offlinemode);

	if (default_profile == NULL)
		return -EINVAL;

	if (default_profile->offlinemode == offlinemode)
		return -EALREADY;

	default_profile->offlinemode = offlinemode;
	offlinemode_changed(default_profile);

	__connman_device_set_offlinemode(offlinemode);

	return 0;
}

int __connman_profile_save_default(void)
{
	_DBG_PROFILE("");

	if (default_profile != NULL)
		__connman_storage_save_profile(default_profile);

	return 0;
}

const char *__connman_profile_active_ident(void)
{
	_DBG_PROFILE("");

	return PROFILE_DEFAULT_IDENT;
}

const char *__connman_profile_active_path(void)
{
	_DBG_PROFILE("");

	if (default_profile == NULL)
		return NULL;

	return default_profile->path;
}

static void append_services(struct connman_profile *profile,
						DBusMessageIter *entry)
{
	DBusMessageIter value, iter;
	const char *key = "Services";

	dbus_message_iter_append_basic(entry, DBUS_TYPE_STRING, &key);

	dbus_message_iter_open_container(entry, DBUS_TYPE_VARIANT,
		DBUS_TYPE_ARRAY_AS_STRING DBUS_TYPE_OBJECT_PATH_AS_STRING,
								&value);

	dbus_message_iter_open_container(&value, DBUS_TYPE_ARRAY,
				DBUS_TYPE_OBJECT_PATH_AS_STRING, &iter);

	if (g_strcmp0(profile->ident, PROFILE_DEFAULT_IDENT) == 0)
		__connman_service_list(&iter);

	dbus_message_iter_close_container(&value, &iter);

	dbus_message_iter_close_container(entry, &value);
}

static guint changed_timeout = 0;

static gboolean services_changed(gpointer user_data)
{
	struct connman_profile *profile = default_profile;
	DBusMessage *signal;
	DBusMessageIter entry;

	changed_timeout = 0;

	if (profile == NULL)
		return FALSE;

	signal = dbus_message_new_signal(profile->path,
				CONNMAN_PROFILE_INTERFACE, "PropertyChanged");
	if (signal == NULL)
		return FALSE;

	dbus_message_iter_init_append(signal, &entry);
	append_services(profile, &entry);
	g_dbus_send_message(connection, signal);

	if (g_strcmp0(profile->ident, PROFILE_DEFAULT_IDENT) != 0)
		return FALSE;

	signal = dbus_message_new_signal(CONNMAN_MANAGER_PATH,
				CONNMAN_MANAGER_INTERFACE, "PropertyChanged");
	if (signal == NULL)
		return FALSE;

	dbus_message_iter_init_append(signal, &entry);
	append_services(profile, &entry);
	g_dbus_send_message(connection, signal);

	return FALSE;
}

void __connman_profile_changed(gboolean delayed)
{
	_DBG_PROFILE("");

	if (changed_timeout > 0) {
		g_source_remove(changed_timeout);
		changed_timeout = 0;
	}

	if (__connman_connection_update_gateway() == TRUE) {
		services_changed(NULL);
		return;
	}

	if (delayed == FALSE) {
		services_changed(NULL);
		return;
	}

	changed_timeout = g_timeout_add_seconds(1, services_changed, NULL);
}

int __connman_profile_add_device(struct connman_device *device)
{
	struct connman_service *service;

	_DBG_PROFILE("device %p", device);

	service = __connman_service_create_from_device(device);
	if (service == NULL)
		return -EINVAL;

	return 0;
}

int __connman_profile_remove_device(struct connman_device *device)
{
	_DBG_PROFILE("device %p", device);

	__connman_service_remove_from_device(device);

	return 0;
}

int __connman_profile_add_network(struct connman_network *network)
{
	struct connman_service *service;

	_DBG_PROFILE("network %p", network);

	service = __connman_service_create_from_network(network);
	if (service == NULL)
		return -EINVAL;

	return 0;
}

int __connman_profile_update_network(struct connman_network *network)
{
	_DBG_PROFILE("network %p", network);

	__connman_service_update_from_network(network);

	return 0;
}

int __connman_profile_remove_network(struct connman_network *network)
{
	_DBG_PROFILE("network %p", network);

	__connman_service_remove_from_network(network);

	return 0;
}

static DBusMessage *get_properties(DBusConnection *conn,
					DBusMessage *msg, void *data)
{
	struct connman_profile *profile = data;
	DBusMessage *reply;
	DBusMessageIter array, dict, entry;

	_DBG_PROFILE("conn %p", conn);

	reply = dbus_message_new_method_return(msg);
	if (reply == NULL)
		return NULL;

	dbus_message_iter_init_append(reply, &array);

	dbus_message_iter_open_container(&array, DBUS_TYPE_ARRAY,
			DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
			DBUS_TYPE_STRING_AS_STRING DBUS_TYPE_VARIANT_AS_STRING
			DBUS_DICT_ENTRY_END_CHAR_AS_STRING, &dict);

	if (profile->name != NULL)
		connman_dbus_dict_append_variant(&dict, "Name",
					DBUS_TYPE_STRING, &profile->name);

	connman_dbus_dict_append_variant(&dict, "OfflineMode",
				DBUS_TYPE_BOOLEAN, &profile->offlinemode);

	dbus_message_iter_open_container(&dict, DBUS_TYPE_DICT_ENTRY,
								NULL, &entry);
	append_services(profile, &entry);
	dbus_message_iter_close_container(&dict, &entry);

	dbus_message_iter_close_container(&array, &dict);

	return reply;
}

static DBusMessage *set_property(DBusConnection *conn,
					DBusMessage *msg, void *data)
{
	struct connman_profile *profile = data;
	DBusMessageIter iter, value;
	const char *name;
	int type;

	_DBG_PROFILE("conn %p", conn);

	if (dbus_message_iter_init(msg, &iter) == FALSE)
		return __connman_error_invalid_arguments(msg);

	dbus_message_iter_get_basic(&iter, &name);
	dbus_message_iter_next(&iter);
	dbus_message_iter_recurse(&iter, &value);

	if (__connman_security_check_privilege(msg,
					CONNMAN_SECURITY_PRIVILEGE_MODIFY) < 0)
		return __connman_error_permission_denied(msg);

	type = dbus_message_iter_get_arg_type(&value);

	if (g_str_equal(name, "Name") == TRUE) {
		const char *name;

		if (type != DBUS_TYPE_STRING)
			return __connman_error_invalid_arguments(msg);

		dbus_message_iter_get_basic(&value, &name);

		g_free(profile->name);
		profile->name = g_strdup(name);

		if (profile->name != NULL)
			name_changed(profile);

		__connman_storage_save_profile(profile);
	} else if (g_str_equal(name, "OfflineMode") == TRUE) {
		connman_bool_t offlinemode;

		if (type != DBUS_TYPE_BOOLEAN)
			return __connman_error_invalid_arguments(msg);

		dbus_message_iter_get_basic(&value, &offlinemode);

		if (profile->offlinemode == offlinemode)
			return g_dbus_create_reply(msg, DBUS_TYPE_INVALID);

		profile->offlinemode = offlinemode;
		offlinemode_changed(profile);

		__connman_storage_save_profile(profile);
	} else
		return __connman_error_invalid_property(msg);

	return g_dbus_create_reply(msg, DBUS_TYPE_INVALID);
}

static GDBusMethodTable profile_methods[] = {
	{ "GetProperties", "",   "a{sv}", get_properties },
	{ "SetProperty",   "sv", "",      set_property   },
	{ },
};

static GDBusSignalTable profile_signals[] = {
	{ "PropertyChanged", "sv" },
	{ },
};

static void free_profile(struct connman_profile *profile)
{
	g_free(profile->name);
	g_free(profile->path);
	g_free(profile);
}

static void unregister_profile(gpointer data)
{
	struct connman_profile *profile = data;

	_DBG_PROFILE("profile %p", profile);

	connman_info("Removing profile %s", profile->ident);

	g_dbus_unregister_interface(connection, profile->path,
						CONNMAN_PROFILE_INTERFACE);

	if (g_strcmp0(profile->ident, PROFILE_DEFAULT_IDENT) == 0)
		default_profile = NULL;

	free_profile(profile);
}

static int create_profile(const char *ident, const char *name,
							const char **path)
{
	struct connman_profile *profile;

	_DBG_PROFILE("ident %s name %s", ident, name);

	profile = g_try_new0(struct connman_profile, 1);
	if (profile == NULL)
		return -ENOMEM;

	profile->ident = g_strdup(ident);
	profile->path = g_strdup_printf("/profile/%s", ident);

	if (profile->ident == NULL || profile->path == NULL) {
		free_profile(profile);
		return -ENOMEM;
	}

	if (g_hash_table_lookup(profiles, profile->path) != NULL) {
		free_profile(profile);
		return -EEXIST;
	}

	profile->name = g_strdup(name);

	__connman_storage_load_profile(profile);

	g_hash_table_insert(profiles, g_strdup(profile->path), profile);

	connman_info("Adding profile %s", ident);

	if (g_strcmp0(ident, PROFILE_DEFAULT_IDENT) == 0)
		default_profile = profile;

	g_dbus_register_interface(connection, profile->path,
					CONNMAN_PROFILE_INTERFACE,
					profile_methods, profile_signals,
							NULL, profile, NULL);

	if (path != NULL)
		*path = profile->path;

	_DBG_PROFILE("profile %p path %s", profile, profile->path);

	return 0;
}

static gboolean validate_ident(const char *ident)
{
	unsigned int i;

	for (i = 0; i < strlen(ident); i++) {
		if (ident[i] >= '0' && ident[i] <= '9')
			continue;
		if (ident[i] >= 'a' && ident[i] <= 'z')
			continue;
		if (ident[i] >= 'A' && ident[i] <= 'Z')
			continue;
		return FALSE;
	}

	return TRUE;
}

int __connman_profile_create(const char *name, const char **path)
{
	struct connman_profile *profile;
	int err;

	_DBG_PROFILE("name %s", name);

	if (validate_ident(name) == FALSE)
		return -EINVAL;

	err = create_profile(name, NULL, path);
	if (err < 0)
		return err;

	profile = g_hash_table_lookup(profiles, *path);
	if (profile == NULL)
		return -EIO;

	__connman_storage_save_profile(profile);

	profiles_changed();

	return 0;
}

int __connman_profile_remove(const char *path)
{
	struct connman_profile *profile;

	_DBG_PROFILE("path %s", path);

	if (default_profile != NULL &&
				g_strcmp0(path, default_profile->path) == 0)
		return -EINVAL;

	profile = g_hash_table_lookup(profiles, path);
	if (profile == NULL)
		return -ENXIO;

	__connman_storage_delete(profile->ident);

	g_hash_table_remove(profiles, path);

	profiles_changed();

	return 0;
}

static int profile_init(void)
{
	GDir *dir;
	const gchar *file;

	_DBG_PROFILE("");

	dir = g_dir_open(STORAGEDIR, 0, NULL);
	if (dir != NULL) {
		while ((file = g_dir_read_name(dir)) != NULL) {
			GString *str;
			gchar *ident;

			if (g_str_has_suffix(file, ".profile") == FALSE)
				continue;

			ident = g_strrstr(file, ".profile");
			if (ident == NULL)
				continue;

			str = g_string_new_len(file, ident - file);
			if (str == NULL)
				continue;

			ident = g_string_free(str, FALSE);

			if (validate_ident(ident) == TRUE)
				create_profile(ident, NULL, NULL);

			g_free(ident);
		}

		g_dir_close(dir);
	}

	if (g_hash_table_size(profiles) == 0)
		create_profile(PROFILE_DEFAULT_IDENT, "Default", NULL);

	profiles_changed();

	return 0;
}

static int profile_load(struct connman_profile *profile)
{
	GKeyFile *keyfile;
	GError *error = NULL;
	connman_bool_t offlinemode;
	char *name;

	_DBG_PROFILE("profile %p", profile);

	keyfile = __connman_storage_open(profile->ident);
	if (keyfile == NULL)
		return -EIO;

	name = g_key_file_get_string(keyfile, "global", "Name", NULL);
	if (name != NULL) {
		g_free(profile->name);
		profile->name = name;
	}

	offlinemode = g_key_file_get_boolean(keyfile, "global",
						"OfflineMode", &error);
	if (error == NULL)
		profile->offlinemode = offlinemode;
	g_clear_error(&error);

	__connman_storage_close(profile->ident, keyfile, FALSE);

	return 0;
}

static int profile_save(struct connman_profile *profile)
{
	GKeyFile *keyfile;

	_DBG_PROFILE("profile %p", profile);

	keyfile = __connman_storage_open(profile->ident);
	if (keyfile == NULL)
		return -EIO;

	if (profile->name != NULL)
		g_key_file_set_string(keyfile, "global",
						"Name", profile->name);

	g_key_file_set_boolean(keyfile, "global",
					"OfflineMode", profile->offlinemode);

	__connman_storage_close(profile->ident, keyfile, TRUE);

	return 0;
}

static struct connman_storage profile_storage = {
	.name		= "profile",
	.priority	= CONNMAN_STORAGE_PRIORITY_LOW,
	.profile_init	= profile_init,
	.profile_load	= profile_load,
	.profile_save	= profile_save,
};

int __connman_profile_init(void)
{
	_DBG_PROFILE("");

	connection = connman_dbus_get_connection();
	if (connection == NULL)
		return -1;

	if (connman_storage_register(&profile_storage) < 0)
		connman_error("Failed to register profile storage");

	profiles = g_hash_table_new_full(g_str_hash, g_str_equal,
						g_free, unregister_profile);

	return 0;
}

void __connman_profile_cleanup(void)
{
	_DBG_PROFILE("");

	if (connection == NULL)
		return;

	g_hash_table_destroy(profiles);
	profiles = NULL;

	connman_storage_unregister(&profile_storage);

	dbus_connection_unref(connection);
}
